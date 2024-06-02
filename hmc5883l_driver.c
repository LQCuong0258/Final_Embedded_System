#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>


#define DRIVER_NAME     "hmc5883l_driver"
#define CLASS_NAME      "hmc5883l_class"
#define DEVICE_NAME     "hmc5883l_device"

#define HMC5883L_ADDRESS              0x1E
#define HMC5883L_REG_CONFIG_A         0x00
#define HMC5883L_REG_CONFIG_B         0x01
#define HMC5883L_REG_MODE             0x02
#define HMC5883L_REG_OUT_X_M          0x03
#define HMC5883L_REG_OUT_X_L          0x04
#define HMC5883L_REG_OUT_Z_M          0x05
#define HMC5883L_REG_OUT_Z_L          0x06
#define HMC5883L_REG_OUT_Y_M          0x07
#define HMC5883L_REG_OUT_Y_L          0x08
#define HMC5883L_REG_STATUS           0x09
#define HMC5883L_REG_IDENT_A          0x0A
#define HMC5883L_REG_IDENT_B          0x0B
#define HMC5883L_REG_IDENT_C          0x0C
/**
 * Configuration Register B(R/W): 0x01
*/

/* Location CRB7 to CRB5 */
#define     HMC5883L_RANGE_0_88GA    0x00
#define     HMC5883L_RANGE_1_3GA     0x20       // Default
#define     HMC5883L_RANGE_1_9GA     0x40
#define     HMC5883L_RANGE_2_5GA     0x60
#define     HMC5883L_RANGE_4GA       0x80
#define     HMC5883L_RANGE_4_7GA     0xA0
#define     HMC5883L_RANGE_5_6GA     0xC0
#define     HMC5883L_RANGE_8_1GA     0xE0
/**
 * Identification Register A, B, C (R): 0x0A
*/
#define     HMC5883L_CHECK_IDENT_A  0x48      
#define     HMC5883L_CHECK_IDENT_B  0x34
#define     HMC5883L_CHECK_IDENT_C  0x33
/**
 * Define variable
*/
#define     Axis_X      0
#define     Axis_Y      1
#define     Axis_Z      2
/**
 * Struct to configuration
*/
struct hmc5883l_config {
    /* Register A */
    uint8_t SAMPLES;
    uint8_t RATE;
    uint8_t MEASUREMENT;
    /* Register B */
    uint8_t GAIN;
    /* Mode Register */
    uint8_t MODE;
};

// List of ioctl command
#define HMC5883L_IOCTL_MAGIC 'h'
#define HMC5883L_IOCTL_CONFIG           _IOW(HMC5883L_IOCTL_MAGIC, 0, struct hmc5883l_config *)
#define HMC5883L_IOCTL_MAGNETIC_X       _IOR(HMC5883L_IOCTL_MAGIC, 1, int)
#define HMC5883L_IOCTL_MAGNETIC_Y       _IOR(HMC5883L_IOCTL_MAGIC, 2, int)
#define HMC5883L_IOCTL_MAGNETIC_Z       _IOR(HMC5883L_IOCTL_MAGIC, 3, int)
#define HMC5883L_IOCTL_GAUSSGAIN        _IOR(HMC5883L_IOCTL_MAGIC, 4, int)
#define HMC5883L_IOCTL_MILIGAUSSGAIN    _IOR(HMC5883L_IOCTL_MAGIC, 5, int)

static struct i2c_client *hmc5883l_client;
static struct class* hmc5883l_class = NULL;
static struct device* hmc5883l_device = NULL;
static int major_number;

static int hmc5883l_check_config(void)
{
    if(i2c_smbus_read_byte_data(hmc5883l_client, HMC5883L_REG_IDENT_A) != HMC5883L_CHECK_IDENT_A){
        printk(KERN_INFO "Failed to Configuration A Register!!!\n");
        return -EIO;
    }
    if(i2c_smbus_read_byte_data(hmc5883l_client, HMC5883L_REG_IDENT_B) != HMC5883L_CHECK_IDENT_B){
        printk(KERN_INFO "Failed to Configuration B Register!!!\n");
        return -EIO;
    }
    if(i2c_smbus_read_byte_data(hmc5883l_client, HMC5883L_REG_IDENT_C) != HMC5883L_CHECK_IDENT_C){
        printk(KERN_INFO "Failed to Configuration C Register!!!\n");
        return -EIO;
    }
    printk(KERN_INFO "Configuration succsefully!\n");
    return 1;
}

static void hmc5883l_wait_status(void)
{
    uint8_t status;
    do {
        status = i2c_smbus_read_byte_data(hmc5883l_client, HMC5883L_REG_STATUS);
    } while ((status & 0x01) != 0x01);
}

static int hmc5883l_Setup(struct hmc5883l_config *config)
{
    uint8_t ret;
    uint8_t registerA  = (config->SAMPLES << 5) | (config->RATE << 2) | (config->MEASUREMENT);

    /* Setup Configuration register A*/
    ret = i2c_smbus_write_byte_data(hmc5883l_client, HMC5883L_REG_CONFIG_A, registerA);
    if(ret < 0){
        printk(KERN_ERR "configuration at register A failed: %d\n", ret); 
        return -EIO;
    }
    /* Setup Configuration register B*/
    ret = i2c_smbus_write_byte_data(hmc5883l_client, HMC5883L_REG_CONFIG_B, config->GAIN);
    if(ret < 0){
        printk(KERN_ERR "configuration at register B failed: %d\n", ret);
        return -EIO;
    }
    /* Setup Register MODE*/
    ret = i2c_smbus_write_byte_data(hmc5883l_client, HMC5883L_REG_MODE,  config->MODE);
    if(ret < 0){
        printk(KERN_ERR "configuration at register MODE failed: %d\n", ret);
        return -EIO;
    }

    hmc5883l_wait_status();

    return 1;
}

static int hmc5883l_get_gain(struct i2c_client *client, int option)
{
    int output, gauss, mgauss;
    uint8_t reg = i2c_smbus_read_byte_data(client, HMC5883L_REG_CONFIG_B);
    if(reg < 0){
        printk(KERN_INFO "Failed to read data Register B!!!\n");
        return -EIO;
    }
    // mili gauss đã nhân 100
    switch(reg){
    case HMC5883L_RANGE_0_88GA:
        gauss   = 1370;
        mgauss  = 73;
        break;
    case HMC5883L_RANGE_1_3GA:      // Default
        gauss   = 1090;
        mgauss  = 92;
        break;
    case HMC5883L_RANGE_1_9GA:
        gauss   = 820;
        mgauss  = 122;
        break;
    case HMC5883L_RANGE_2_5GA:
        gauss   = 660;
        mgauss  = 152;
        break;
    case HMC5883L_RANGE_4GA:
        gauss   = 440;
        mgauss  = 227;
        break;
    case HMC5883L_RANGE_4_7GA:
        gauss   = 390;
        mgauss  = 256;
        break;
    case HMC5883L_RANGE_5_6GA:
        gauss   = 330;
        mgauss  = 303;
        break;
    case HMC5883L_RANGE_8_1GA:
        gauss   = 230;
        mgauss  = 435;
        break;
    default:
        printk(KERN_ERR "Invalid gain value read from Register B\n");
        break;
    }

    if(option == 1) {output = mgauss;}
    else {output = gauss;}

    return output;
}
static int16_t hmc5883l_get_magnetic(struct i2c_client *client, int axis)
{
    uint8_t data[6];
    int16_t array[3];

    for (int i = 0; i < 6; i++) {
        data[i] = i2c_smbus_read_byte_data(client, HMC5883L_REG_OUT_X_M + i);
    }
    
    array[0] = (int16_t)((data[0] << 8) | data[1]);     // X
    array[2] = (int16_t)((data[2] << 8) | data[3]);     // Z
    array[1] = (int16_t)((data[4] << 8) | data[5]);     // Y

    /* Wait ready register */
    hmc5883l_wait_status();

    return array[axis];
}

static int hmc5883l_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "HMC5883L device opened\n");
    return 0;
}
static int hmc5883l_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "HMC5883L device closed\n");
    return 0;
}

/**
 * In/Out_Control function
*/
static long hmc5883l_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
    struct hmc5883l_config      config;     // Struct Contains information for Initialization
    int magnetic, Gain;

    switch(command){
        case HMC5883L_IOCTL_CONFIG:
            // lấy giá trị arg từ user space gán vào struct config
            if(copy_from_user(&config, (struct hmc5883l_config __user *)arg, sizeof(config))) {return -EFAULT;}
            // Tiến hành Setup
            if(hmc5883l_Setup(&config) < 0) {return -EFAULT;}
            // Kiểm tra cấu hình
            if(hmc5883l_check_config() < 0) {return -EFAULT;}
            break;
        
        case HMC5883L_IOCTL_MAGNETIC_X:
            magnetic = hmc5883l_get_magnetic(hmc5883l_client, Axis_X);
            if(copy_to_user((int __user *)arg, &magnetic, sizeof(magnetic))) {return -EFAULT;}
            break;

        case HMC5883L_IOCTL_MAGNETIC_Y:
            magnetic = hmc5883l_get_magnetic(hmc5883l_client, Axis_Y);
            if(copy_to_user((int __user *)arg, &magnetic, sizeof(magnetic))) {return -EFAULT;}
            break;

        case HMC5883L_IOCTL_MAGNETIC_Z:
            magnetic = hmc5883l_get_magnetic(hmc5883l_client, Axis_Z);
            if(copy_to_user((int __user *)arg, &magnetic, sizeof(magnetic))) {return -EFAULT;}
            break;

        case HMC5883L_IOCTL_GAUSSGAIN:
            // lấy giá trị arg từ user space
            Gain = hmc5883l_get_gain(hmc5883l_client, 0);
            if(copy_to_user((int __user *)arg, &Gain, sizeof(Gain))) {return -EFAULT;}
        break;

        case HMC5883L_IOCTL_MILIGAUSSGAIN:
            // lấy giá trị arg từ user space
            Gain = hmc5883l_get_gain(hmc5883l_client, 1);
            if(copy_to_user((int __user *)arg, &Gain, sizeof(Gain))) {return -EFAULT;}
        break;

        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .open               = hmc5883l_open,
    .release            = hmc5883l_release,
    .unlocked_ioctl     = hmc5883l_ioctl,
};

static int hmc5883l_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    hmc5883l_client = client;
    // Create a character device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if(major_number < 0){
        printk(KERN_ERR "Registering a major number FAILED!!!\n");
        return major_number;
    }
    printk(KERN_INFO "MHMC5883L driver INSTALLED with major number: %d\n", major_number);

    hmc5883l_class = class_create(THIS_MODULE, CLASS_NAME);
    if(IS_ERR(hmc5883l_class)){
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Create class FAILED!!!\n");
        return PTR_ERR(hmc5883l_class);
    }
    hmc5883l_device = device_create(hmc5883l_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if(IS_ERR(hmc5883l_device)){
        class_destroy(hmc5883l_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Device creation FAILED!!!\n");
        return PTR_ERR(hmc5883l_device);
    }
    return 0;
}

static void hmc5883l_remove(struct i2c_client *client)
{
    device_destroy(hmc5883l_class, MKDEV(major_number, 0));     // Hủy thiết bị
    class_unregister(hmc5883l_class);                           // Xóa đăng ký class
    class_destroy(hmc5883l_class);                              // Hủy class
    unregister_chrdev(major_number, DEVICE_NAME);               // Xóa đăng ký character device

    printk(KERN_INFO "HMC5883L driver removed!!!\n");
}

static const struct of_device_id hmc5883l_of_match[] = {
    { .compatible = "Honeywell, HMC5883L", },
    { },
}; MODULE_DEVICE_TABLE(of, hmc5883l_of_match);

static struct i2c_driver hmc5883l_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(hmc5883l_of_match),
    },
    .probe      = hmc5883l_probe,
    .remove     = hmc5883l_remove,
};

static int __init hmc5883l_init (void)
{
    printk(KERN_INFO "Initializing HMC5883L deriver!!!\n");
    return i2c_add_driver(&hmc5883l_driver); // add driver into i2c of system
}

static void __exit hmc5883l_exit (void)
{
    printk(KERN_INFO "Exiting HMC5883L driver!!!\n");
    i2c_del_driver(&hmc5883l_driver);
}

module_init(hmc5883l_init);
module_exit(hmc5883l_exit);

MODULE_AUTHOR("Syaoran");
MODULE_DESCRIPTION("HMC5883L I2C Client Driver, Version: 0.2");
MODULE_LICENSE("GPL");