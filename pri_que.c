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


#define PRI_QUE_MAJOR 0
#define PRI_QUE_NR_DEVS 2

int pri_que_major = PRI_QUE_MAJOR;
int pri_que_minor = 0;
int pri_que_nr_devs = PRI_QUE_NR_DEVS;

module_param(pri_que_major, int, S_IRUGO);
module_param(pri_que_minor, int, S_IRUGO);
module_param(pri_que_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Mert Koprucu, Latif Uluman and Fatih Kuru");
MODULE_LICENSE("Dual BSD/GPL");

int pri_que_init_module(void)
{
	printk(KERN_INFO "Hello Everyone\n");
}

void pri_que_cleanup_module(void)
{
	printk(KERN_INFO "Goodbye\n");
}

module_init(pri_que_init_module);
module_exit(pri_que_cleanup_module);
