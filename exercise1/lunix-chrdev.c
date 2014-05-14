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
#include <linux/fcntl.h>

#include <string.h>
#include <asm/byteorder.h>

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
    
    debug("going into lunix_chrdev_state_needs_refresh\n");
	
	WARN_ON ( !(sensor = state->sensor));
	/* ? */
    
    state_time = state->buf_timestamp;
    msr_time = (sensor->msr_data[state->type])->last_update;
    
    if (msr_time>state_time) {
        debug("leaving with ret: 1 \n");
        return 1;
    }
    
	/* The following return is bogus, just for the stub to compile */
    debug("leaving with ret: 0 \n");
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
    unsigned long flags;
    int new_data;
    uint16_t portion; /* fract_portion; */
    long res ; /* res_fract */
    uint32_t temp, temp1 ;
    unsigned char temp_buf[20];
	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */
	/* ? */
	/* Why use spinlocks? See LDD3, p. 119 */
    
    debug("going into lunix_chrdev_state_update \n ");
    spin_lock_irqsave(&sensor->lock, flags );
    
	/*
	 * Any new data available?
	 */
	/* ? */
    
    new_data = lunix_chrdev_state_needs_refresh(&state);
    spin_unlock_irqrestore(&sensor->lock, flags);
    
	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore
	 */
    /* ? */
    
    if (down_interruptible(&state->lock)) {
        debug("Could not acquire lock\n");
        return -ERESTARTSYS;
    }
    if ((new_data) == 1) {
        debug("there is new data")
        memcpy(&temp1,((sensor->msr_data[state->type])->values[0]), 4);
        temp = cpu_to_le32(temp1);
        memcpy(&portion,&temp,2);
       /* temp = temp >> 16; */
       /* memcpy(&integ_portion,&temp,2); */
        if ((state->type) == 0 ) {
            res = uint16_to_batt(portion);
            /* res_fract = uint16_to_batt(fract_portion); */
        }
        if ((state->type) == 1 ){
            res = uint16_to_temp(portion);
            /* res_fract = uint16_to_temp(fract_portion); */
        }
        if ((state->type) == 2 ){
            res = uint16_to_light(portion);
            /* res_fract = uint16_to_light(fract_portion); */
        }
        up(&state->lock);
        sprintf(temp_buf, "%ld" , res);
        (state->buf_lim) = strlen(temp_buf);
        memcpy((state->buf_data), &temp_buf, (state->buf_lim));
        return 0;
        
    }
    
    else {
        up(&state->lock);
        return -EAGAIN;
    }
	debug("leaving\n");
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
    
	debug("entering open\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;
    
	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */
	
	/* Allocate a new Lunix character device private state structure */
	/* ? */
    
    temp = kzalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL);
    if (!temp) {
		printk(KERN_ERR "Failed to allocate memory for Lunix chrdev state struct\n");
		goto out;
	}
    
    temp->type = iminor(inode)%8;
    
    /* temp->sensor = kzalloc(sizeof(struct lunix_sensor_struct), GFP_KERNEL);
    if (!(temp->sensor)) {
		printk(KERN_ERR "Failed to allocate memory for Lunix sensor struct\n");
		goto out_with_lunix_chrdev_state_struct; 
	}
     */
    sema_init(&(temp->lock),1);
    
    filp->private_data=temp;
    
    debug("completed successfully\n");
	return 0;
    
out_with_lunix_chrdev_state_struct:
    debug("at out_with_lunix_chrdev_state_struct\n");
    kfree(temp);
out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
    debug("entering release");
   /* kzfree(filp->private_data->lunix_chrdev_state_struct->lunix_sensor_struct);
    printk(KERN_INFO "Lunix:TNG lunix_sensor_struct unloaded successfully\n"); */
    kzfree(filp->private_data->lunix_chrdev_state_struct);
    printk(KERN_INFO "Lunix:TNG lunix_chrdev_state_struct unloaded successfully\n");
    
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
    
	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;
    
	state = filp->private_data;
	WARN_ON(!state);
    
	sensor = state->sensor;
	WARN_ON(!sensor);
    
	/* Lock? */
    debug("entering read /n");
    if (down_interruptible(&state->lock)) {
        debug("Could not acquire lock\n");
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
            debug("going to sleep");
            up(&state->lock); /* release the lock */
            if (wait_event_interruptible(sensor->wq, (lunix_chrdev_state_update(state) != -EAGAIN)))
                return -ERESTARTSYS;
            debug("wake up from sleep");
            if (down_interruptible(&state->lock)) {
                debug("Could not acquire lock\n"); /* acquire again the lock */
                return -ERESTARTSYS;
            }
        }
	}
    /* End of file */
	/* ? */
	
	/* Determine the number of cached bytes to copy to userspace */
	/* ? */
    while (((*f_pos) < cnt ) and ((*f_pos) < (state->buf_lim))) {
        if (cnt < buf_lim){
            if (copy_to_user(usrbuf, state->buf_data, cnt)){
                return -EFAULT;
            *f_pos += cnt;
            }
        }
        else {
            if (copy_to_user(usrbuf, state->buf_data, state->buf_lim))
                return -EFAULT;
            *f_pos += state->buf_lim ;
        }
    }
    ret=*f_pos;
    
	/* Auto-rewind on EOF mode? */
	/* ? */
    
    if (*f_pos == state->buf_lim){
        *f_pos=0;
    }
        
out:
	/* Unlock? */
    up(&state->lock);
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
	
	debug("initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	/* ? */
	/* register_chrdev_region? */
    ret=register_chrdev_region(dev_no,lunix_minor_cnt,"Lunix:TNG");
	if (ret < 0) {
		debug("failed to register region, ret = %d\n", ret);
		goto out;
	}
	/* ? */
	/* cdev_add? */
    ret=cdev_add(&lunix_chrdev_cdev,dev_no,lunix_minor_cnt);
	if (ret < 0) {
		debug("failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
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
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}

