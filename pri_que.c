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
#include <asm/switch_to.h>	/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */


#define QUEUE_MAJOR 0
#define QUEUE_NR_DEVS 2

int queue_major = QUEUE_MAJOR;
int queue_minor = 0;
int queue_nr_devs = QUEUE_NR_DEVS;

module_param(queue_major, int, S_IRUGO);
module_param(queue_minor, int, S_IRUGO);
module_param(queue_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Mert Koprucu, Latif Uluman and Fatih Kuru");
MODULE_LICENSE("Dual BSD/GPL");

int result;
int err;
int i;

struct queue_dev{
	char data[100];
	struct semaphore sem;
	struct cdev cdev;
	
};

struct queue_dev *queue_devices;

void queue_cleanup_module(void);

struct file_operations queue_fops = {
    .owner =    THIS_MODULE
    /*
    .llseek =   scull_llseek,
    .read =     scull_read,
    .write =    scull_write,
    .unlocked_ioctl =  scull_ioctl,
    .open =     scull_open,
    .release =  scull_release,
    */
};


int queue_init_module(void)
{
	dev_t devno;
	struct queue_dev *dev;
	printk(KERN_INFO "Hello Everyone, I am queue and My Device Couunt: %d \n",queue_nr_devs);	
	
	if(queue_major)
	{
		devno = MKDEV(queue_major,queue_minor);
		result = register_chrdev_region(devno, queue_nr_devs, "queue");	
	}
	else
	{
		result = alloc_chrdev_region(&devno, queue_minor, queue_nr_devs,"queue");
        queue_major = MAJOR(devno);
	}
	if (result < 0) 
	{
        printk(KERN_WARNING "queue: can't get major %d\n", queue_major);
        return result;
    }
    
    queue_devices = kmalloc(queue_nr_devs*sizeof(struct queue_dev),GFP_KERNEL);
	
	if (!queue_devices) {
        result = -ENOMEM;
        goto fail;
    }
    
    memset(queue_devices, 0, queue_nr_devs * sizeof(struct queue_dev));
	
	for(i = 0; i < queue_nr_devs; i++)
	{
		dev = &queue_devices[i];
		//dev->data = "";
		sema_init(&dev->sem,1);
		devno = MKDEV(queue_major,queue_minor+i);
		cdev_init(&dev->cdev,&queue_fops);
		
		dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &queue_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        
        if (err)
            printk(KERN_NOTICE "queue: Error %d adding queue%d", err, i);	
	}
	printk(KERN_INFO "queue: Major number: %d \n",queue_major);
	return 0;
fail:
		queue_cleanup_module();
		return result;
}

int queue_trim(struct queue_dev *dev)
{
	return 0;
}

void queue_cleanup_module(void)
{
    dev_t devno = MKDEV(queue_major, queue_minor);

    if (queue_devices) 
    {
        for (i = 0; i < queue_nr_devs; i++) 
        {
            queue_trim(queue_devices + i);
            cdev_del(&queue_devices[i].cdev);
        }
    kfree(queue_devices);
    }

    unregister_chrdev_region(devno, queue_nr_devs);
	printk(KERN_INFO "queue: Goodbye\n");
}

module_init(queue_init_module);
module_exit(queue_cleanup_module);
