#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/string.h>
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
#include <linux/list.h>

#define QUEUE_MAJOR 0
#define QUEUE_NR_DEVS 1

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


typedef struct dev_message
{
	char data[500];
	struct list_head list;
}device_message;


struct queue_dev
{
	device_message *message_head;
	struct semaphore sem;
	struct cdev cdev;
	
};

struct queue_dev *queue_devices;


void queue_cleanup_module(void)
{
	dev_t devno = MKDEV(queue_major, queue_minor);

    if (queue_devices) 
    {
        for (i = 0; i < queue_nr_devs; i++) 
        {
            //queue_trim(queue_devices + i);
            cdev_del(&queue_devices[i].cdev);
        }
    kfree(queue_devices);
    }

    unregister_chrdev_region(devno, queue_nr_devs);
	printk(KERN_INFO "queue: Module Finished\n");
}

int queue_release(struct inode *inode, struct file *filp)
{
    struct queue_dev *dev = filp->private_data;
	up(&dev->sem);
    return 0;
}

int queue_open(struct inode *inode, struct file *filp)
{
	struct queue_dev *dev;
    dev = container_of(inode->i_cdev, struct queue_dev, cdev);
    filp->private_data = dev;
    if (down_interruptible(&dev->sem) != 0)
    {
		printk(KERN_ALERT "queue: Semaphore starting error\n");
		return -1;
	}
    printk(KERN_INFO "queue: Open Function Done");
	return 0;
}

ssize_t queue_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos)
{
	struct list_head *node;
	device_message *dev_msg;
	struct queue_dev *dev = filp->private_data;
	ssize_t msg_size;
	ssize_t read_length;
	char sent_data[1500];
	int messages_counter = 0;

	strcpy(sent_data,dev->message_head->data);
	messages_counter += strlen(dev->message_head->data);
	list_for_each(node,&(dev->message_head->list))
	{
		dev_msg = list_entry(node,device_message,list);
		messages_counter += strlen(dev->message_head->data);
		strcat(sent_data,dev_msg->data);
		printk(KERN_INFO "iterated values: %s\n",dev_msg->data);
	}
	printk(KERN_INFO "Messages Counter: %d\n",messages_counter);
	strcat(sent_data,"\n\0");
	
	printk(KERN_INFO "Reading f_pos: %u and count: %u\n",*f_pos,count);
	msg_size = strlen(sent_data)+1; // +1 for null character
	if(*f_pos >= msg_size)
	{
		printk(KERN_INFO "f_pos is greater than count\n");
		return 0;
	}
	
	read_length = min(msg_size - *f_pos, count);
	printk(KERN_INFO "read_length is: %d\n",read_length);

	if (read_length <= 0)
	{
		printk(KERN_INFO "read_length is equal or lower than 0:\n");
		return 0;
	}
	if(copy_to_user(buf,sent_data,sizeof(sent_data)))
	{
		printk(KERN_ALERT "All data are not copied\n"); // Buralari duzelt semaphıre filan kaldir
		return 0;
	}
	
	*f_pos += read_length;
	
	return read_length; 
}

ssize_t queue_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos)
{
	int ret;
	//device_message *dev_msg = kmalloc(sizeof(device_message),GFP_KERNEL);
	
	struct queue_dev *dev = filp->private_data;
	//strcat(buf, "\0");
	ssize_t x = strlen(buf);
	ssize_t len = min(x - *f_pos, count);
	printk(KERN_INFO "Length in writing: %u\n",len);
	if (len <= 0)
		return 0;
	printk(KERN_INFO "queue: Writing to device: %s\n",buf);
	//strcpy(dev_msg->data,buf);
	

	
	ret = copy_from_user(dev->message_head->data,"buf",len);
	strcpy(dev->message_head->data,"slm\n");
	
	*f_pos += len;
	return len;
}

long queue_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

struct file_operations queue_fops = {
    .owner =    THIS_MODULE,
    .open =     queue_open,
    .unlocked_ioctl =  queue_ioctl,
    .release =  queue_release,
    .read =     queue_read,
    .write =    queue_write,
};

int queue_init_module(void)
{
	dev_t devno;
	struct queue_dev *dev;
	device_message *newMsg;
	printk(KERN_INFO "queue: Module Started\n");
	
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
		printk("queue: Unsufficent memory area\n");
        result = -ENOMEM;
        goto fail;
    }
    printk("queue: Malloc Success\n");
    memset(queue_devices, 0, queue_nr_devs * sizeof(struct queue_dev));
	
	for(i = 0; i < queue_nr_devs; i++)
	{
		dev = &queue_devices[i];
		
		//strcpy(dev->data,"Latif-Mert");
		
		sema_init(&dev->sem,1);
		devno = MKDEV(queue_major,queue_minor+i);
		cdev_init(&dev->cdev,&queue_fops);
		dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &queue_fops;
        err = cdev_add(&dev->cdev, devno, 1);

		dev->message_head = kmalloc(sizeof(device_message),GFP_KERNEL);
		strcpy(dev->message_head->data,"X");
		INIT_LIST_HEAD(&(dev->message_head->list));
		
		newMsg = kmalloc(sizeof(device_message),GFP_KERNEL);
		strcpy(newMsg->data,"Y");
		list_add_tail(&newMsg->list,&dev->message_head->list);

		newMsg = kmalloc(sizeof(device_message),GFP_KERNEL);
		strcpy(newMsg->data,"Z");
		list_add_tail(&newMsg->list,&dev->message_head->list);

		newMsg = kmalloc(sizeof(device_message),GFP_KERNEL);
		strcpy(newMsg->data,"T");
		list_add_tail(&newMsg->list,&dev->message_head->list);
		









        
        if (err)
            printk(KERN_NOTICE "queue: Error %d adding queue%d", err, i);	
	}
	
	printk(KERN_INFO "queue: Major number: %d \n",queue_major);
	return 0;
	
fail:
		queue_cleanup_module();
		return result;
	
}


module_init(queue_init_module);
module_exit(queue_cleanup_module);

