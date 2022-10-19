#include "misc.h"
#include "minfat.h"
#include "disk.h"
#include "interrupts.h"
#include "osd.h"
#include "host.h"

#ifndef debug
#define debug(a) {}
#endif

#define MAX_TRACKS 80
#define MAX_SIDES   2
static unsigned char ecpcTrackSize[NR_DISKS][MAX_TRACKS*MAX_SIDES];
static int ecpcTrackNr[NR_DISKS] = {0};
static char ecpcSides[NR_DISKS] = {0};

#define MAX_SECTORS 10
static unsigned char ecpcStatus[NR_DISKS][MAX_SIDES][MAX_SECTORS][8]; //c h r n st1 st2 len
static unsigned int ecpcTrackOffset[NR_DISKS][MAX_SIDES] = {0};

static int ecpcSectorNr[NR_DISKS] = {0};
static int ecpcCurrTrack[NR_DISKS] = {-1};
static int ecpcExtended[NR_DISKS] = {0};

const char trackInfoId[] = "Track-Info";
int DiskSeekECPC(int dsk, int trk) {
  int i, blk, j;
  unsigned char *ptr;
  
  // already at this track?
  if (trk == ecpcCurrTrack[dsk]) {
    return 1;
  }
  
  // locate track
  for (j=0; j<ecpcSides[dsk]; j++) {
    blk = 1;
    for (i=0; i<(trk*ecpcSides[dsk]+j); i++) {
      blk += ecpcTrackSize[dsk][i];
    }
    ecpcTrackOffset[dsk][j] = blk + 1;
    
    // read track record
    sd_read_sector(diskLba[dsk][blk>>1], sector_buffer);
    ptr = ((blk&1) == 1) ? &sector_buffer[256] : sector_buffer;
    
    // check track record
  //   hexdump(ptr, 256);
    if (compare(ptr, trackInfoId, 10)) {
      // not a track record
      debug(("Good track record\n"));
      return 0;
    }
    
    // read sector sizes
    ecpcSectorNr[dsk] = ptr[0x15];
    for (i=0; i<ecpcSectorNr[dsk]; i++) {
      memcpy(ecpcStatus[dsk][j][i], &ptr[0x18+i*8], 8);
      if (!ecpcExtended[dsk]) {
        ecpcStatus[dsk][j][i][6] = 0x00;
        ecpcStatus[dsk][j][i][7] = ptr[0x14];
      }
    }
  }
//   for (; i<MAX_SECTORS; i++) {
//     memset(ecpcStatus[i], 0xff, 8);
//   }
  ecpcCurrTrack[dsk] = trk;
  
  return 1;
}

int DiskGetSectorIdECPC(int dsk, int ndx, int side) {
  if (ndx < 0) return ecpcSectorNr[dsk];
  if (ndx < ecpcSectorNr[dsk]) return ecpcStatus[dsk][side][ndx][2] | (ecpcStatus[dsk][side][ndx][1] << 8);
  return -1;
}

//TODO only supports 512 byte sectors at the moment
static int DiskReadWriteSectorECPC(int dsk, int side, int track, unsigned char sector_id,
                                   unsigned char *sect, unsigned char *md, int read)  {
  unsigned long offset = 0;
  int i, s;
  
#if 0
  // move track
  if (!DiskSeekECPC(dsk, track)) {
    // failed
    return 0;
  }
#endif

  // find sector while adding offset
  s = side >> 7;
  side &= 0x7f;
  
  for (i=0; i<ecpcSectorNr[dsk]; i++) {
    if (ecpcStatus[dsk][s][i][2] == sector_id && ecpcStatus[dsk][s][i][1] == side) {
      break;
    } else {
      offset += (ecpcStatus[dsk][s][i][7]<<8)|ecpcStatus[dsk][s][i][6];
    }
  }
  
  // was sector found?
  if (i < ecpcSectorNr[dsk]) {
    unsigned char *ptr;
    int sectOffset = 0;
    
    unsigned int blk = ecpcTrackOffset[dsk][s] + (offset >> 8);
    unsigned int blkRead = 0;
    unsigned long thisBlk;
    
    if (md) {
      md[0] = ecpcStatus[dsk][s][i][0];
      md[1] = ecpcStatus[dsk][s][i][1];
      md[2] = ecpcStatus[dsk][s][i][2];
      md[3] = ecpcStatus[dsk][s][i][3];
      md[4] = ecpcStatus[dsk][s][i][4];
      md[5] = ecpcStatus[dsk][s][i][5];
    }
  
    do {
      // read track record
      thisBlk = diskLba[dsk][blk>>1];
      if (blkRead != thisBlk) {
        sd_read_sector(thisBlk, sector_buffer);
        blkRead = thisBlk;
      }
        
      ptr = ((blk&1) == 1) ? &sector_buffer[256] : sector_buffer;

      // copy into sector buffer
      if (read) {
        memcpy(&sect[sectOffset], ptr, 256);
      } else {
        memcpy(ptr, &sect[sectOffset], 256);
        if (sectOffset == 256 || (blk&1) == 1) {
          sd_write_sector(thisBlk, sector_buffer);
        }
      }
      sectOffset += 256;
      blk++;
    } while (sectOffset < 512);

    return 1;
  } else return 0;
}

int DiskReadSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, unsigned char *md) {
  return DiskReadWriteSectorECPC(dsk, side, track, sector_id, sect, md, 1);
}

int DiskWriteSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  return DiskReadWriteSectorECPC(dsk, side, track, sector_id, sect, NULL, 0);
}

static const char ecpcDiskID[]   = "EXTENDED";
static const char cpcemuDiskID[] = "MV - CPC";
int DiskTryECPC(int i, int len) {
  int n;
  
  // read first block
  sd_read_sector(diskLba[i][0], sector_buffer);

  // has recognised ID
  if (!compare(ecpcDiskID, sector_buffer, 8)) {
    // yes it's an extended crc disk
    debug(("It's an extended disk\n"));
    ecpcExtended[i] = 1;
    ecpcTrackNr[i] = sector_buffer[0x30];
    ecpcSides[i] = sector_buffer[0x31];

    if (ecpcTrackNr[i] > MAX_TRACKS) {
      debug(("Too many tracks\n"));
      return 0;
    }

    // note track sizes
    for (n=0; n<(ecpcTrackNr[i]*ecpcSides[i]); n++) {
      ecpcTrackSize[i][n] = sector_buffer[0x34+n];
    }
    
    ecpcCurrTrack[i] = -1;
    
    return 1;
  }

  // has recognised ID
  if (!compare(cpcemuDiskID, sector_buffer, 8)) {
    // yes it's an extended crc disk
    debug(("It's a standard CPCEMU disk\n"));
    ecpcTrackNr[i] = sector_buffer[0x30];
    ecpcSides[i] = sector_buffer[0x31];
    ecpcExtended[i] = 0;

    if (ecpcTrackNr[i] > MAX_TRACKS) {
      debug(("Too many tracks\n"));
      return 0;
    }

    // note track sizes
    for (n=0; n<(ecpcTrackNr[i]*ecpcSides[i]); n++) {
      ecpcTrackSize[i][n] = sector_buffer[0x33];
    }
    ecpcCurrTrack[i] = -1;
    
    return 1;
  }
  
  // not recognised
  return 0;
}

static const char diskCreateBlankText1[] = "EXTENDED Disk-File\r\nCPCEC 20220412\r\n\x1a"; // 0x25
static const char diskCreateBlankText2[] = "Track-Info\r\n"; // 0x25
static const unsigned char sectorIdLut[] = {0xc1, 0xc6, 0xc2, 0xc7, 0xc3, 0xc8, 0xc4, 0xc9, 0xc5};

static void DiskCreateBlankTrackRecord(unsigned char *data, int track) {
  int t;
  
  memcpy(data, diskCreateBlankText2, 12);
  data[0x10] = track; // track number
  data[0x11] = 0x00; // side number
  data[0x14] = 0x02; // sector size
  data[0x15] = 0x09; // sector number
  data[0x16] = 0x52; // sector number
  data[0x17] = 0xe5; // sector number

  // sector records
  for (t=0; t<9; t++) {
    data[0x18+t*8] = track; // track number
    data[0x18+t*8+1] = 0x00; // side number
    data[0x18+t*8+2] = sectorIdLut[t]; // sector id
    data[0x18+t*8+3] = 0x02; // sector len
    data[0x18+t*8+7] = 0x02; // sector len
  }
}


int diskCreateBlankBlocks = 0;
int diskCreateBlankTracks = 0;
static int DiskCreateBlankCallback(unsigned char *data) {
  int i;
  if (diskCreateBlankTracks == 0) {
    // first block
    memset(data, 0, 512);
    memcpy(data, diskCreateBlankText1, 0x25);
    data[0x30] = 40;
    data[0x31] = 1;
    memset(&data[0x34], 0x13, 40);
    
    // second block
    DiskCreateBlankTrackRecord(&data[0x100], 0);
    diskCreateBlankTracks++;
    diskCreateBlankBlocks = 0;
  } else {
    for (i=0; i<2; i++) {
      diskCreateBlankBlocks ++;
      if (diskCreateBlankBlocks == 19) {
        memset(data, 0x00, 256);
        DiskCreateBlankTrackRecord(data, diskCreateBlankTracks++);
        diskCreateBlankBlocks = 0;
      } else {
        memset(data, 0xe5, 256);
      }
      data += 256;
    }
  }
  return 1;
}

int DiskCreateBlank(char *fn) {
  diskCreateBlankTracks = 0;
	return SaveFile(fn, DiskCreateBlankCallback, 194816);
}
