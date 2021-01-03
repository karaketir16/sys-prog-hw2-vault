#ifndef __VAULT_H
#define __VAULT_H

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

typedef struct vault_key_t {
    char *ptr;
    int size;
} vault_key_t;


#define VAULT_IOC_MAGIC  'k'
#define VAULT_IOC_SET_KEY   _IOW(VAULT_IOC_MAGIC,  0, vault_key_t)
#define VAULT_IOC_CLEAR    _IO(VAULT_IOC_MAGIC, 2)
#define VAULT_IOC_MAXNR 2

#endif
