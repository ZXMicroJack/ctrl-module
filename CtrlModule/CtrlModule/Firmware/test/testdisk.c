#include <stdlib.h>
#include <stdio.h>

#define INCLUDE_FILE_REMOVE
#define INCLUDE_ECPC
//#define STORE_LBAS

unsigned long HW_HOST_LL2(int reg) {
}

void write_data(unsigned int c);
unsigned char read_data(void);

#pragma pack(1)
#define debug(a) printf a

#if BLOCK_SIZE == 256
#define MAX_SECTOR  17
#define S1          0
#else
#define MAX_SECTOR  9
#ifdef AMSTRADCPC
#define S1          0xc1
#define BLOCKCALC   167
#else
#define S1          1
#define BLOCKCALC   788
#endif
#endif

unsigned int DISK_SR = 0;
unsigned int DISK_CR = 0;
unsigned int DISK_DATA = 0;

unsigned int TIMER_MS = 0;

#define HW_TIMER() TIMER_MS


#define HW_DISK_SR_R() DISK_SR
#define HW_DISK_CR_W(d) DISK_CR = d
// #define HW_DISK_DATA_R() DISK_DATA
// #define HW_DISK_DATA_W(d) DISK_DATA = d
#define HW_DISK_DATA_W(d) write_data(d)
#define HW_DISK_DATA_R() read_data()

void OSD_Puts(char *s);
//#include "../swap.c"

void OSD_ProgressBar(int n, int m) {
  printf("OSD_ProgressBar: %d / %d\n", n, m);
}

unsigned char diskbuff[512*1024];

void MEM_write(unsigned long address, unsigned char data) {
  // printf("MEM_write(%08X, %02X)\n", address, data);
  diskbuff[address] = data;
}

unsigned char MEM_read(unsigned long address) {
  // printf("MEM_read(%08X) returns %02X\n", address, diskbuff[address]);
  return diskbuff[address];
}

#include "../misc.c"
#include "../minfat.c"
#define UNDER_TEST
#include "../disk.c"
#include "../diskecpc.c"
#include "../diskraw.c"
#include "../storage.c"

#define WRITECOPY
#include "common.h"

typedef void (*handler_t)(void);

static handler_t handlers[IRQ_MAX] = {0};

void SetIntHandler(int irq, void (*new_handler)()) {
  handlers[irq] = new_handler;
}

/************************************************************************************/
void testDiskInit(void) {
  passif(DirCd("zx"), "cd zx");
  passif(DirCd("disks"), "cd disks");
  //passifeq(handlers[IRQ_DISK], NULL, "no ISR installed");
  DiskInit();
}

void diskhandler() {
  // read from CR -> write to SR
}


unsigned char sector0[] = {
  0x18, 0x05, 0x28, 0x12, 0x40, 0x4a, 0x0b, 0x7e, 0xdd, 0x77, 0x00, 0x23, 0x7e, 0xdd, 0x77, 0x01,
  0xdd, 0x7e, 0x02, 0xe6, 0x2f, 0x57, 0x23, 0x7e, 0xe6, 0xd0, 0xb2, 0xdd, 0x77, 0x02, 0xc9, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x40, 0xe5, 0x01, 0xf7, 0x18, 0x4e, 0x0c, 0x00, 0x03, 0xf5, 0x01, 0xfe, 0x01, 0x27, 0x01, 0x00,
  0x01, 0x07, 0x01, 0x01, 0x01, 0xf7, 0x16, 0x4e, 0x0c, 0x00, 0x03, 0xf5, 0x01, 0xfb, 0x40, 0xe5,
  0x40, 0xe5, 0x40, 0xe5, 0x40, 0xe5, 0x01, 0xf7, 0x18, 0x4e, 0x0c, 0x00, 0x03, 0xf5, 0x01, 0xfe,
  0x01, 0x27, 0x01, 0x00, 0x01, 0x0e, 0x01, 0x01, 0x01, 0xf7, 0x16, 0x4e, 0x0c, 0x00, 0x03, 0xf5,
  0x01, 0xfb, 0x40, 0xe5, 0x40, 0xe5, 0x40, 0xe5, 0x40, 0xe5, 0x01, 0xf7, 0x18, 0x4e, 0x0c, 0x00,
  0x03, 0xf5, 0x01, 0xfe, 0x01, 0x27, 0x01, 0x00, 0x01, 0x03, 0x01, 0x01, 0x01, 0xf7, 0x16, 0x4e,
  0x0c, 0x00, 0x03, 0xf5, 0x01, 0xfb, 0x40, 0xe5, 0x40, 0xe5, 0x40, 0xe5, 0x40, 0xe5, 0x01, 0xf7,
  0x18, 0x4e, 0x0c, 0x00, 0x03, 0xf5, 0x01, 0xfe, 0x01, 0x27, 0x01, 0x00, 0x01, 0x0a, 0x01, 0x01,
#if BLOCK_SIZE == 512
  0xff, 0x00, 0x00, 0x00, 0x06, 0x00, 0x44, 0x49, 0x53, 0x43, 0x20, 0x31, 0x20, 0x20, 0x20, 0x20,
  0xce, 0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x41, 0x64, 0x76, 0x2e, 0x64, 0x61, 0x74, 0x61, 0x20,
  0x52, 0x00, 0x08, 0x00, 0x08, 0x00, 0x72, 0x75, 0x6e, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
  0x81, 0x00, 0x09, 0x00, 0x17, 0x00, 0x6c, 0x6f, 0x67, 0x6f, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20,
  0xc6, 0x00, 0x18, 0x00, 0x1b, 0x00, 0x43, 0x6f, 0x6d, 0x70, 0x61, 0x6e, 0x2e, 0x74, 0x78, 0x74,
  0x06, 0x00, 0x1c, 0x00, 0x26, 0x00, 0x4f, 0x70, 0x75, 0x73, 0x76, 0x31, 0x2e, 0x74, 0x78, 0x74,
  0x06, 0x00, 0x27, 0x00, 0x2b, 0x00, 0x45, 0x78, 0x74, 0x72, 0x69, 0x63, 0x2e, 0x74, 0x78, 0x74,
  0x86, 0x00, 0x2c, 0x00, 0x67, 0x00, 0x44, 0x69, 0x73, 0x63, 0x45, 0x64, 0x2e, 0x74, 0x78, 0x74,
  0x86, 0x00, 0x68, 0x00, 0x7e, 0x00, 0x53, 0x2e, 0x44, 0x75, 0x6d, 0x70, 0x2e, 0x74, 0x78, 0x74,
  0x46, 0x00, 0x7f, 0x00, 0x81, 0x00, 0x4d, 0x2f, 0x66, 0x61, 0x63, 0x65, 0x2e, 0x74, 0x78, 0x74,
  0x86, 0x00, 0x82, 0x00, 0x86, 0x00, 0x43, 0x6f, 0x70, 0x69, 0x65, 0x72, 0x2e, 0x74, 0x78, 0x74,
  0xc6, 0x00, 0x87, 0x00, 0x8b, 0x00, 0x45, 0x6e, 0x64, 0x75, 0x72, 0x6f, 0x2e, 0x74, 0x78, 0x74,
  0xc6, 0x00, 0x8c, 0x00, 0x90, 0x00, 0x51, 0x75, 0x61, 0x7a, 0x61, 0x74, 0x2e, 0x74, 0x78, 0x74,
  0x46, 0x00, 0x91, 0x00, 0x9b, 0x00, 0x45, 0x73, 0x63, 0x61, 0x70, 0x65, 0x2e, 0x74, 0x78, 0x74,
  0xc6, 0x00, 0x9c, 0x00, 0xa0, 0x00, 0x53, 0x70, 0x65, 0x65, 0x64, 0x20, 0x2e, 0x74, 0x78, 0x74,
  0x8e, 0x00, 0xa1, 0x00, 0xaa, 0x00, 0x63, 0x6f, 0x6d, 0x70, 0x61, 0x6e, 0x64, 0x65, 0x72, 0x20
#endif
};

/************************************************************************************/
unsigned char buffer[512];
unsigned char buffer2[512];

unsigned char read_data(void) {

}

int wpos = 0;
void write_data(unsigned int c) {
  buffer[wpos] = c & 0xff;
  if (c & 0x100) {
    wpos ++;
  }
}

void reset_handler() {
  DISK_SR = 0;
  DiskHandler();
}

void read_sector(int disk, int sector) {
  printf("read_sector: disk:%d sector:%06x\n", disk, sector);
  DISK_SR = (disk ? HW_DISK_READSECTOR1 : HW_DISK_READSECTOR0) | sector;
  //handlers[IRQ_DISK]();
  DiskHandler();
//   DISK_SR = HW_DISK_RESET;
//   DiskHandler();
}

void seek_track(int disk, int track) {
  DISK_SR = (disk ? HW_DISK_MOVETRACK1 : HW_DISK_MOVETRACK0) | (track << 8);
  DiskHandler();
  DISK_SR = HW_DISK_RESET;
  DiskHandler();
}

void update_disk(int flags, int sector) {
  DISK_SR = flags | sector;
  //handlers[IRQ_DISK]();
  DiskHandler();
}

// void read_data(int disk, unsigned char *buffer, int n) {
//   DISK_SR = disk ? HW_DISK_READDATA1 : HW_DISK_READDATA0;
//   for (int i=0; i<n; i++) {
//     handlers[IRQ_DISK]();
//     buffer[i] = disk ? DISK_DATA1 : DISK_DATA0;
//   }
// }

int isBlank(unsigned char *buffer) {
  int i;
  for (i=0; i<BLOCK_SIZE; i++) {
    if (buffer[i] != 0xe5) break;
  }
  return i==BLOCK_SIZE;
}


void testHappyPath(int disk) {
  // int ack = (disk ? HW_DISK_CR_ACK1 : HW_DISK_CR_ACK0);
  // int fin = (disk ? HW_DISK_CR_FIN1 : HW_DISK_CR_FIN0);
  //passifeq(handlers[IRQ_DISK], DiskISR, "ISR installed");
  passif(!diskIsInserted[disk], "no disk inserted");
  passif(DiskOpen(disk, "SDCLD01.OPD", NULL), "open actual disk");
  passif(diskIsInserted[disk], "disk inserted");

  // read sector
  wpos = 0;
  read_sector(disk, STS(0,0,S1));
  passifeq(DISK_CR & HW_DISK_CR_SACK, HW_DISK_CR_SACK, "expecting ack");
  passifeq(wpos, BLOCK_SIZE, "expecting data");

  // DISK_SR = disk ? HW_DISK_READDATA1 : HW_DISK_READDATA0;
  // handlers[IRQ_DISK]();
  // passifeq(disk ? DISK_DATA1 : DISK_DATA0, sector0[0], "read first byte");
  // passifeq(DISK_CR, ack, "expecting ack");
  // buffer[0] = disk ? DISK_DATA1 : DISK_DATA0;
  //
  // read_data(disk, buffer+1, BLOCK_SIZE-1);

  // passif(DISK_CR == (ack|fin), "expecting ack and fin");
  passif(!memcmp(buffer, sector0, BLOCK_SIZE), "first sector compares ok");

#if BLOCK_SIZE == 512
  passifeq(DTStoBlock(disk, 0,39,MAX_SECTOR), BLOCKCALC, "check block calculation");
#else
  passifeq(DTStoBlock(disk, 0,39,MAX_SECTOR), 359, "check block calculation");
#endif
  wpos = 0;
#if BLOCK_SIZE != 512
  
  read_sector(disk, STS(0,39,MAX_SECTOR));
//   read_sector(disk, DTStoBlock(disk, 0,39,MAX_SECTOR));
  passifeq(256, wpos, "check read was done");
  // read_data(disk, buffer, BLOCK_SIZE);
  // read sector
//   update_disk(0, DTStoBlock(disk, 0,39,MAX_SECTOR));
  update_disk(0, STS(0,39,MAX_SECTOR));

  passifeq(DISK_CR & HW_DISK_CR_SACK, HW_DISK_CR_SACK, "expecting clear");
  passif(isBlank(buffer), "last sector compares ok");
//   hexdump(buffer, 512);
#endif
}

void testDiskFormats() {
#if BLOCK_SIZE == 512
//   unsigned long t1s0_sts = STS(0,1,1);

  ChangeDirectory(NULL);
#ifdef SAMCOUPE
  DirCd("sam");
  passif(DiskOpen(0, "samdos2.dsk", NULL), "open samdos2 disk");
  passifeq(DTStoBlock(0, 0,1,1), 20, "check 10 sectors per track format");
  passif(DiskOpen(0, "cpmdisk.cpm", NULL), "open cpm disk");
  passifeq(DTStoBlock(0, 0,1,1), 18, "check 9 sectors per track format");

// cpm 737280
// samdos2 819200
#endif
  
#endif
}

static unsigned long chksum(unsigned long sum, unsigned char b) {
  return ((sum << 8) | (sum >> 24)) ^ b;
}

static unsigned long calcchksum(unsigned char *d, int len) {
  unsigned long sum = 0;
  for (int i=0; i<len; i++) {
    sum = chksum(sum, d[i]);
  }
  return sum;
}

void testExtDiskFormat() {
#ifdef AMSTRADCPC
  int blkno = 0;
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
  dir(show);

  passif(DiskOpen(0, "bouldash.img", NULL), "open raw disk");
  DiskHandler();
  passifeq((DISK_CR>>24), 0xc1, "default sector number");
  
  blkno = STS(0,0,0xc1);
  wpos = 0;

  reset_handler();
  read_sector(0, blkno);
  if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));

//   return; //TODO remove

  blkno = STS(0,0,0xc2);
  wpos = 0;
  reset_handler();
  read_sector(0, blkno);
  if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));

  

  DiskClose(0);
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, 0, "no more sector number");
  
  // open dsk version of the same disk
  passif(DiskOpen(0, "bouldash.dsk", NULL), "open extended disk");
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "no more sector number");
  DiskHandler();
  passifeq((DISK_CR>>24), 0xc2, "default sector number");
  
  blkno = STS(0,0,0xc1);
  wpos = 0;
  read_sector(0, blkno);
  if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));

  blkno = STS(0,0,0xc2);
  wpos = 0;
  read_sector(0, blkno);
  if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));
    
#endif
}


// #undef STS
// #define STS(a,b,c) DTStoBlock(0,a,b,c)
void testExtDiskFormatAdvanced() {
#ifdef AMSTRADCPC
  int blkno = 0;
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
//   dir(show);
  
//   passifeq(STS(0,0,0xc1), 0, "first block");
//   passifeq(STS(0,0,0xc9), 8, "last block track 1 block");
// 
  reset_handler();

  passif(DiskOpen(0, "bouldash.img", NULL), "open raw disk");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "no more sector number");
//   passifeq((DISK_CR>>24), 0xc2, "default sector number");
  
  blkno = STS(0,0,0xc1);
  read_sector(0, blkno);
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));

  int nrchksums = 0;
  unsigned long chksums[MAX_BLOCK_NR];
  
  reset_handler();
  for (int t=0; t<40; t++) {
    for (int s=0; s<9; s++) {
      wpos = 0;
      blkno = STS(0,t,(0xc1+s));
      read_sector(0, blkno);
      if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
      unsigned long chk = calcchksum(buffer, 512);
      
      if (nrchksums < MAX_BLOCK_NR) {
        chksums[nrchksums++] = chk;
      } else fprintf(stderr, "TOO MANY SECTORS!\n");
      printf("%02d-%02d [%04X]: %08X\n", t, s, blkno, chk);
    }
  }
  passifeq(nrchksums, MAX_BLOCK_NR, "collected enough checksums");

  DiskClose(0);
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, 0, "no more sector number");
  DiskHandler();
//   passifeq((DISK_CR>>24), 0x00, "no more sector number");
  
  // open dsk version of the same disk
  passif(DiskOpen(0, "bouldash.dsk", NULL), "open extended disk");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "no more sector number");
//   passifeq((DISK_CR>>24), 0xc2, "default sector number");
  
  int ndx = 0;
  int nrcorrect = 0;
  for (int t=0; t<40; t++) {
    seek_track(0, t);
    for (int s=0; s<9; s++) {
      wpos = 0;
      blkno = STSX(0,0,t,(0xc1+s));
      read_sector(0, blkno);
      if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
      unsigned long chk = calcchksum(buffer, 512);
      
      if (ndx < nrchksums) {
        if (chksums[ndx] == chk) {
          nrcorrect ++;
        }
      } else fprintf(stderr, "TOO MANY SECTORS!\n");
      printf("%02d-%02d [%04X]: %08X (%08X)\n", t, s, blkno, chk, chksums[ndx]);
      ndx++;
    }
  }
  passifeq(nrchksums, nrcorrect, "checksums correct");
  return;
  
  unsigned char blk[512];
  unsigned char blk2[512];

  memset(blk, 0x33, sizeof blk);
  memset(blk2, 0x00, sizeof blk2);
  DiskWriteSectorECPC(0, 0, 33, 0xc1, blk);
  DiskReadSectorECPC(0, 0, 33, 0xc1, blk2, NULL);
  passif(!memcmp(blk, blk2, 512), "check blocks can be written and read back");

  memset(blk, 0x44, sizeof blk);
  memset(blk2, 0x00, sizeof blk2);
  DiskWriteSectorECPC(0, 0, 33, 0xc5, blk);
  DiskReadSectorECPC(0, 0, 33, 0xc5, blk2, NULL);
  passif(!memcmp(blk, blk2, 512), "check blocks can be written and read back");
  
  
  memset(blk, 0x55, sizeof blk);
  memset(blk2, 0x00, sizeof blk2);
  DiskWriteSectorECPC(0, 0, 34, 0xc1, blk);
  DiskReadSectorECPC(0, 0, 34, 0xc1, blk2, NULL);
  passif(!memcmp(blk, blk2, 512), "check blocks can be written and read back");

  memset(blk, 0x44, sizeof blk);
  memset(blk2, 0x00, sizeof blk2);
  DiskWriteSectorECPC(0, 0, 34, 0xc5, blk);
  DiskReadSectorECPC(0, 0, 34, 0xc5, blk2, NULL);
  passif(!memcmp(blk, blk2, 512), "check blocks can be written and read back");

  
  
#endif
}

void testExtDiskFormatAdvanced2() {
#ifdef AMSTRADCPC
  int blkno = 0;
  sectorNr[0] = 0;
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
//   dir(show);

  reset_handler();

  passif(DiskOpen(0, "XEVIOUS.IMG", NULL), "open raw disk");
  DiskHandler();
//   passifeq((DISK_CR>>24), 0xc1, "default sector number");
  
#if 1
  blkno = STS(0,0,0xc1);
  read_sector(0, blkno);
  fprintf(stderr, "[%04X]: %08X\n", blkno, calcchksum(buffer, 512));

  int nrchksums = 0;
  unsigned long chksums[MAX_BLOCK_NR];
  
  reset_handler();
  for (int t=0; t<40; t++) {
    for (int s=0; s<9; s++) {
      wpos = 0;
      blkno = STS(0,t,(0xc1+s));
      read_sector(0, blkno);
      if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
      unsigned long chk = calcchksum(buffer, 512);
      
      if (nrchksums < MAX_BLOCK_NR) {
        chksums[nrchksums++] = chk;
      } else fprintf(stderr, "TOO MANY SECTORS!\n");
      printf("%02d-%02d [%04X]: %08X\n", t, s, blkno, chk);
    }
  }
  passifeq(nrchksums, MAX_BLOCK_NR, "collected enough checksums");

  DiskClose(0);
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, 0, "no more sector number");
  
  // open dsk version of the same disk
  passif(DiskOpen(0, "XEVIOUS.DSK", NULL), "open extended disk");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "disk is in");
  
  int ndx = 0;
  int nrcorrect = 0;
  for (int t=0; t<40; t++) {
    seek_track(0, t);
    for (int s=0; s<9; s++) {
      wpos = 0;
      blkno = STS(0,t,(0xc1+s));
      read_sector(0, blkno);
      if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
      unsigned long chk = calcchksum(buffer, 512);
      
      if (ndx < nrchksums) {
        if (chksums[ndx++] == chk) {
          nrcorrect ++;
        }
      } else fprintf(stderr, "TOO MANY SECTORS!\n");
      printf("%02d-%02d [%04X]: %08X\n", t, s, blkno, chk);
    }
  }
  passifeq(nrchksums, nrcorrect, "checksums correct");
#endif
#endif
}



#ifdef AMSTRADCPC
void testExtFormatCP1() {
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
  passif(DiskOpen(0, "BATMANFR.DSK", NULL), "open batman forever disk (42 tracks 9-10 sectors)");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "disk is in");

  unsigned short sectorTests[] = {0x00c1, 0x00c9, 0x0101, 0x010a, 0x2801, 0x290a};
  unsigned long lastchk = calcchksum(buffer, 512);
  unsigned long chk;
  unsigned long blkno;
  
  for (int i=0; i<sizeof sectorTests / sizeof sectorTests[0]; i++) {
    wpos = 0;
    seek_track(0, sectorTests[i] >> 8);
    blkno = STS(0,sectorTests[i] >> 8,sectorTests[i] & 0xff);
    read_sector(0, blkno);
    passifeq(512, wpos, "check sector was read");
    chk = calcchksum(buffer, 512);
    
    passif(chk != lastchk, "read sector differently");
    if (chk == lastchk) {
      hexdump(buffer, 512);
    }
    lastchk = chk;
  }
  DiskClose(0);
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, 0, "disk is out");
}

void testread(int side, int track, int sector, unsigned long expchksum) {
  unsigned long chk, blkno;
  wpos = 0;
  seek_track(0, track);
  blkno = STSX(side, side,track,sector);
  fprintf(stderr, "blkno = %08X\n", blkno);
  read_sector(0, blkno);
  passifeq(DISK_CR & (HW_DISK_CR_SACK|HW_DISK_CR_ERR), HW_DISK_CR_SACK, "read ok");
  passifeq(512, wpos, "check sector was read");
  chk = calcchksum(buffer, 512);
  fprintf(stderr, "chk = %08X\n", chk);
  passifeq(chk, expchksum, "read sector correct");
}

void testExtFormatDSDD() {
  unsigned long chk, blkno;
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
  passif(DiskOpen(0, "ddle_2.dsk", NULL), "open ddle_2 disk (dsdd)");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "disk is in");

  memset(buffer, 0x00, 512);
  chk = calcchksum(buffer, 512);
  fprintf(stderr, "chk = %08X\n", chk);

  testread(0,2,4,0x9AE66CF8);
  testread(1,2,4,0xAEF7A810);
  DiskClose(0);
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, 0, "disk is out");
}


void testBlankDisk() {
  ChangeDirectory(NULL);
  DirCd("AMSTRA~1");
  
  DiskCreateBlank("DISK.DSK");
  
  passif(DiskOpen(0, "DISK.DSK", NULL), "open blank disk");
  DiskHandler();
  passifeq(DISK_CR & HW_DISK_CR_DISK0IN, HW_DISK_CR_DISK0IN, "default sector number");

  int nrcorrect = 0;
  for (int t=0; t<40; t++) {
    seek_track(0, t);
    for (int s=0; s<9; s++) {
      wpos = 0;
      unsigned long blkno = STS(0,t,(0xc1+s));
      read_sector(0, blkno);
      if (wpos != 512) fprintf(stderr, "FAILED TO READ!\n");
      if (isBlank(buffer)) nrcorrect++;
    }
  }
  passifeq(40*9, nrcorrect, "number of blank sectors correct");

}
#endif

#if 0
void testOverlappedRead(void) {
  read_sector(0, STS(0,0,0));
  read_sector(1, STS(0,39,MAX_SECTOR));

  read_data(0,buffer2,128);
  read_data(1,buffer,128);
  read_data(0,buffer2+128,256);
  read_data(1,buffer+128,128);
  read_data(0,buffer2+128+256,128);
  read_data(1,buffer+128+128,256);

  passif(!memcmp(buffer2, sector0, BLOCK_SIZE), "sector 0 reads ok");
  passif(isBlank(buffer), "sector 359 reads ok");
}

void testOverlappedRead2(void) {
  read_sector(0, STS(0,39,MAX_SECTOR));
  read_sector(1, STS(0,0,0));

  DISK_SR = HW_DISK_READDATA1 | HW_DISK_READDATA0;

  handlers[IRQ_DISK]();
  buffer[0] = DISK_DATA0;
  buffer2[0] = DISK_DATA1;
  passif(DISK_CR == (HW_DISK_CR_ACK1|HW_DISK_CR_ACK0), "both interfaces acked");

  for (int i=1; i<BLOCK_SIZE; i++) {
    handlers[IRQ_DISK]();
    buffer[i] = DISK_DATA0;
    buffer2[i] = DISK_DATA1;
  }
  // printf("DISK_CR = %08X\n", DISK_SR);
  passif(DISK_CR == (HW_DISK_CR_ACK1|HW_DISK_CR_ACK0|HW_DISK_CR_FIN1|HW_DISK_CR_FIN0), "both interfaces acked and finned");

  passif(!memcmp(buffer2, sector0, BLOCK_SIZE), "sector 0 reads ok");
  passif(isBlank(buffer), "sector 359 reads ok");
}
#endif

// void testTimer() {
//   TIMER_MS = 0;
//   passifeq(0, HW_TIMER(), "check test code works 1");
//   TIMER_MS = 333;
//   passifeq(333, HW_TIMER(), "check test code works 2");
//   TIMER_MS = 0;
//   
//   passifeq(333, TimerElapsed(0,333), "test positive one");
//   passifeq(333, TimerElapsed(-333,0), "test negative one");
//   passifeq(512, TimerElapsed(0xfffffe00, 0), "test elapsed overflows");
//   passifeq(512, TimerElapsed(0xffffff00, 0x100), "test elapsed overflows");
//   passifeq(512, TimerElapsed(0x00000000, 0x200), "test elapsed overflows");
//   passifeq(512, TimerElapsed(0x00000100, 0x300), "test elapsed overflows");
// }

//TODO: disk menus - increased memory
//TODO: add file offset finding
//TODO: add disk logic interface

//TODO: check emulation matches zpu specifics
//TODO: rom menus
//TODO: cleanup files
//TODO: Firmware size reduction somehow
//TODO: should not open file dialog

#define test(a,b) fprintf(stderr, "-----------------\nTESTING " # a "\n-----------------\n"); printf("-----------------\nTESTING " # a "\n-----------------\n"); test##a b

int main(int argc, char **argv) {
  openCard();

  passif(FindDrive(), "open disk");

  test(DiskInit,());
  test(HappyPath,(0));
#if NR_DISKS > 1
  test(HappyPath,(1));
#endif
  test(DiskFormats,());
  test(ExtDiskFormat,());
  test(ExtDiskFormatAdvanced,());
  test(ExtDiskFormatAdvanced2,());
#ifdef AMSTRADCPC
  test(BlankDisk,());
  test(ExtFormatCP1,());
  test(ExtFormatDSDD,());
#endif

  
  

// #ifdef AMSTRADCPC
//   test(ExtFormatDSDD,());
// #endif
  
//   test(Timer,());

  // test(OverlappedRead,());
  // test(OverlappedRead2,());
//  passifeq(STS_TO_BLOCK(0,39,MAX_SECTOR), 359, "check block calculation");

  closeCard();
  info();
  return tests_run - tests_passed;
}
