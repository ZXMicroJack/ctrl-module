#include "minfat.h"
#include "disk.h"
#include "interrupts.h"
#include "misc.h"
#include "osd.h"
#include "host.h"

// control register (from ctrl-module to HDL)
// #define HW_DISK_CR_ACK      1
#define HW_DISK_CR_SACK     16
#define HW_DISK_CR_ERR      8
#define HW_DISK_CR_DISK0IN   32
#define HW_DISK_CR_DISK1IN   64
#define HW_DISK_CR_SEEK0ACK     1
#define HW_DISK_CR_SEEK1ACK     2
#define HW_DISK_CR_WPERROR     4

// status register (from HDL to ctrl-module)
// #define HW_DISK_RESET         0x80000000
// #define HW_DISK_READSECTOR0   0x20000
// #define HW_DISK_READSECTOR1   0x40000
// #define HW_DISK_WRITESECTOR0  0x100000
// #define HW_DISK_WRITESECTOR1  0x200000
// #define HW_DISK_NEXTSECTORID0 0x400000
// #define HW_DISK_NEXTSECTORID1 0x800000
// #define HW_DISK_MOVETRACK0    0x1000000
// #define HW_DISK_MOVETRACK1    0x2000000

#define HW_DISK_RESET         0x00800000
#define HW_DISK_READSECTOR0   0x01000000
#define HW_DISK_READSECTOR1   0x02000000
#define HW_DISK_WRITESECTOR0  0x04000000
#define HW_DISK_WRITESECTOR1  0x08000000
#define HW_DISK_NEXTSECTORID0 0x10000000
#define HW_DISK_NEXTSECTORID1 0x20000000
#define HW_DISK_MOVETRACK0    0x40000000
#define HW_DISK_MOVETRACK1    0x80000000

#define HW_DISK_SR_BLOCKNR    0x007fffff
#define HW_DISK_SR_CMDMASK    0xff000000


// general defines / macros
#define NOREQ   0xffffffff
#define DISK_BLOCK_NUMBER_MASK 0x7fffffff
#define DISK_BLOCK_WRITE 0x80000000

#ifndef UNDER_TEST
#define HW_DISK_SR_R() *(volatile unsigned int *)(0xFFFFFA10)
#define HW_DISK_CR_W(d) *(volatile unsigned int *)(0xFFFFFA10) = d
#define HW_DISK_DATA_R() *(volatile unsigned int *)(0xFFFFFA14)
#define HW_DISK_DATA_W(d) *(volatile unsigned int *)(0xFFFFFA14) = d
#endif

unsigned long disk_cr = 0;

#ifndef debug
#define debug(a) {}
#endif


static unsigned char diskIsInserted[NR_DISKS];
static unsigned long diskReadBlock[NR_DISKS];
unsigned long laststs = 0;
char diskwp[NR_DISKS];

int (*preadSector[NR_DISKS])(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, unsigned char *md) = {0};
int (*pwriteSector[NR_DISKS])(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) = {0};
int (*pgetSectorId[NR_DISKS])(int dsk, int ndx, int side) = {0};
int (*pseek[NR_DISKS])(int dsk, int track) = {0};

int DiskReadSectorNone(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, unsigned char *md) { return 1; }
int DiskWriteSectorNone(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) { return 1; }
int DiskGetSectorIdNone(int dsk, int ndx, int side) { return 1; }
int DiskSeekNone(int dsk, int track) { return 1; }

#ifdef FDDDEBUG
extern void OSD_Putchar(int c);
extern void intToStringNoPrefix(char *s, unsigned int n);

void puts(char *s) {
  while (*s) OSD_Putchar(*s++);
}

void putln() {
  OSD_Putchar('\n');
}

void puth(unsigned int a) {
  char s[9];
  intToStringNoPrefix(s, a);
  s[8] = '\0';
  puts(s);
}

extern int stopmenus;
static void dbg() {
  if (stopmenus) {
    OSD_Putchar('-');
    OSD_Putchar('-');
    puth(HW_DISK_SR_R()); 
    OSD_Putchar('-');
    puth(disk_cr);
    putln();
  }
}
#else
#define dbg() {}
#endif

unsigned long nrReads = 0;

unsigned char sectorNr[NR_DISKS] = {0};

unsigned int lastSr = 0;

static void DiskNextSectorId(int disk, int side) {
  int sector_id;
  
  disk_cr &= 0x00ff00ff;
  sector_id = pgetSectorId[disk](disk, sectorNr[disk]++, side);
  if (sector_id < 0) {
    sectorNr[disk] = 0;
    sector_id = pgetSectorId[disk](disk, sectorNr[disk]++, side);
  }
  
  if (sector_id >= 0) {
    disk_cr |= ((sector_id & 0xff) << 24) | (sector_id & 0xff00);
  } else {
    disk_cr |= HW_DISK_CR_ERR;
  }
  disk_cr |= HW_DISK_CR_SACK;
}


unsigned char lastTrack[NR_DISKS] = {0};
void DiskSignalsHandler(void) {
  unsigned long sr = HW_DISK_SR_R();
  if (lastSr == sr) return;
  
  if (sr & (HW_DISK_READSECTOR0|HW_DISK_WRITESECTOR0)) {
    diskReadBlock[0] = sr & HW_DISK_SR_BLOCKNR;
    if (sr & HW_DISK_WRITESECTOR0) diskReadBlock[0] |= DISK_BLOCK_WRITE;
    lastTrack[0] = (sr >> 8) & 0x7f;
    nrReads++;
  }
#if NR_DISKS>1
  if (sr & (HW_DISK_READSECTOR1|HW_DISK_WRITESECTOR1)) {
    diskReadBlock[1] = sr & HW_DISK_SR_BLOCKNR;
    if (sr & HW_DISK_WRITESECTOR1) diskReadBlock[1] |= DISK_BLOCK_WRITE;
    lastTrack[1] = (sr >> 8) & 0x7f;
    nrReads++;
  }
#endif

  if (sr & HW_DISK_RESET) {
    disk_cr &= ~(HW_DISK_CR_SACK|HW_DISK_CR_ERR|HW_DISK_CR_SEEK1ACK|HW_DISK_CR_SEEK0ACK|HW_DISK_CR_WPERROR);
    HW_DISK_CR_W(disk_cr);
//     dbg();
  }
  
#define disk 0
  if (sr & HW_DISK_NEXTSECTORID0) {
    DiskNextSectorId(disk, (sr >> 22) & 1);
    dbg();
  }
  
  if (sr & HW_DISK_MOVETRACK0) {
    if (pseek[disk](disk, (sr >> 8) & 0x7f)) {
      lastTrack[0] = (sr >> 8) & 0x7f;
      sectorNr[disk] = 0;
//       DiskNextSectorId(disk);
      disk_cr |= HW_DISK_CR_SEEK0ACK;
    } else disk_cr |= HW_DISK_CR_SEEK0ACK|HW_DISK_CR_ERR;
    dbg();
  }
#undef disk

#if NR_DISKS>1
#define disk 1
  if (sr & HW_DISK_NEXTSECTORID1) {
    DiskNextSectorId(disk, (sr >> 22) & 1);
    dbg();
  }
  
  if (sr & HW_DISK_MOVETRACK1) {
    lastTrack[1] = (sr >> 8) & 0x7f;
    if (pseek[disk](disk, (sr >> 8) & 0x7f)) {
      sectorNr[disk] = 0;
//       DiskNextSectorId(disk);
      disk_cr |= HW_DISK_CR_SEEK1ACK;
    } else disk_cr |= HW_DISK_CR_SEEK1ACK|HW_DISK_CR_ERR;
    dbg();
  }
#undef disk
#endif
  lastSr = sr;
  HW_DISK_CR_W(disk_cr);
}

#ifdef FDDDEBUG
void diskDebug(void) {
  puts("readblock:"); puth(diskReadBlock[0]); putln();
  puts(" cr:"); puth(disk_cr); putln();
  puts(" sr:"); puth(HW_DISK_SR_R()); putln();
//   puts(" laststs:"); puth(laststs); putln();
  puts(" nrReads:"); puth(nrReads); putln();
  puts(" debug: "); puth((*(unsigned int *)DEBUGSTATE)); putln();
  puts(" debug2:"); puth((*(unsigned int *)DEBUGSTATE2)); putln();
//   puts(" timer:"); puth(HW_TIMER()); putln();
  
}
#endif

#ifdef STORE_LBAS
unsigned int diskLba[2][NR_DISK_LBA];
#else
fileTYPE diskFile[2];
#endif

unsigned char disk_sector[BLOCK_SIZE];

// unsigned int TimerElapsed(unsigned int then, unsigned int now) {
//   return now - then;
// }

// unsigned int lastSectorUpdate = 0;
// unsigned char sectorNr = 0;
void DiskHandlerSingle(int disk) {
  unsigned long mem, lba, now;
  int sector_id;
  int i;
  unsigned char md[6];
  
  if (diskReadBlock[disk] != NOREQ) {
    if (!diskIsInserted[disk]) {
      diskReadBlock[disk] = NOREQ;
      disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;
      return;
    }

    unsigned char track = (diskReadBlock[disk] >> 8) & 0x7f;
    unsigned char sector = diskReadBlock[disk] & 0xff;
    unsigned char side = (diskReadBlock[disk] >> 15) & 0xff;

    if (diskReadBlock[disk] & DISK_BLOCK_WRITE) {
      // is write protected?
      if (diskwp[disk]) {
        disk_cr &= 0x00ffffff;
        disk_cr |= HW_DISK_CR_SACK | (sector << 24) | HW_DISK_CR_ERR | HW_DISK_CR_WPERROR;
      } else {
        // write to half of sector then write whole sector
        for (i=0; i<BLOCK_SIZE; i++) {
          disk_sector[i] = HW_DISK_DATA_R();
          HW_DISK_DATA_W(0x200);
          HW_DISK_DATA_W(0x000);
        }
        pwriteSector[disk](disk, side, track, sector, disk_sector);
        disk_cr &= 0x00ffffff;
        disk_cr |= HW_DISK_CR_SACK | (sector << 24);
      }
    } else if (preadSector[disk](disk, side, track, sector, disk_sector, md)) {
        // read
        for (i=0; i<BLOCK_SIZE; i++) {
          HW_DISK_DATA_W(disk_sector[i] | 0x100);
          HW_DISK_DATA_W(disk_sector[i] | 0x000);
        }
        disk_cr |= HW_DISK_CR_SACK;
        disk_cr &= 0x00ff00ff;
        disk_cr |= (md[1] << 8) | (sector << 24);
    } else disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;

    diskReadBlock[disk] = NOREQ;
    dbg();
  }
  
  // spin disk id
//   now = HW_TIMER();
//   if (TimerElapsed(lastSectorUpdate, now) > 20) {
//     if (diskIsInserted[disk]) {
//       disk_cr &= 0x00ffffff;
//       sector_id = pgetSectorId[disk](disk, sectorNr++);
//       if (sector_id < 0) {
//         sectorNr = 0;
//         sector_id = pgetSectorId[disk](disk, sectorNr++);
//       }
//       if (sector_id >= 0) {
//         disk_cr |= (sector_id & 0xff) << 24;
//       }
//     }
//     lastSectorUpdate = now;
//   }
}

void DiskHandler(void) {
  DiskSignalsHandler();
  DiskHandlerSingle(0);
#if NR_DISKS>1
  DiskHandlerSingle(1);
#endif
  HW_DISK_CR_W(disk_cr);
}

#ifdef STORE_LBAS
static unsigned int mem = 0, disk=0;
static void LbaCallback(unsigned int lba) {
  diskLba[disk][mem++] = lba;
}
#endif

void DiskSetWp(int disk, int value) {
  diskwp[disk] = value;
}


int DiskOpen(int i, const char *fn, DIRENTRY *p) {
#ifdef STORE_LBAS
  disk = i;
  mem = 0;
#endif

  DiskClose(i);
//   HW_DISK_CR_W(disk_cr);

#ifdef STORE_LBAS
  int len = Open(fn, p, LbaCallback);
#else
  int len = OpenFile(&diskFile[i], fn, p);
#endif
  if (len == 0) return 0;
  
#ifdef INCLUDE_ECPC
  if (DiskTryECPC(i, len)) {
    preadSector[i] = DiskReadSectorECPC;
    pwriteSector[i] = DiskWriteSectorECPC;
    pgetSectorId[i] = DiskGetSectorIdECPC;
    pseek[i] = DiskSeekECPC;
  } else
#endif
#ifndef EXCLUDE_RAW
    if (DiskTryRAW(i, len)) {
    preadSector[i] = DiskReadSectorRAW;
    pwriteSector[i] = DiskWriteSectorRAW;
    pgetSectorId[i] = DiskGetSectorIdRAW;
    pseek[i] = DiskSeekRAW;
  } else
#endif
  {
    preadSector[i] = DiskReadSectorNone;
    pwriteSector[i] = DiskWriteSectorNone;
    pgetSectorId[i] = DiskGetSectorIdNone;
    pseek[i] = DiskSeekNone;
    return 0;
  }
  diskIsInserted[i] = 1;

  // seek to last track and fetch next sector id from that track
  pseek[i](i, lastTrack[i]);
  sectorNr[i] = 0;
//   DiskNextSectorId(disk);
  
  if (i) {
    disk_cr |= HW_DISK_CR_DISK1IN;
  } else {
    disk_cr |= HW_DISK_CR_DISK0IN;
  }
  HW_DISK_CR_W(disk_cr);
  
  return 1;
}

int DiskClose(int n) {
  diskIsInserted[n] = 0;
  if (n) {
    disk_cr &= ~HW_DISK_CR_DISK1IN;
  } else {
    disk_cr &= ~HW_DISK_CR_DISK0IN;
  }
  HW_DISK_CR_W(disk_cr);
  return 0;
}

void DiskInit(void) {
	int i, j;
	for (i=0; i<NR_DISKS; i++) {
		diskIsInserted[i] = 0;
		diskReadBlock[i] = NOREQ;
#ifdef STORE_LBAS
		for (j=0; j<NR_DISK_LBA; j++) {
			diskLba[i][j] = 0;
		}
#endif
    preadSector[i] = DiskReadSectorNone;
    pwriteSector[i] = DiskWriteSectorNone;
    pgetSectorId[i] = DiskGetSectorIdNone;
    pseek[i] = DiskSeekNone;
    diskwp[i] = 1;
	}
#ifdef STORE_LBAS
	mem = 0;
	disk = 0;
#endif
	disk_cr = 0;
	laststs = 0;
  HW_DISK_CR_W(disk_cr);
}
