/*
 * virtio-crypto.c
 *
 * virtio-crypto (QEMU)
 *
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 * Spyridon (Spiros) Mastorakis
 * George Matikas
 *
 */

#include "iov.h"
#include "virtio.h"
#include "virtio-crypto.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

/*
 * This is the struct that represents our device in the QEMU world.
 * vdev field must always be first for casting between different types.
*/
struct VirtIOCrypto {
	VirtIODevice vdev;

	/* Control VirtQueues. */
	VirtQueue *c_ivq, *c_ovq;

	/* Data VirtQueues. */
	VirtQueue *ivq, *ovq;

	/* The configuration struct of virtio crypto device. */
	struct virtio_crypto_config config;

	/* This is the file descriptor for /dev/crypto file on the Host. */
	int fd;
};

/*
 * Called when guest negotiates the virtio device features. 
 * We don't need any feature for virtio-crypto yet so just return 
 * the given features.
 */
static uint32_t get_features(VirtIODevice *vdev, uint32_t features)
{
	FUNC_IN;
	FUNC_OUT;
	
	return features;
}

/*
 * Called when guest requests virtio device config info. 
 */
static void get_config(VirtIODevice *vdev, uint8_t *config_data)
{
	VirtIOCrypto *crdev;
	
	FUNC_IN;
	crdev = DO_UPCAST(VirtIOCrypto, vdev, vdev);
	memcpy(config_data, &crdev->config, sizeof(struct virtio_crypto_config));
	FUNC_OUT;
}

/*
 * Called when host wants to change the virtio device config.
 * We dont need that feature so we just dummy copy config_data.
 */
static void set_config(VirtIODevice *vdev, const uint8_t *config_data)
{
	struct virtio_crypto_config config;
	
	FUNC_IN;
	memcpy(&config, config_data, sizeof(config));
	FUNC_OUT;
}

/*
 * Handler for control c_ivq virtqueue.
 * Called when host sends a control message to the guest.
 */
static void control_in(VirtIODevice *vdev, VirtQueue *vq)
{
	FUNC_IN;
	FUNC_OUT;
}

static size_t send_buffer(VirtIOCrypto *crdev, const uint8_t *buf, size_t size)
{
	VirtQueueElement elem;
	VirtQueue *vq;
	size_t len;
	
	vq = crdev->ivq;

	/* Is the virtqueue ready? */
	if (!virtio_queue_ready(vq)) {
		return 0;
	}

	/* Is there a buffer in the queue? */
	/* If not we can not send data to the Guest. */	
	/* ? */
    
    if (!virtqueue_pop(vq, &elem)) {
		return 0;
	}
	/* apo buffer se scattergatherlist */
	len = iov_from_buf(elem.in_sg, elem.in_num, 0,
			   buf, size);

	/* Push the buffer to the virtqueue and notify guest */
	/* ? */
    
    virtqueue_push(vq, &elem, len);
	virtio_notify(&crdev->vdev, vq);
	FUNC_OUT;
	return len;
}

/*
 * Send a control event to the guest.
 */
static size_t send_control_event(VirtIOCrypto *crdev, uint16_t event, 
                                 uint16_t value)
{
	struct virtio_crypto_control cpkt;
	VirtQueueElement elem;
	VirtQueue *vq;

	FUNC_IN;
    /* logika apothikeyei event kai value se pedia tou struct virtio_crypto_control */
	stw_p(&cpkt.event, event);
	stw_p(&cpkt.value, value);

	vq = crdev->c_ivq;
	if (!virtio_queue_ready(vq)) {
		return 0;
	}

	if (!virtqueue_pop(vq, &elem)) {
		return 0;
	}
	
	memcpy(elem.in_sg[0].iov_base, &cpkt, sizeof(cpkt));
	
	virtqueue_push(vq, &elem, sizeof(cpkt));
	virtio_notify(&crdev->vdev, vq);
	FUNC_OUT;
	return sizeof(cpkt);
}

/*
 * This is the function that is called whenever the guest sends us 
 * control events.
 */
static void handle_control_message(VirtIOCrypto *crdev, void *buf, size_t len)
{
	struct virtio_crypto_control cpkt, *gcpkt;

	FUNC_IN;
	gcpkt = buf;
    int fd;
    size_t cnt;

	if (len < sizeof(cpkt)) {
		/* The guest sent an invalid control packet */
		return;
	}
    /*logika fortwnei apo gcpkt ta pedia event kai value kai ta apothikeyei
     sta pedia event kai value tou cpkt */
	cpkt.event = lduw_p(&gcpkt->event);
	cpkt.value = lduw_p(&gcpkt->value);

	if (cpkt.event == VIRTIO_CRYPTO_DEVICE_GUEST_OPEN) {
		/* cpkt.value = 1 for file open and 0 for file close. */
		if (cpkt.value) {
			printf("in open file\n");

			/* Open crypto device file and send the appropriate
 			 * message (event) to the guest */
			/* ? */
            fd = open("/dev/crypto",O_RDWR);
            
            crdev->fd=fd;
            
            cnt = send_control_event(crdev, VIRTIO_CRYPTO_DEVICE_HOST_OPEN, fd);
            
            if (!cnt) return;
            
		} 
		else {
			printf("in close file\n");

			/* Close the previously opened file */
			/* ? */
            
            close(crdev->fd);
            
		}
	}
	FUNC_OUT;
}

/*
 * Handler for control c_ovq virtqueue.
 * Called when guest sends a control message to the host.
 */
static void control_out(VirtIODevice *vdev, VirtQueue *vq)
{
	VirtQueueElement elem;
	VirtIOCrypto *crdev;
	uint8_t *buf;
	size_t len;

	FUNC_IN;	
	crdev = DO_UPCAST(VirtIOCrypto, vdev, vdev);
	
	len = 0;
	buf = NULL;

	if (!virtqueue_pop(vq, &elem))
		return;
    /* size of scattergatherlist */
	len = iov_size(elem.out_sg, elem.out_num);
	buf = g_malloc(len);
	/* metatroph apo scattergatherlist se buffer */
	iov_to_buf(elem.out_sg, elem.out_num, 0, buf, len);
	handle_control_message(crdev, buf, len);

	virtqueue_push(vq, &elem, 0);
	virtio_notify(vdev, vq);

	g_free(buf);
	FUNC_OUT;
}

/*
 * Handler for ivq virtqueue.
 * Called when host sends data to the guest.
 * No need to worry about it.
 */
static void handle_input(VirtIODevice *vdev, VirtQueue *vq)
{
	FUNC_IN;
	FUNC_OUT;
}

/*
 * Performs ioctl to the actual device
 */
static ssize_t crypto_handle_ioctl_packet(VirtIOCrypto *crdev, 
                                          const uint8_t *buf, 
                                          ssize_t len) 
{
	ssize_t ret = 0;
	struct crypt_op crypt;
	virtio_crypto_buffer *cr_data = (virtio_crypto_buffer *) buf;
	//uint32_t sess_id;

	FUNC_IN;
	switch (cr_data->cmd) {

	/* set the metadata for every operation and perform the ioctl to 
 	 * the actual device */
	case CIOCGSESSION:
		/* ? */
            
            cr_data->op.sess.key = cr_data->keyp;
            
            if (ioctl(crdev->fd, CIOCGSESSION, &cr_data->op.sess)) {
                perror("ioctl(CIOCGSESSION)");
                return -1;
            }
            
		break;

	case CIOCCRYPT:
		/* ? */
            
            crypt.ses = cr_data->op.crypt.ses;
            crypt.iv = cr_data->ivp;
            crypt.src = cr_data->srcp;
            crypt.dst = cr_data->dstp;
            crypt.len = cr_data->op.crypt.len;
            crypt.op = cr_data->op.crypt.op;
            
            if (ioctl(crdev->fd, CIOCCRYPT, &crypt)) {
                perror("ioctl(CIOCCRYPT)");
                return -1;
            }
            
		break;

	case CIOCFSESSION:
		/* ? */
            
            if (ioctl(crdev->fd, CIOCFSESSION, &cr_data->op.sess_id)) {
                perror("ioctl(CIOCFSESSION)");
                return -1;
            }
            
		break;

	default:
		return -EINVAL;
	}

	/* Ok now we can send the reply to the guest */
	/* ? */
    
    ret = send_buffer(crdev,(const uint8_t *)cr_data,sizeof(cr_data));

	FUNC_OUT;
	return ret;
}

/*
 * Handler for ovq virtqueue.
 * Called when guest sends data to the host.
 */
static void handle_output(VirtIODevice *vdev, VirtQueue *vq)
{
    /* Macro poy kanei castarisma metaksy tou VirtIOCrypto struct kai ths domhs 
     tou Linux gia devices ,ths virtio_device */  
	VirtIOCrypto *crdev = DO_UPCAST(VirtIOCrypto, vdev, vdev);
	VirtQueueElement elem;
	uint8_t *buf;
	size_t buf_size;
	//ssize_t ret;

	FUNC_IN;
	
	/* Dummy return. Delete once the driver is ready. */

	/* pop buffer from virtqueue and check return value */
	/* ? */
    
    if (!virtqueue_pop(vq, &elem))
		return;

	/* FIXME: Are we sure all data is in one sg list?? */
    
    buf_size = iov_size(elem.out_sg, elem.out_num);
	buf = g_malloc(buf_size);
	
	iov_to_buf(elem.out_sg, elem.out_num, 0, buf, len);
	crypto_handle_ioctl_packet(crdev, buf, buf_size);

	/* Put buffer back and notify guest. */
	/* ? */
    
    virtqueue_push(vq, &elem, 0);
	virtio_notify(vdev, vq);

	FUNC_OUT;
}

/*
 * Called from virtio_crypto_init_pci() before the initialization of
 * the pci device.
 *
 * We need to create the virtio device, add the needed virtqueues and
 * set all the handlers we need.
 */
VirtIODevice *virtio_crypto_init(DeviceState *dev)
{
	VirtIOCrypto *crdev;
	VirtIODevice *vdev;

	FUNC_IN;
	vdev = virtio_common_init("virtio-crypto", VIRTIO_ID_CRYPTO,
	                          sizeof(struct virtio_crypto_config),
	                          sizeof(VirtIOCrypto));

	crdev = DO_UPCAST(VirtIOCrypto, vdev, vdev);

	/* Add the virtqueues we need for the device. */
	crdev->c_ivq = virtio_add_queue(vdev, 32, control_in);
	crdev->c_ovq = virtio_add_queue(vdev, 32, control_out);
	crdev->ivq = virtio_add_queue(vdev, 128, handle_input);
	crdev->ovq = virtio_add_queue(vdev, 128, handle_output);

	/* An initial value to indicate not opened file. */
	crdev->fd = -13;

	/* Set the callback functions for the virtio device we made. */
	crdev->vdev.get_features = get_features;
	crdev->vdev.get_config = get_config;
	crdev->vdev.set_config = set_config;

	FUNC_OUT;
	return vdev;
}

/*
 * Called from virtio_crypto_exit_pci() 
 */
void virtio_crypto_exit(VirtIODevice *vdev)
{
	FUNC_IN;
	FUNC_OUT;
}
