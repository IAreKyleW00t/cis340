#ifndef FAT12ENTRY_H
#define FAT12ENTRY_H

typedef struct {
    unsigned char FILENAME[8];
    unsigned char EXT[3];
    unsigned char ATTRIBUTES[1];
    unsigned char RESERVED[2];
    unsigned short CREATION_TIME;
    unsigned short CREATION_DATE;
    unsigned short LAST_ACCESS_DATE;
    unsigned short MODIFY_TIME;
    unsigned short MODIFY_DATE;
    unsigned short START_CLUSTER;
    unsigned long FILE_SIZE;
} Fat12Entry;

#endif
