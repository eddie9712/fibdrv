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
static void add_bignum(char *str,
                       char *cp1,
                       char *cp2)  //*cp2 should be longer than cp1
{
    char str1[50];
    char str2[50];
    strncpy(str1, cp1, 50);
    strncpy(str2, cp2, 50);
    int n1 = strlen(str1);  // caculate the length of two strings
    int n2 = strlen(str2);
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
    stringReverse(str);
}
static void sub_bignum(char *str,
                       char *str1,
                       char *str2)  // string 1 should be  bigger  than str2
{
    char cp1[50], cp2[50];
    strncpy(cp1, str1, 50);  // avoid to change  the original value
    strncpy(cp2, str2, 50);
    int n1 = strlen(cp1);  // caculate the length of two strings
    int n2 = strlen(cp2);
    stringReverse(cp1);  // reverse two string
    stringReverse(cp2);
    int i;
    for (i = 0; i < n2; i++) {
        int borrow = 0;
        if (cp1[i] < cp2[i])  // borrow
            borrow = 1;
        int sum = cp1[i] - cp2[i];
        str[i] = sum + '0';
        if (borrow == 1) {
            str[i] = sum + '\n' + '0';
            cp1[i + 1] -= ('2' - '1');  // let str[i+1] minus one
        }
    }

    for (i = n2; i < n1; i++)  // remain part
    {
        str[i] = cp1[i];
    }
    while (str[n2] == '0' || str[n2] == '\0')  // get rid of leading zero
    {
        str[n2] = '\0';
        n2--;
    }
    stringReverse(str);
}
static void mult_bignum(char *str,
                        char *str1,
                        char *str2)  // multiply big numbers
{
    int l1 = strlen(str1);
    int l2 = strlen(str2);

    int result[50] = {0};
    // Below two indexes are used to find positions
    // in result.
    int i_n1 = 0;
    // Go from right to left in str1
    for (int i = l1 - 1; i >= 0; i--) {
        int carry = 0;
        int n1 = str1[i] - '0';
        // To shift position to left after every
        // multiplication of a digit in str2
        int i_n2 = 0;


        // Go from right to left in num2
        for (int j = l2 - 1; j >= 0; j--) {
            // Take current digit of second number
            int n2 = str2[j] - '0';

            // Multiply with current digit of first number
            // and add result to previously stored result
            // at current position.
            int sum = n1 * n2 + result[i_n1 + i_n2] + carry;
            // Carry for next iteration
            carry = sum / 10;
            // Store result
            result[i_n1 + i_n2] = sum % 10;

            i_n2++;
        }

        // store carry in next cell
        if (carry > 0)
            result[i_n1 + i_n2] += carry;
        // To shift position to left after every
        // multiplication of a digit in num1.
        i_n1++;
    }
    // ignore '0's from the right
    int i = 49;
    while (i >= 0 && result[i] == 0)
        i--;  // first non-zero term
    if (i != -1) {
        // generate the result string
        while (i >= 0) {
            str[i] = result[i] + '0';
            i--;
        }
        stringReverse(str);
    } else {
        strncpy(str, "0", 50);
    }
}
static void fib_sequence(char *a, long long k)
{
    kt = ktime_get();
    unsigned int h = 0;
    if (k != 0) {
        h = __builtin_clz(k);
    } else
        h = 0;
    // h=0;
    // for (unsigned int i = k; i; ++h, i >>= 1);

    char b[50];
    strncpy(a, "0", 50);  // F(0)=0
    strncpy(b, "1", 50);  // F(1)=1

    for (int j = h - 1; j >= 0; --j) {
        char c[50] = {'\0'};
        char d[50] = {'\0'};
        char temp[50] = {'\0'};

        mult_bignum(c, b, "2");  // get c
        sub_bignum(c, c, a);
        mult_bignum(c, c, a);

        mult_bignum(d, a, a);  // get d
        mult_bignum(temp, b, b);
        add_bignum(d, d, temp);

        if ((k >> j) & 1) {
            strncpy(a, d, 50);
            add_bignum(b, c, d);
        } else {
            strncpy(a, c, 50);
            strncpy(b, d, 50);
        }
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
    char str[50] = {'\0'};
    fib_sequence(str, *offset);
    copy_to_user(buf, str, 50);
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
