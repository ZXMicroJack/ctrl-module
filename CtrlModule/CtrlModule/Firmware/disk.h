#ifndef _DISK_H
#define _DISK_H

#ifdef SAMCOUPE
#	define DISK_EXTENSION      ".DSK"
#	define DISK_BLOCKS   1600
#elif defined(AMSTRADCPC)
#	define DISK_EXTENSION      ".IMG"
#	define DISK_BLOCKS   360
# define WRITE_E5_TO_DISK
#else
#	define DISK_EXTENSION      ".OPD"
#	define DISK_BLOCKS   360
#endif
extern int DiskOpen(int i, const char *fn, DIRENTRY *p);
extern int DiskClose(int i);
extern void DiskInit(void);
extern void DiskHandler(void);

#ifndef NR_DISKS
#define NR_DISKS    2
#endif

int DiskWriteSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect);
int DiskReadSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, unsigned char *md);
int DiskTryECPC(int i, int len);
int DiskGetSectorIdECPC(int dsk, int ndx, int side);
int DiskSeekECPC(int dsk, int track);

int DiskWriteSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect);
int DiskReadSectorRAW(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, unsigned char *md);
int DiskTryRAW(int i, int len);
int DiskGetSectorIdRAW(int dsk, int ndx, int side);
int DiskSeekRAW(int dsk, int track);

#ifdef SAMCOUPE
#define MAX_BLOCK_NR 1600
#define NR_DISK_LBA   1600
#define SECTOR_COUNT_512 10
#define SECTOR_COUNT_512_CPM 9
#define SECTOR_COUNT_256 20
#elif defined(AMSTRADCPC)
#define MAX_BLOCK_NR 360
// #define NR_DISK_LBA   441
#define NR_DISK_LBA   1679
#define SECTOR_COUNT_512 9
#define SECTOR_COUNT_256 18
#else
#define MAX_BLOCK_NR 720
#define NR_DISK_LBA   360
#define SECTOR_COUNT_512 9
#define SECTOR_COUNT_256 18
#endif

extern int DiskCreateBlank(char *fn);
extern void DiskSetWp(int disk, int value);

#ifdef STORE_LBAS
extern unsigned int diskLba[2][NR_DISK_LBA];
#else
extern fileTYPE diskFile[2];
#endif


#endif
