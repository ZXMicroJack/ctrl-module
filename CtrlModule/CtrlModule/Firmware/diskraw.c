#include "minfat.h"
#include "disk.h"
#include "interrupts.h"
#include "misc.h"
#include "osd.h"
#include "host.h"


#ifdef SAMCOUPE
#define MAX_BLOCK_NR 1600
#define SECTOR_COUNT_512 10
#define SECTOR_COUNT_512_CPM 9
#define SECTOR_COUNT_256 20

int isCPM[NR_DISKS];

#elif defined(AMSTRADCPC)
#define MAX_BLOCK_NR 360
#define SECTOR_COUNT_512 9
#define SECTOR_COUNT_256 18
#else
#define MAX_BLOCK_NR 720
#define NR_DISK_LBA   360
#define SECTOR_COUNT_512 9
#define SECTOR_COUNT_256 18
#endif


#if BLOCK_SIZE == 512
#ifdef SAMCOUPE
# define STS_TO_BLOCK(side, track, sector) 
# define STS_TO_BLOCK_CPM(side, track, sector) (((((track)*2)+(side))*9)+(sector)-1)
#elif defined(AMSTRADCPC)
# define STS_TO_BLOCK(side, track, sector) ((side)*40+((track)*SECTOR_COUNT_512)+((sector)-0xc1))
#else
# define STS_TO_BLOCK(side, track, sector) ((side)*80+(track*SECTOR_COUNT_512)+(sector))
#endif

#define BLOCK_OFFSET(sector) (0)
#elif BLOCK_SIZE == 256
#define STS_TO_BLOCK(side, track, sector) (((side)*80+(track*SECTOR_COUNT_256)+(sector))>>1)
#define BLOCK_OFFSET(sector) ((sector)&1) ? BLOCK_SIZE : 0
#endif

// #ifdef AMSTRADCPC
#define STS(side, track, sector) ((((side)&1)<<15)|(((track)&0x7f)<<8)|((sector)&0xff))
// #else
// #define STS(side, track, sector) ((((side)&1)<<12)|(((track)&0x7f)<<5)|((sector)&0x1f))
// #endif

#ifdef SAMCOUPE
int DTStoBlock(int drv, int side, int track, int sector) {
  return (((((track)*2)+(side))*(isCPM[drv] ? SECTOR_COUNT_512_CPM : SECTOR_COUNT_512))+(sector)-1);
}
#elif defined(AMSTRADCPC)
int DTStoBlock(int drv, int side, int track, int sector) {
  return ((side)*40+((track)*SECTOR_COUNT_512)+((sector)-0xc1));
}
#else
int DTStoBlock(int drv, int side, int track, int sector) {
  printf("DTStoBlock(drv:%d, side:%d, track:%d, sector:%d)\n", drv, side, track, sector);
  return ((side)*80+(track*SECTOR_COUNT_256)+(sector))>>1;
}
#endif

#if BLOCK_SIZE==256
int DiskWriteSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  unsigned long blk = DTStoBlock(dsk, side, track, sector_id);
  if (blk >= NR_DISK_LBA) return 0;
  
  unsigned long lba = diskLba[disk][blk];
  if (lba >= MaxLba()) return 0;
  
  if (!sd_read_sector(lba, sector_buffer)) return 0;
  
  if (sector_id & 1) {
    memcpy(&sector_buffer[256], sect, 256);
  } else {
    memcpy(sector_buffer, sect, 256);
  }
  
  return sd_write_sector(lba, sector_buffer);
}

int DiskReadSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  unsigned long blk = DTStoBlock(dsk, side, track, sector_id);
  printf("%d/%d/%d\n", side, track, sector_id);
  printf("blk: %d\n", blk);
  if (blk >= NR_DISK_LBA) return 0;
  
  unsigned long lba = diskLba[disk][blk];
  if (lba >= MaxLba()) return 0;
  
  if (!sd_read_sector(lba, sector_buffer)) return 0;
  
  if (sector_id & 1) {
    memcpy(sect, &sector_buffer[256], 256);
  } else {
    memcpy(sect, sector_buffer, 256);
  }
  
//   return 1;
}
#else
int DiskWriteSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  unsigned long lba, blk;
  
  blk = DTStoBlock(dsk, side, track, sector_id);
  if (blk >= NR_DISK_LBA) return 0;
  
  lba = diskLba[dsk][blk];
  if (lba >= MaxLba()) return 0;

  return (lba < MaxLba() && sd_write_sector(lba, sect)) ? 1 : 0;
//   return 1;
}

int DiskReadSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  unsigned long lba, blk;
  
  blk = DTStoBlock(dsk, side, track, sector_id);
  if (blk >= NR_DISK_LBA) return 0;
  
  lba = diskLba[dsk][blk];
  if (lba >= MaxLba()) return 0;


  return sd_read_sector(lba, sect);
//   int result = sd_read_sector(lba, sect);
//   
//   hexdump(sect, 512);
//   return result;
//   return (lba < MaxLba() && sd_read_sector(lba, sect)) ? 1 : 0;
}
#endif

int DiskTryRAW(int i, int len, unsigned char *sector_id) {
#ifdef SAMCOUPE
  isCPM[i] = len == 737280;
#endif
#ifdef AMSTRADCPC
  debug(("len = %d\n", len));
  if (len == 184320) {
    *sector_id = 0xc1;
    return 1;
  }
  return 0;
#endif
  return 1;
}
