/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * Spyridon (Spiros) Mastorakis
 * George Matikas
 *
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

/*
 * Just a quick [unlocked] check to see if the cached
 * chrdev state needs to be updated from sensor measurements.
 */
static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{
    uint32_t state_time, msr_time;
	struct lunix_sensor_struct *sensor;
    
    debug("[Refresh] going into lunix_chrdev_state_needs_refresh\n");
	
	WARN_ON ( !(sensor = state->sensor));
	/* ? */
    
    state_time = state->buf_timestamp;
    msr_time = (sensor->msr_data[state->type])->last_update;
    
    if (msr_time>state_time) {
        debug("[Refresh] leaving with ret: 1 \n");
        return 1;
    }
    
	/* The following return is bogus, just for the stub to compile */
    debug("[Refresh] leaving with ret: 0 \n");
	return 0; /* ? */
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
    // unsigned long flags;
    uint32_t raw;
    long value;
    
    debug("[Update] going into lunix_chrdev_state_update\n");
    
    if (!lunix_chrdev_state_needs_refresh(state))
        return -EAGAIN;
    
	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */
	/* ? */
	/* Why use spinlocks? See LDD3, p. 119 */

	/*
	 * Any new data available?
	 */
	/* ? */

    WARN_ON (!(sensor = state->sensor));
    
    spin_lock_irqsave(&(sensor->lock));
    //spin_lock_irqsave disables interrupts (on the local processor only) before
    //taking the spinlock; the previous interrupt state is stored in flags.
    
    raw = sensor->msr_data[state->type]->values[0];
    
    spin_unlock_irqsave(&(sensor->lock));
    
    debug("[Update] raw data: %d\n",raw);
    
	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore
	 */

	switch (state->type) {
        case 0:
            value = lookup_voltage[raw];
            break;
        case 1:
            value = lookup_temperature[raw];
            break;
        case 2:
            value = lookup_light[raw];
            break;
        default:
            break;
    }
    
    debug("[Update] formatted value: %ld\n",value);
    
    state->buf_lim = sprintf(state->buf_data,"%ld.%.3ld\n",value/1000,value%1000);
    
    debug("[Update] buf length: %d\n",state->buf_lim);
    
    state->buf_timestamp = get_seconds();

	debug("[Update] leaving\n");
	return 0;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	/* Declarations */
	/* ? */
    struct lunix_chrdev_state_struct *temp;
	int ret;

	debug("[Open] entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)//this is used by subsystems
        //that don't want seekable file descriptors.
		goto out;

	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */
	
	/* Allocate a new Lunix character device private state structure */
	/* ? */
    
    temp = kzalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL);
    if (!temp) {
		printk(KERN_ERR "[Open] Failed to allocate memory for Lunix chrdev state struct\n");
		goto out;
	}
    
    temp->type = iminor(inode)%8;
    
    temp->sensor = &(lunix_sensors[iminor(inode)/8]);
    
    temp->buf_timestamp = 0;
    
    sema_init(&(temp->lock),1);
    
    filp->private_data=temp;
    
    debug("[Open] completed successfully\n");
	return 0;
    
out:
	debug("[Open] leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{   /**
     3786  * kfree - free previously allocated memory
     3787  * @objp: pointer returned by kmalloc.
     3788  *
     3789  * If @objp is NULL, no operation is performed.
     3790  *
     3791  * Don't free memory not originally allocated by kmalloc()
     3792  * or you will run into trouble.
     3793  */
	kfree(filp->private_data);
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	/* Why? */
	return -EINVAL;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{
	ssize_t ret;
    
    int left;
    
	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;
    
	state = filp->private_data;
	WARN_ON(!state);
    
	sensor = state->sensor;
	WARN_ON(!sensor);
    
	/* Lock? */
    debug("[Read] entering read\n");
    if (down_interruptible(&state->lock)) {
        debug("[Read] Could not acquire lock\n");
        return -ERESTARTSYS;
    }
    
	/*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e. we need to report
	 * on a "fresh" measurement, do so
	 */
	if (*f_pos == 0) {
		while (lunix_chrdev_state_update(state) == -EAGAIN) {
			/* ? */
			/* The process needs to sleep */
			/* See LDD3, page 153 for a hint */
            debug("[Read] going to sleep\n");
            up(&(state->lock)); /* release the lock */
            if (wait_event_interruptible(sensor->wq, lunix_chrdev_state_needs_refresh(state)))
                //wait_event_interruptible - sleep until a condition gets true
                //@wq: the waitqueue to wait on
                //@condition: a C expression for the event to wait for
                return -ERESTARTSYS;
            debug("[Read] woke up from sleep\n");
            if (down_interruptible(&state->lock)) {
                //down_interruptible - acquire the semaphore unless interrupted
                //@sem: the semaphore to be acquired
                //Attempts to acquire the semaphore. If no more tasks are allowed to
                //acquire the semaphore, calling this function will put the task to
                //sleep.If the sleep is interrupted by a signal, this function will
                //return -EINTR.If the semaphore is successfully acquired, this
                //function returns 0.
                debug("[Read] Could not acquire lock\n"); /* acquire again the lock */
                return -ERESTARTSYS;
            }
        }
	}
    /* End of file */
	/* ? */
    
    left = state->buf_lim - *f_pos;
    debug("[Read] left: %d bytes to read\n",left);
	
	/* Determine the number of cached bytes to copy to userspace */
	/* ? */
    
    if (cnt < left){
        if (copy_to_user(usrbuf, state->buf_data + *f_pos, cnt)) {
            ret = -EFAULT;/* Bad address */
            goto out;
        }
        debug("[Read] cnt < left\n");
        *f_pos += cnt;
        ret = cnt;
    }
    else {
        if (copy_to_user(usrbuf, state->buf_data + *f_pos, left)) {
            ret = -EFAULT;
            goto out;
        }
        debug("[Read] left < cnt\n");
        *f_pos = 0;
        ret = left;
    }
    
	/* Auto-rewind on EOF mode? */
	/* ? */
    
    debug("[Read] read: %d bytes\n",(int) ret);
    
out:
	/* Unlock? */
    up(&(state->lock));
	return ret;

}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops =
{
        .owner          = THIS_MODULE,
	.open           = lunix_chrdev_open,
	.release        = lunix_chrdev_release,
	.read           = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap           = lunix_chrdev_mmap
};

int lunix_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
	
	debug("[Init] initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);//initialiazes
    //lunix_chrdev_cdev , remembering lunix_chrdev_fops, making it
    //ready to add to the system with cdev_add
    //void cdev_init(struct cdev *cdev, const struct file_operations *fops)
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);//combines major number with minor number
    //to convert them into one device. Here, we begin with LINUX_CHRDEV_MAJOR:0
    //MKDEV(int major, int minor);
	/* ? */
	/* register_chrdev_region? */
    //register_chrdev_region(dev_t from, unsigned count, const char *name)
    //register a range of device numbers
    //@from: the first in the desired range of device numbers; must include
    //the major number.@count: the number of consecutive device numbers required
    //@name: the name of the device or driver.
    //Return value is zero on success, a negative error code on failure.
    ret=register_chrdev_region(dev_no,lunix_minor_cnt,"Lunix:TNG");
	if (ret < 0) {
		debug("[Init] failed to register region, ret = %d\n", ret);
		goto out;
	}	
	/* ? */
	/* cdev_add? */
    ret=cdev_add(&lunix_chrdev_cdev,dev_no,lunix_minor_cnt);//adds the device
    //represented by lunix_chrdev_cdev to the system, making it live immediately.
    //A negative error code is returned on failure
    //int cdev_add(struct cdev *p, dev_t dev, unsigned count)
	if (ret < 0) {
		debug("[Init] failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("[Init] completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
		
	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);//removes lunix_chrdev_cdev from the system
    //,possibly freeing the structure itself
    //void cdev_del(struct cdev *p)
	unregister_chrdev_region(dev_no, lunix_minor_cnt); //This function will
    //unregister a range of @count device numbers
    //starting with @from.  The caller should normally be the one who
    //allocated those numbers in the first place...
    //unregister_chrdev_region() - return a range of device numbers
    //@from: the first in the range of numbers to unregister
    //@count: the number of device numbers to unregister
	debug("leaving\n");
}
