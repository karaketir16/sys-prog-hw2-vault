#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 3, 0)
    #include <asm/switch_to.h>
#else
    #include <asm/system.h> /* cli(), *_flags */
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 16)
#define init_MUTEX(SEM)     sema_init(SEM, 1)
#endif
		
#include <asm/uaccess.h>	/* copy_*_user */


#include "encrypt.h"

#include "vault_ioctl.h"


#define VAULT_MAJOR 0
#define VAULT_NR_DEVS 4
#define INITIAL_KEY "abcd",4

int vault_major = VAULT_MAJOR;
int vault_minor = 0;
int vault_nr_devs = VAULT_NR_DEVS;

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct vault_dev {
    char_vector key;
    char_vector encrypted_text;
    int written;
    int readed;

    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct vault_dev *vault_devices;


// int vault_trim(struct vault_dev *dev)
// {
//     // int i;

//     // if (dev->data) {
//     //     for (i = 0; i < dev->qset; i++) {
//     //         if (dev->data[i])
//     //             kfree(dev->data[i]);
//     //     }
//     //     kfree(dev->data);
//     // }
//     // dev->data = NULL;
//     //dev->quantum = vault_quantum;
//     //dev->qset = vault_qset;
//     // dev->written = 0;
//     // dev->size = 0;
//     // dev->encrypted_text = CV_create(0);
//     return 0;
// }


int vault_open(struct inode *inode, struct file *filp)
{
    struct vault_dev *dev;

    dev = container_of(inode->i_cdev, struct vault_dev, cdev);
    filp->private_data = dev;

    /* trim the device if open was write-only */
    // if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
    //     if (down_interruptible(&dev->sem))
    //         return -ERESTARTSYS;
    //     vault_trim(dev);
    //     up(&dev->sem);
    // }
    return 0;
}


int vault_release(struct inode *inode, struct file *filp)
{
    return 0;
}

int i=0;
#define PRINT_CV(x) for(i=0;i<(x).size;i++) printk(KERN_CONT "%c", (x).data[i]);

ssize_t vault_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    struct vault_dev *dev = filp->private_data;
    ssize_t retval = 0;
    char_vector decrypted;

    printk("DEBUG Read entered, f_pos: %lld , count: %ld\n", *f_pos, count);

    if (down_interruptible(&dev->sem)){
        printk("Mutex err\n");
        return -ERESTARTSYS;
    }
        
    
    if( ! dev->written){
        retval = -ENOENT;
        printk("DEBUG No such file\n");
        goto out;
    }

    if(*f_pos >= dev->size){
        retval = 0;
        printk("End Of File\n");
        goto out;
    }

    if (*f_pos + count > dev->size)
        count = dev->size - *f_pos;

    // if (dev->size != count || *f_pos != 0) {//can not read part, just read full of the text
    //     retval = -ESRCH;	/* No such process */
    //     printk("DEBUG No such process \n");
    //     goto out;
    // }


    decrypted  = decrypt(dev->encrypted_text, dev->key);


    if (copy_to_user(buf, decrypted.data, count)) {
        retval = -EFAULT;
        goto out;
    }

    dev->readed = 1;

    printk("DEBUG Decrypted: ");
    PRINT_CV(decrypted);

    *f_pos += count;
    retval = count;

  out:
    printk("DEBUG Read End, retval: %ld", retval);
    up(&dev->sem);
    return retval;
}


ssize_t vault_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    struct vault_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;
    char_vector not_crypted;
    char_vector crypted;
    printk("DEBUG Write entered, count: %ld\n", count);

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;


    if (dev->written && ( ! dev->readed)){
        retval = -EEXIST;
        goto out;
    }


    not_crypted = CV_create(count);

    if (copy_from_user(not_crypted.data, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    crypted = encrypt(not_crypted, dev->key);
    CV_move(&not_crypted, &null_vector); //delete not cyrpted

    CV_move(&(dev->encrypted_text), &crypted);
    dev->written = 1;
    dev->readed = 0;
    dev->size = count;

    *f_pos += count;
    retval = count;

    printk("DEBUG Encrypted: ");
    PRINT_CV(dev->encrypted_text);

  out:
    printk("DEBUG Write End, retval: %ld", retval);
    up(&dev->sem);
    return retval;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
int vault_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
long vault_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    struct vault_dev *dev = filp->private_data;
    char_vector tmp_key;
    vault_key_t my_key;
    int i;
    char *key_ptr;

	int err = 0;
	int retval = 0;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != VAULT_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > VAULT_IOC_MAXNR) return -ENOTTY;

	/*
	 * the direction is a bitmask, and VERIFY_WRITE catches R/W
	 * transfers. `Type' is user-oriented, while
	 * access_ok is kernel-oriented, so the concept of "read" and
	 * "write" is reversed
	 */

#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 0, 0)
    err =  !access_ok((void __user *)arg, _IOC_SIZE(cmd));
    if (err) return -EFAULT;
#else
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err) return -EFAULT;
#endif

	switch(cmd) {
	  case VAULT_IOC_SET_KEY:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;

        
        if(copy_from_user((void *)&my_key, (void *)arg, sizeof(vault_key_t))) {
            retval = -EFAULT;
            return retval;
        }
        key_ptr =( char *) kmalloc(my_key.size * sizeof(char), GFP_KERNEL);
        if(copy_from_user((void *)key_ptr, (void *)my_key.ptr, my_key.size * sizeof(char))) {
            retval = -EFAULT;
            return retval;
        }
        
        printk("DEBUG key: ");
        for(i = 0; i < my_key.size; i++) printk(KERN_CONT "%c", key_ptr[i]);
        tmp_key = CV_create_from_cstr(key_ptr, my_key.size);
        CV_move(&(dev->key), &tmp_key); 

		break;

    case VAULT_IOC_CLEAR:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
        
        dev->size = 0;
        dev->written = 0;
        dev->readed = 0;
        CV_move(&(dev->encrypted_text), &null_vector); 

		break;


	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t vault_llseek(struct file *filp, loff_t off, int whence)
{
    struct vault_dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;

        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;

        case 2: /* SEEK_END */
            newpos = dev->size + off;
            break;

        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0)
        return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}


struct file_operations vault_fops = {
    .owner =    THIS_MODULE,
    .llseek =   vault_llseek,
    .read =     vault_read,
    .write =    vault_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    .ioctl =    vault_ioctl,
#else
    .unlocked_ioctl =    vault_ioctl,
#endif
    .open =     vault_open,
    .release =  vault_release,
};


void vault_cleanup_module(void)
{
    int i;    
    dev_t devno = MKDEV(vault_major, vault_minor);

    printk("DEBUG Entered clean");

    if (vault_devices) {
        for (i = 0; i < vault_nr_devs; i++) {
            // vault_trim(vault_devices + i);
            CV_move(&(vault_devices[i].encrypted_text), &null_vector); 
            CV_move(&(vault_devices[i].key), &null_vector); 

            cdev_del(&vault_devices[i].cdev);
        }
    kfree(vault_devices);
    }

    unregister_chrdev_region(devno, vault_nr_devs);

    printk("DEBUG End clean");
}


int vault_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct vault_dev *dev;

    if (vault_major) {
        devno = MKDEV(vault_major, vault_minor);
        result = register_chrdev_region(devno, vault_nr_devs, "vault");
    } else {
        result = alloc_chrdev_region(&devno, vault_minor, vault_nr_devs,
                                     "vault");
        vault_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "vault: can't get major %d\n", vault_major);
        return result;
    }

    vault_devices = kmalloc(vault_nr_devs * sizeof(struct vault_dev),
                            GFP_KERNEL);
    if (!vault_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(vault_devices, 0, vault_nr_devs * sizeof(struct vault_dev));

    /* Initialize each device. */
    for (i = 0; i < vault_nr_devs; i++) {
        dev = &vault_devices[i];

        dev->encrypted_text = null_vector;
        dev->key = CV_create_from_cstr(INITIAL_KEY);
        dev->size = 0;
        dev->written = 0;
        dev->readed = 0;

        init_MUTEX(&dev->sem);

        devno = MKDEV(vault_major, vault_minor + i);
        cdev_init(&dev->cdev, &vault_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &vault_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding vault%d", err, i);
    }

    return 0; /* succeed */

  fail:
    vault_cleanup_module();
    return result;
}

module_init(vault_init_module);
module_exit(vault_cleanup_module);
