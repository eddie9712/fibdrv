#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 110

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);
static ktime_t kt;
static void stringReverse(char *str)
{
    char *p1, *p2;
    int i = strlen(str);
    for (p1 = str, p2 = str + i - 1; p2 > p1; ++p1, --p2) {
        *p1 ^= *p2;
        *p2 ^= *p1;
        *p1 ^= *p2;
    }
}
static void add_bignum(char *str, char *str1, char *str2)
{
    int n1 = strlen(str1);  // caculate the length of two strings
    int n2 = strlen(str2);

    if (n1 > n2)  // let string2 the bigger number
    {
        char *temp = str1;
        int swap;
        str1 = str2;
        str2 = temp;
        swap = n1;  // swap n1,n2
        n1 = n2;
        n2 = swap;
    }
    stringReverse(str1);  // reverse two string
    stringReverse(str2);
    int carry = 0, i = 0;
    for (i = 0; i < n1; i++)  // add each digit
    {
        int sum = ((str1[i] - '0') + (str2[i] - '0') + carry);
        str[i] = (sum % 10 + '0');
        carry = sum / 10;
    }
    for (i = n1; i < n2; i++)  // deal remaining digits
    {
        int sum = ((str2[i] - '0') + carry);
        str[i] = (sum % 10 + '0');
        carry = sum / 10;
    }

    if (carry)  // deal remain carry
    {
        str[i] = carry + '0';
    }
    stringReverse(str1);  // reverse two string
    stringReverse(str2);

    stringReverse(str);
}

static void fib_sequence(char **f, long long k)
{
    kt = ktime_get();
    /* FIXME: use clz/ctz and fast algorithms to speed up */
    int i;
    for (i = 0; i < k + 2; i++) {
        f[i] = kmalloc(sizeof(char) * 50, GFP_KERNEL);
        strncpy(f[i], "\0", 50);  // initialize
    }
    strncpy(f[0], "0", 50);
    strncpy(f[1], "1", 50);
    for (int j = 2; j <= k; j++) {
        add_bignum(f[j], f[j - 1], f[j - 2]);
    }
    kt = ktime_sub(ktime_get(), kt);
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    char *str[(*offset) + 2];
    fib_sequence(str, *offset);
    copy_to_user(buf, str[*offset], 50);
    for (int i = 0; i < (*offset) + 2; i++)  // free the memory
    {
        kfree(str[i]);
    }
    return 1;
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return ktime_to_ns(kt);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }
    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);
    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
