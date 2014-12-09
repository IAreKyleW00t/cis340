#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "network.h"

static int cur_byte = 0;

void network_seek(int len, int mode) {
    if (mode == SEEK_SET_N) {
        cur_byte = len;
    } else if (mode == SEEK_CUR_N) {
        cur_byte += len;
    }
}

void network_read(void *buf, int len, unsigned char *bytes) {
    bzero((char *)buf, sizeof(buf));
    memcpy(buf, (bytes+cur_byte), len);
    cur_byte += len;
}
