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

#include "scull_ioctl.h"


#define SCULL_MAJOR 0
#define SCULL_NR_DEVS 4

int scull_major = SCULL_MAJOR;
int scull_minor = 0;
int scull_nr_devs = SCULL_NR_DEVS;

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct scull_dev {
    char_vector key;
    char_vector encrypted_text;
    int written;
    int readed;

    unsigned long size;
    struct semaphore sem;
    struct cdev cdev;
};

struct scull_dev *scull_devices;


int scull_trim(struct scull_dev *dev)
{
    // int i;

    // if (dev->data) {
    //     for (i = 0; i < dev->qset; i++) {
    //         if (dev->data[i])
    //             kfree(dev->data[i]);
    //     }
    //     kfree(dev->data);
    // }
    // dev->data = NULL;
    //dev->quantum = scull_quantum;
    //dev->qset = scull_qset;
    // dev->written = 0;
    // dev->size = 0;
    // dev->encrypted_text = CV_create(0);
    return 0;
}


int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev;

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;

    /* trim the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        scull_trim(dev);
        up(&dev->sem);
    }
    return 0;
}


int scull_release(struct inode *inode, struct file *filp)
{
    return 0;
}

#define PRINT_CV(x) int i=0;for(i=0;i<(x).size;i++) printk(KERN_CONT "%c", (x).data[i]);

ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                   loff_t *f_pos)
{
    printk("DEBUG Read entered, f_pos: %d , count: %d\n", *f_pos, count);
    struct scull_dev *dev = filp->private_data;
    ssize_t retval = 0;

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


    char_vector decrypted = decrypt(dev->encrypted_text, dev->key);


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
    printk("DEBUG Read End, retval: %d", retval);
    up(&dev->sem);
    return retval;
}


ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                    loff_t *f_pos)
{
    printk("DEBUG Write entered, count: %d\n", count);
    struct scull_dev *dev = filp->private_data;
    ssize_t retval = -ENOMEM;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;


    if (dev->written && ( ! dev->readed)){
        retval = -EEXIST;
        goto out;
    }


    char_vector not_crypted = CV_create(count);

    if (copy_from_user(not_crypted.data, buf, count)) {
        retval = -EFAULT;
        goto out;
    }

    dev->encrypted_text = encrypt(not_crypted, dev->key);
    dev->written = 1;
    dev->readed = 0;
    dev->size = count;

    *f_pos += count;
    retval = count;

    printk("DEBUG Encrypted: ");
    PRINT_CV(dev->encrypted_text);

  out:
    printk("DEBUG Write End, retval: %d", retval);
    up(&dev->sem);
    return retval;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
int scull_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
long scull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    struct scull_dev *dev = filp->private_data;

	int err = 0, tmp;
	int retval = 0;
	/*
	 * extract the type and number bitfields, and don't decode
	 * wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
	 */
	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

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
	  case SCULL_IOC_SET_KEY:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;

        vault_key_t my_key;
        copy_from_user((void *)&my_key, (void *)arg, sizeof(vault_key_t));
        int i;
        printk("DEBUG key: ");
        for(i = 0; i < my_key.size; i++) printk(KERN_CONT "%c", my_key.ptr[i]);

        dev->key = CV_create_from_cstr(my_key.ptr, my_key.size);

		break;

    case SCULL_IOC_CLEAR:
		if (! capable (CAP_SYS_ADMIN))
			return -EPERM;
        
        dev->size = 0;
        dev->written = 0;
        dev->readed = 0;
        dev->encrypted_text = null_vector;

		break;


	  default:  /* redundant, as cmd was checked against MAXNR */
		return -ENOTTY;
	}
	return retval;
}


loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
    struct scull_dev *dev = filp->private_data;
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


struct file_operations scull_fops = {
    .owner =    THIS_MODULE,
    .llseek =   scull_llseek,
    .read =     scull_read,
    .write =    scull_write,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36)
    .ioctl =    scull_ioctl,
#else
    .unlocked_ioctl =    scull_ioctl,
#endif
    .open =     scull_open,
    .release =  scull_release,
};


void scull_cleanup_module(void)
{
    printk("DEBUG Entered clean");
    int i;
    dev_t devno = MKDEV(scull_major, scull_minor);

    if (scull_devices) {
        for (i = 0; i < scull_nr_devs; i++) {
            scull_trim(scull_devices + i);

            cdev_del(&scull_devices[i].cdev);
        }
    kfree(scull_devices);
    }

    unregister_chrdev_region(devno, scull_nr_devs);

    printk("DEBUG End clean");
}


int scull_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct scull_dev *dev;

    if (scull_major) {
        devno = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(devno, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&devno, scull_minor, scull_nr_devs,
                                     "scull");
        scull_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }

    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev),
                            GFP_KERNEL);
    if (!scull_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    /* Initialize each device. */
    for (i = 0; i < scull_nr_devs; i++) {
        dev = &scull_devices[i];

        dev->encrypted_text = null_vector;
        dev->key = CV_create_from_cstr("dcba", 4);
        dev->size = 0;
        dev->written = 0;
        dev->readed = 0;

        init_MUTEX(&dev->sem);

        devno = MKDEV(scull_major, scull_minor + i);
        cdev_init(&dev->cdev, &scull_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &scull_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        if (err)
            printk(KERN_NOTICE "Error %d adding scull%d", err, i);
    }

    return 0; /* succeed */

  fail:
    scull_cleanup_module();
    return result;
}

module_init(scull_init_module);
module_exit(scull_cleanup_module);
