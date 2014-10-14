#ifndef _ED_IOCTL_H
#define _ED_IOCTL_H
#define MAJOR_NUM_REC 200
#define MAJOR_NUM_TX  201
#define IOCTL_SET_BUSY _IOWR(MAJOR_NUM_TX,1,int)

#endif
