#ifndef _NETWORK_H
#define _NETWORK_H

#define SEEK_SET_N   1
#define SEEK_CUR_N   0

void network_seek(int len, int mode);
void network_read(void *buf, int len, unsigned char *bytes);

#endif
