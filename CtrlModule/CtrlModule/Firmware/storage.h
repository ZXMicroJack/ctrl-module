#ifndef _STORAGE_H
#define _STORAGE_H

extern int GetBits(long filesize);
extern int LoadROM(const char *filename);
extern int LoadFile(const char *filename, void (*callback)(unsigned char *data));
extern int SaveFile(const char *fn, int (*callback)(unsigned char *data), long len);
extern int Open(const char *fn, DIRENTRY *p, void (*lbacb)(uint32_t));

#endif
