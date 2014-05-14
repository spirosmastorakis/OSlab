#ifndef _CRYPTO_H
#define _CRYPTO_H

#include <linux/semaphore.h>

#include "cryptodev.h"

/* The Virtio ID for virtio crypto ports */
#define VIRTIO_ID_CRYPTO          13

#define VIRTIO_CRYPTO_BAD_ID      (~(u32)0)

struct virtio_crypto_config {
	__u32 unused;
} __attribute__((packed));

/*
 * A message that's passed between the Host and the Guest.
 */
struct virtio_crypto_control {
	__u16 event;            /* The kind of control event (see below) */
	short value;            /* Extra information for the key */
};

/* Some events for control messages */
#define VIRTIO_CRYPTO_DEVICE_GUEST_OPEN 0
#define VIRTIO_CRYPTO_DEVICE_HOST_OPEN  1

/*
 * This is the struct that holds the global driver data.
 */
struct crypto_driver_data {
	/* The list of the devices we are handling. */
	struct list_head devs;

	/* The minor number that we give to the next device. */
	unsigned int next_minor;
};
extern struct crypto_driver_data crdrvdata;
extern spinlock_t crdrvdata_lock;


struct crypto_vq_buffer;
/*
 * This is the struct that holds per device info.
 */
struct crypto_device {
	/* Next crypto device in the list, head is in the crdrvdata struct */
	struct list_head list;

	/* VirtQueues for Host to Guest and Guest to Host control messages. */
	struct virtqueue *c_ivq, *c_ovq;

	/* VirtQueues for Host to Guest and Guest to Host communications. */
	struct virtqueue *ivq, *ovq;

	/* The virtio device we are associated with. */
    /* 79
     80  * virtio_device - representation of a device using virtio
     81  * @index: unique position on the virtio bus
     82  * @dev: underlying device.
     83  * @id: the device type identification (used to match it with a driver).
     84  * @config: the configuration ops for this device.
     85  * @vringh_config: configuration ops for host vrings.
     86  * @vqs: the list of virtqueues for this device.
     87  * @features: the features supported by both driver and device.
     88  * @priv: private pointer for the driver's use.
     89  */
    /* 10  * virtio_config_ops - operations for configuring a virtio device
        11  * @get: read the value of a configuration field
        12  *      vdev: the virtio_device
        13  *      offset: the offset of the configuration field
        14  *      buf: the buffer to write the field value into.
        15  *      len: the length of the buffer
        16  * @set: write the value of a configuration field
        17  *      vdev: the virtio_device
        18  *      offset: the offset of the configuration field
        19  *      buf: the buffer to read the field value from.
        20  *      len: the length of the buffer
        21  * @get_status: read the status byte
        22  *      vdev: the virtio_device
        23  *      Returns the status byte
        24  * @set_status: write the status byte
        25  *      vdev: the virtio_device
        26  *      status: the new status byte
        27  * @reset: reset the device
        28  *      vdev: the virtio device
        29  *      After this, status and feature negotiation must be done again
        30  *      Device must not be reset from its vq/config callbacks, or in
        31  *      parallel with being added/removed.
        32  * @find_vqs: find virtqueues and instantiate them.
        33  *      vdev: the virtio_device
        34  *      nvqs: the number of virtqueues to find
        35  *      vqs: on success, includes new virtqueues
        36  *      callbacks: array of callbacks, for each virtqueue
            37  *              include a NULL entry for vqs that do not need a callback
                38  *      names: array of virtqueue names (mainly for debugging)
                39  *              include a NULL entry for vqs unused by driver
                    40  *      Returns 0 on success or error status
                    41  * @del_vqs: free virtqueues found by find_vqs().
                    42  * @get_features: get the array of feature bits for this device.
                        43  *      vdev: the virtio_device
                        44  *      Returns the first 32 feature bits (all we currently need).
                        45  * @finalize_features: confirm what device features we'll be using.
                        46  *      vdev: the virtio_device
                        47  *      This gives the final feature bits for the device: it can change
                            48  *      the dev->feature bits if it wants.
                                49  * @bus_name: return the bus name associated with the device
                                50  *      vdev: the virtio_device
                                51  *      This returns a pointer to the bus name a la pci_name from which
                                52  *      the caller can then copy.
                                53  * @set_vq_affinity: set the affinity for a virtqueue.
                                    54
                                    55 typedef void vq_callback_t(struct virtqueue *);
                                    56 struct virtio_config_ops {
                                        57         void (*get)(struct virtio_device *vdev, unsigned offset,
                                                               58                     void *buf, unsigned len);
                                        59         void (*set)(struct virtio_device *vdev, unsigned offset,
                                                               60                     const void *buf, unsigned len);
                                        61         u8 (*get_status)(struct virtio_device *vdev);
                                        62         void (*set_status)(struct virtio_device *vdev, u8 status);
                                        63         void (*reset)(struct virtio_device *vdev);
                                        64         int (*find_vqs)(struct virtio_device *, unsigned nvqs,
                                                                   65                         struct virtqueue *vqs[],
                                                                   66                         vq_callback_t *callbacks[],
                                                                   67                         const char *names[]);
                                        68         void (*del_vqs)(struct virtio_device *);
                                        69         u32 (*get_features)(struct virtio_device *vdev);
                                        70         void (*finalize_features)(struct virtio_device *vdev);
                                        71         const char *(*bus_name)(struct virtio_device *vdev);
                                        72         int (*set_vq_affinity)(struct virtqueue *vq, int cpu);
                                        73 };
    74
    90 struct virtio_device {
        91         int index;
        92         struct device dev;
        93         struct virtio_device_id id;
        94         const struct virtio_config_ops *config;
        95         const struct vringh_config_ops *vringh_config;
        96         struct list_head vqs;
        97         /* Note that this is a Linux set_bit-style bitmap.
        98         unsigned long features[1];
        99         void *priv;
        100 };
     */
	struct virtio_device *vdev;

	/* The minor number of the device. */
	unsigned int minor;

	/* The fd that this device has on the Host. */
	int fd;

	/* waitqueues for host control and input events. */
	wait_queue_head_t c_wq, i_wq;

	/* Indicates that the ovq is full. */
	bool ovq_full;

	/* The buffer that we last read from ivq. */
	struct crypto_vq_buffer *inbuf;

	/* FIXME: Do we need any lock? */
	/* ? */
    struct semaphore sem;
    
    spinlock_t inbuf_lock;
    
};

/* This struct represents the data that we send for the ioctl(). */
typedef struct crypto_data {
	union
	{
		struct session_op sess;
		struct crypt_op crypt;
		uint32_t sess_id;
	} op;
	uint8_t keyp[CRYPTO_CIPHER_MAX_KEY_LEN];
	uint8_t srcp[CRYPTO_DATA_MAX_LEN];
	uint8_t dstp[CRYPTO_DATA_MAX_LEN];
	uint8_t ivp[CRYPTO_BLOCK_MAX_LEN];
	unsigned int cmd;
} crypto_data;

#endif
