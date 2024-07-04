#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/ioctl.h>
#include <linux/moduleparam.h>

#define MAX_DEVICES 10

static int major_number = 0;
static int num_devices = 4;

module_param(major_number, int, S_IRUGO);
module_param(num_devices, int, S_IRUGO);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Whitby, Thomas.Whitby@students.mq.edu.au");
MODULE_DESCRIPTION("COMP2291 Counter driver");

#define DEV_IOC_MAGIC       0x5b
#define DEV_IOC_RST         _IO(DEV_IOC_MAGIC, 0)
#define DEV_IOC_GET         _IOR(DEV_IOC_MAGIC, 1, uint8_t*)
#define DEV_IOC_STP         _IOW(DEV_IOC_MAGIC, 2, uint8_t*)

#define MAX_NUMBER 255
#define MIN_NUMBER -1

#define DEVICE_NAME "Counter"

static struct class *counter_class;

struct device_data {
    uint8_t counter_value;
    int step;
    struct cdev cdev;
};

static struct device_data counter_devices[MAX_DEVICES];

static int counter_open(struct inode *inode, struct file *file);
static int counter_release(struct inode *inode, struct file *file);
static ssize_t counter_read(struct file *file, char __user *buffer, size_t length, loff_t *offset);
static ssize_t counter_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset);
static long counter_ioctl(struct file *file, unsigned int command, unsigned long arg);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = counter_open,
    .release = counter_release,
    .read = counter_read,
    .write = counter_write,
    .unlocked_ioctl = counter_ioctl,
};

static int __init counter_init(void) {
    int ret, i;
    dev_t dev_number;

    if (major_number) {
        dev_number = MKDEV(major_number, 0);
        ret = register_chrdev_region(dev_number, num_devices, DEVICE_NAME);
    } else {
        ret = alloc_chrdev_region(&dev_number, 0, num_devices, DEVICE_NAME);
        major_number = MAJOR(dev_number);
    }

    if (ret < 0) {
        pr_notice("Error %d adding %s\n", ret, DEVICE_NAME);
        return ret;
    }

    counter_class = class_create(DEVICE_NAME);
    if (IS_ERR(counter_class)) {
        unregister_chrdev_region(MKDEV(major_number, 0), num_devices);
        pr_err("Failed to create class\n");
        return PTR_ERR(counter_class);
    }

    for (i = 0; i < num_devices; i++) {
        cdev_init(&counter_devices[i].cdev, &fops);
        counter_devices[i].cdev.owner = THIS_MODULE;
        counter_devices[i].counter_value = 0;
        counter_devices[i].step = 1;

        ret = cdev_add(&counter_devices[i].cdev, MKDEV(major_number, i), 1);
        if (ret) {
            pr_notice("Error %d adding %s%d\n", ret, DEVICE_NAME, i);
            while (i--)
                cdev_del(&counter_devices[i].cdev);
            class_destroy(counter_class);
            unregister_chrdev_region(MKDEV(major_number, 0), num_devices);
            return ret;
        }

        device_create(counter_class, NULL, MKDEV(major_number, i), NULL, "%s%d", DEVICE_NAME, i);
        pr_info("Device %s%d registered with major %d and minor %d\n", DEVICE_NAME, i, major_number, i);
    }

    pr_info("Device %s inserted\n", DEVICE_NAME);
    return 0;
}

static void __exit counter_exit(void) {
    int i;

    for (i = 0; i < num_devices; i++) {
        device_destroy(counter_class, MKDEV(major_number, i));
        cdev_del(&counter_devices[i].cdev);
    }
    class_destroy(counter_class);
    unregister_chrdev_region(MKDEV(major_number, 0), num_devices);

    pr_info("Device %s removed\n", DEVICE_NAME);
}

static int counter_open(struct inode *inode, struct file *file) {
    int minor = iminor(inode);
    file->private_data = &counter_devices[minor];
    return 0;
}

static int counter_release(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t counter_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    struct device_data *dev = file->private_data;
    uint8_t data = dev->counter_value;
    int ret;
    int dev_minor = MINOR(file->f_inode->i_rdev);

    if (dev->counter_value > MAX_NUMBER) {
        dev->counter_value = 0;
    }
    if (dev->counter_value <= MIN_NUMBER) {
        dev->counter_value = 255;
    }

    if (length < sizeof(data)) {
        return -EINVAL;
    }

    ret = copy_to_user(buffer, &data, sizeof(data));
    if (ret) {
        return -EFAULT;
    }

    dev->counter_value += dev->step;
    
    pr_debug("%s%d: read %zu\n", DEVICE_NAME, dev_minor, sizeof(data));
    return sizeof(data);
}

static ssize_t counter_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    struct device_data *dev = file->private_data;
    uint8_t value;
    ssize_t bytes_written;
    int dev_minor = MINOR(file->f_inode->i_rdev);

    if (length != sizeof(value)) {
        return -EINVAL;
    }

    bytes_written = copy_from_user(&value, buffer, sizeof(value));
    if (bytes_written < 0) {
        return bytes_written;
    }

    dev->counter_value = value;
    pr_debug("%s%d: wrote %zu\n", DEVICE_NAME, dev_minor, sizeof(value));
    return sizeof(value);
}

static long counter_ioctl(struct file *file, unsigned int command, unsigned long arg) {
    struct device_data *my_dev = file->private_data;
    int ret = 0;
    int counter_value;
    int step_value;
    int dev_minor = MINOR(file->f_inode->i_rdev);

    switch (command) {
        case DEV_IOC_RST:
            pr_debug("%s%d: DEV_IOC_RST\n", DEVICE_NAME, dev_minor);
            my_dev->counter_value = 0;
            my_dev->step = 1;
            break;
        case DEV_IOC_GET:
            counter_value = my_dev->counter_value;
            pr_debug("%s%d: DEV_IOC_GET: %d\n", DEVICE_NAME, dev_minor, counter_value);
            if (copy_to_user((int __user *)arg, &counter_value, sizeof(counter_value))) {
                return -EFAULT;
            }
            break;
        case DEV_IOC_STP:
            if (copy_from_user(&step_value, (int __user *)arg, sizeof(step_value))) {
                return -EFAULT;
            }
            pr_debug("%s%d: DEV_IOC_STP: %d\n", DEVICE_NAME, dev_minor, step_value);
            my_dev->step = step_value;
            break;
        default:
            pr_notice("%s%d: DEV_IOC_ERR\n", DEVICE_NAME, dev_minor);
            return -ENOTTY;
    }
    return ret;
}

module_init(counter_init);
module_exit(counter_exit);
