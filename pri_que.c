#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/kernel.h>  /* printk() */
#include <linux/slab.h>    /* kmalloc() */
#include <linux/fs.h>    /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>  /* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <asm/switch_to.h>  /* cli(), *_flags */
#include <asm/uaccess.h>  /* copy_*_user */
#include <linux/list.h>

#include "queue_ioctl.h"

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
  char *data;
  int *message_count;
  struct list_head list;
}device_message;


typedef struct queue_dev
{
  device_message *message_head;
  struct semaphore sem;
  struct cdev cdev;
}queue_device;

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
  if(MINOR(dev->cdev.dev) == 0)
    return -EINVAL;


  ssize_t msg_size;
  ssize_t read_length;
  char *sent_data;
  size_t prev_msg_count = 0;
  size_t messages_counter = 0;

  if(dev->message_head == NULL)
  {
    printk(KERN_ALERT "dev message head is null\n");
    copy_to_user(buf,"\0",sizeof("\0"));
    return 0;
  }
  printk(KERN_INFO "Before message counter\n");
  messages_counter += (*(dev->message_head->message_count))-1;
  prev_msg_count = messages_counter;
  printk("MESSAGES COUNTER for head: %d\n",messages_counter);

  sent_data = (char *)kmalloc(messages_counter +1 * sizeof(char),GFP_KERNEL);
  memset(sent_data,0,sizeof(char) * messages_counter);
  sent_data[0] = '\0';
  strncat(sent_data,dev->message_head->data,messages_counter);
  sent_data[prev_msg_count] = '\0';

  list_for_each(node,&(dev->message_head->list))
  {
    printk(KERN_INFO "List for each started \n");
    dev_msg = list_entry(node,device_message,list);  
    messages_counter += (*(dev_msg->message_count))-1;
    sent_data = (char *)krealloc(sent_data,(prev_msg_count + (*(dev_msg->message_count))) * sizeof(char),GFP_KERNEL);
    strncat(sent_data,dev_msg->data,(*(dev_msg->message_count))-1);
    sent_data[messages_counter] = '\0';
    prev_msg_count = messages_counter;
  }
  printk("MESSAGES COUNTER at end of list for each: %d\n",messages_counter);
  sent_data = (char *)krealloc(sent_data,(messages_counter + 2) * sizeof(char),GFP_KERNEL);
  *(sent_data + messages_counter) = '\0';
  strcat(sent_data,"\n");
  printk("Send data(after list for each): %s\n",sent_data);
  
  printk(KERN_INFO "Reading f_pos: %u and count: %u\n",*f_pos,count);
  msg_size = messages_counter+1;//strlen(sent_data)+1; // +1 for null character
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
  if(copy_to_user(buf,sent_data,read_length * sizeof(char)))
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
  struct queue_dev *dev = filp->private_data;
  if(MINOR(dev->cdev.dev) == 0)
    return -EINVAL;
  ssize_t buffer_size = count; //Terminator character dahil
  device_message *newMsg=NULL; 
  ssize_t len;

  if(*f_pos >= buffer_size)
  {
    printk(KERN_INFO "f_pos is greater than count in writing\n");
    return 0;
  }
  len = min(buffer_size - *f_pos, count);
  if (len <= 0)
    return 0;
  

  newMsg = (newMsg == NULL ? (device_message *) kmalloc(sizeof(device_message),GFP_KERNEL):newMsg);
  
  newMsg->data = (char *)kmalloc(count * sizeof(char) ,GFP_KERNEL);
  newMsg->message_count = (int *)kmalloc(sizeof(int) ,GFP_KERNEL);
  *(newMsg->message_count) = count; // Terminator character ignores

  printk(KERN_INFO "newmsg->counter and length buf respectively: %u and %u in writing\n", *(newMsg->message_count), strlen(buf));
  
  if(newMsg == NULL || newMsg->data == NULL || copy_from_user(newMsg->data,buf,len))
  {
    printk(KERN_ALERT "Error in copy_from_user \n");
    return -1;
  }
  else
  {
    printk(KERN_ALERT "Passed 0 \n");

    if(dev->message_head == NULL)
    {
      printk(KERN_ALERT "Passed 1\n");
      dev->message_head = (device_message *)kmalloc(sizeof(device_message),GFP_KERNEL);
      dev->message_head->data = (char *)kmalloc(count*sizeof(char),GFP_KERNEL);
      dev->message_head->message_count = (int *)kmalloc(sizeof(int),GFP_KERNEL);
      printk(KERN_ALERT "Passed 2\n");
      *(dev->message_head->message_count) = count;
      printk(KERN_ALERT "Passed 3 and message_head_count: %d \n",*(dev->message_head->message_count));
      strcpy(dev->message_head->data,newMsg->data);
      printk(KERN_INFO "Head is created\n");
      INIT_LIST_HEAD(&(dev->message_head->list));
      //printk("Wrting in head :%s \n",dev->message_head->data);
      //free işlemi var newMsg için
      
    }
    else
    {
      printk(KERN_ALERT "Passed 4\n");
      list_add_tail(&newMsg->list,&dev->message_head->list);
      printk(KERN_ALERT "Passed 5\n");

    }
  }
  printk(KERN_ALERT "All Passed\n");
  *f_pos += len;
  return len;
}
void pop_first_message(char *buf)
{
    int i;
    struct queue_dev *dev;
    for (i = 1; i < queue_nr_devs;i++)
    {
        dev = (queue_devices + i);
        printk("pop_first_message\n");
        if(dev == NULL || dev->message_head == NULL || dev->message_head->data == NULL)
            continue;
        printk("Reading data: %s \n",dev->message_head->data);
        strcpy(buf,dev->message_head->data);
        *(buf+(*(dev->message_head->message_count))) = '\0';

    }
    return NULL;
}
long queue_ioctl(struct file *filp, unsigned int cmd, char *arg)
{
    int retval = -1;
    struct queue_dev *dev = filp->private_data;
    size_t minor = MINOR(dev->cdev.dev);
    if(minor != 0)
        return -EINVAL;

    printk("Above Switch\n");
    
    switch(cmd) 
    {
        case QUEUE_POP:
            printk("POPPPPPPPPPPP Switch\n");
            pop_first_message(arg);
            return 0;
        break;
        default:
            printk("default Switch\n");
            return -EINVAL;
        break;
    }
    return retval;
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
    
    sema_init(&dev->sem,1);
    devno = MKDEV(queue_major,queue_minor+i);
    cdev_init(&dev->cdev,&queue_fops);
    dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &queue_fops;
        err = cdev_add(&dev->cdev, devno, 1);
    dev->message_head = NULL;


    /*
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
    */









        
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