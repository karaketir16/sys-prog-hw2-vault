#ifndef __SCULL_H
#define __SCULL_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

typedef struct vault_key_t {
    char *ptr;
    int size;
} vault_key_t;


#define SCULL_IOC_MAGIC  'k'
#define SCULL_IOC_SET_KEY   _IOW(SCULL_IOC_MAGIC,  0, vault_key_t)
#define SCULL_IOC_CLEAR    _IO(SCULL_IOC_MAGIC, 2)
#define SCULL_IOC_MAXNR 2

#endif
