#include "minfat.h"
#include "disk.h"
#include "interrupts.h"
#include "misc.h"
#include "osd.h"
#include "host.h"

// control register (from ctrl-module to HDL)
#define HW_DISK_CR_ACK      1
#define HW_DISK_CR_SACK     16
#define HW_DISK_CR_ERR      8
#define HW_DISK_CR_DISK0IN   32

// status register (from HDL to ctrl-module)
#define HW_DISK_READSECTOR0  0x20000
#define HW_DISK_READSECTOR1  0x40000
#define HW_DISK_RESET   0x10000
#define HW_DISK_WRITESECTOR0 0x100000
#define HW_DISK_WRITESECTOR1 0x200000
#define HW_DISK_SR_BLOCKNR   0xffff

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

int (*preadSector[NR_DISKS])(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) = {0};
int (*pwriteSector[NR_DISKS])(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) = {0};

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
#endif

unsigned long nrReads = 0;

void DiskSignalsHandler(void) {

  unsigned long sr = HW_DISK_SR_R();
  if (sr & (HW_DISK_READSECTOR0|HW_DISK_WRITESECTOR0)) {
    diskReadBlock[0] = sr & HW_DISK_SR_BLOCKNR;
    if (sr & HW_DISK_WRITESECTOR0) diskReadBlock[0] |= DISK_BLOCK_WRITE;
    nrReads++;
  }
#if NR_DISKS>1
  if (sr & (HW_DISK_READSECTOR1|HW_DISK_WRITESECTOR1)) {
    diskReadBlock[1] = sr & HW_DISK_SR_BLOCKNR;
    if (sr & HW_DISK_WRITESECTOR1) diskReadBlock[1] |= DISK_BLOCK_WRITE;
    nrReads++;
  }
#endif

  if (sr & HW_DISK_RESET) {
    disk_cr &= ~(HW_DISK_CR_SACK|HW_DISK_CR_ERR);
    HW_DISK_CR_W(disk_cr);
  }
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
}
#endif

unsigned int diskLba[NR_DISKS][NR_DISK_LBA];

unsigned char disk_sector[BLOCK_SIZE];

void DiskHandlerSingle(int disk) {
  unsigned long mem, lba;
  int i;
  if (diskReadBlock[disk] != NOREQ) {
    if (!diskIsInserted[disk]) {
      diskReadBlock[disk] = NOREQ;
      disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;
      return;
    }

    unsigned char track = (diskReadBlock[disk] >> 8) & 0x7f;
    unsigned char sector = diskReadBlock[disk] & 0xff;
    unsigned char side = (diskReadBlock[disk] >> 15) & 1;

    if (diskReadBlock[disk] & DISK_BLOCK_WRITE) {
      // write to half of sector then write whole sector
      for (i=0; i<BLOCK_SIZE; i++) {
        disk_sector[i] = HW_DISK_DATA_R();
        HW_DISK_DATA_W(0x200);
        HW_DISK_DATA_W(0x000);
      }
      pwriteSector[disk](disk, side, track, sector, disk_sector);
      disk_cr |= HW_DISK_CR_SACK;
    } else if (preadSector[disk](disk, side, track, sector, disk_sector)) {
        // read
        for (i=0; i<BLOCK_SIZE; i++) {
          HW_DISK_DATA_W(disk_sector[i] | 0x100);
          HW_DISK_DATA_W(disk_sector[i] | 0x000);
        }
        disk_cr |= HW_DISK_CR_SACK;
    } else disk_cr |= HW_DISK_CR_SACK|HW_DISK_CR_ERR;

    diskReadBlock[disk] = NOREQ;
  }
}

void DiskHandler(void) {
  DiskSignalsHandler();
  DiskHandlerSingle(0);
#if NR_DISKS>1
  DiskHandlerSingle(1);
#endif
  HW_DISK_CR_W(disk_cr);
}

static unsigned int mem = 0, disk=0;
static void LbaCallback(unsigned int lba) {
  diskLba[disk][mem++] = lba;
}

int DiskOpen(int i, const char *fn, DIRENTRY *p) {
  disk = i;
  mem = 0;

  int len = Open(fn, p, LbaCallback);
  diskIsInserted[i] = len ? 1 : 0;
  
  disk_cr &= 0x00ffffff;
  disk_cr &= ~HW_DISK_CR_DISK0IN;
  HW_DISK_CR_W(disk_cr);
  

  unsigned char sector_id;
  if (DiskTryECPC(i, len, &sector_id)) {
    preadSector[i] = DiskReadSectorECPC;
    pwriteSector[i] = DiskWriteSectorECPC;
  } else if (DiskTryRAW(i, len, &sector_id)) {
    preadSector[i] = DiskReadSectorRAW;
    pwriteSector[i] = DiskWriteSectorRAW;
  } else {
    return 0;
  }
  disk_cr |= (sector_id << 24) | HW_DISK_CR_DISK0IN;
  HW_DISK_CR_W(disk_cr);
  
  return len ? 1 : 0;
}

int DiskClose(void) {
  disk_cr &= 0x00ffffff;
  diskIsInserted[0] = 0;
  return 0;
}

void DiskInit(void) {
	int i, j;
	for (i=0; i<NR_DISKS; i++) {
		diskIsInserted[i] = 0;
		diskReadBlock[i] = NOREQ;
		for (j=0; j<NR_DISK_LBA; j++) {
			diskLba[i][j] = 0;
		}
	}
	mem = 0;
	disk = 0;
	disk_cr = 0;
	laststs = 0;
  HW_DISK_CR_W(disk_cr);
}
