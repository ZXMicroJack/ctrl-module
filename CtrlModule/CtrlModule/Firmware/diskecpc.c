#include "misc.h"
#include "minfat.h"
#include "disk.h"
#include "interrupts.h"
#include "osd.h"
#include "host.h"

#ifndef debug
#define debug(a) {}
#endif

#define MAX_TRACKS 43
static unsigned char ecpcTrackSize[MAX_TRACKS];
static int ecpcTrackNr = 0;

#define MAX_SECTORS 10
static unsigned char ecpcStatus[MAX_SECTORS][8]; //c h r n st1 st2 len
static unsigned int ecpcTrackOffset = 0;

static int ecpcSectorNr = 0;
static int ecpcCurrTrack = -1;
static int ecpcExtended = 0;

const char trackInfoId[] = "Track-Info";
static int DiskMoveTrackECPC(int dsk, int trk) {
  int i, blk = 1;
  unsigned char *ptr;
  
  // already at this track?
  if (trk == ecpcCurrTrack) {
    return 1;
  }
  
  // locate track
  for (i=0; i<trk; i++) {
    blk += ecpcTrackSize[i];
  }
  ecpcTrackOffset = blk + 1;
  
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
  ecpcSectorNr = ptr[0x15];
  for (i=0; i<ecpcSectorNr; i++) {
    memcpy(ecpcStatus[i], &ptr[0x18+i*8], 8);
    if (!ecpcExtended) {
      ecpcStatus[i][6] = 0x00;
      ecpcStatus[i][7] = ptr[0x14];
    }
  }
//   for (; i<MAX_SECTORS; i++) {
//     memset(ecpcStatus[i], 0xff, 8);
//   }
  ecpcCurrTrack = trk;
  
  return 1;
}

//TODO only supports 512 byte sectors at the moment
static int DiskReadWriteSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect, int read) {
  unsigned long offset = 0;
  int i;
  
  // move track
  if (!DiskMoveTrackECPC(dsk, track)) {
    // failed
    return 0;
  }
  
  // find sector while adding offset
  for (i=0; i<ecpcSectorNr; i++) {
    if (ecpcStatus[i][2] == sector_id) {
      break;
    } else {
      offset += (ecpcStatus[i][7]<<8)|ecpcStatus[i][6];
    }
  }
  
  // was sector found?
  if (i < ecpcSectorNr) {
    unsigned char *ptr;
    int sectOffset = 0;
    
    unsigned int blk = ecpcTrackOffset + (offset >> 8);
    unsigned int blkRead = 0;
    unsigned long thisBlk;
  
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

int DiskReadSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  return DiskReadWriteSectorECPC(dsk, side, track, sector_id, sect, 1);
}

int DiskWriteSectorECPC(int dsk, int side, int track, unsigned char sector_id, unsigned char *sect) {
  return DiskReadWriteSectorECPC(dsk, side, track, sector_id, sect, 0);
}

static const char ecpcDiskID[]   = "EXTENDED";
static const char cpcemuDiskID[] = "MV - CPC";
int DiskTryECPC(int i, int len, unsigned char *sector_id) {
  int n;
  
  // read first block
  sd_read_sector(diskLba[i][0], sector_buffer);

  // has recognised ID
  if (!compare(ecpcDiskID, sector_buffer, 8)) {
    // yes it's an extended crc disk
    debug(("It's an extended disk\n"));
    ecpcExtended = 1;
    ecpcTrackNr = sector_buffer[0x30];

    if (ecpcTrackNr > MAX_TRACKS) {
      debug(("Too many tracks\n"));
      return 0;
    }

    // note track sizes
    for (n=0; n<ecpcTrackNr; n++) {
      ecpcTrackSize[n] = sector_buffer[0x34+n];
    }
    
    ecpcCurrTrack = -1;
    DiskMoveTrackECPC(i, 0);
    
    *sector_id = ecpcStatus[0][2];
    
    return 1;
  }

  // has recognised ID
  if (!compare(cpcemuDiskID, sector_buffer, 8)) {
    // yes it's an extended crc disk
    debug(("It's a standard CPCEMU disk\n"));
    ecpcTrackNr = sector_buffer[0x30];
    ecpcExtended = 0;

    if (ecpcTrackNr > MAX_TRACKS) {
      debug(("Too many tracks\n"));
      return 0;
    }

    // note track sizes
    for (n=0; n<ecpcTrackNr; n++) {
//       ecpcTrackSize[n] = sector_buffer[0x34+n];
      ecpcTrackSize[n] = sector_buffer[0x33];
    }
    
    DiskMoveTrackECPC(i, 0);
    
    *sector_id = ecpcStatus[0][2];
    
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
