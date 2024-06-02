#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <errno.h> // Include errno header

/**
 * Configuration Register A(R/W): 0x00
*/
  
/* Location CRA6 to CRA5 */
#define    HMC5883L_SAMPLES_1     0x00   // Default
#define    HMC5883L_SAMPLES_2     0x01
#define    HMC5883L_SAMPLES_4     0x02
#define    HMC5883L_SAMPLES_8     0x03

/* Location CRA4 to CRA2 */
#define     HMC5883L_RATES_0_75_HZ    0x00
#define     HMC5883L_RATES_1_5HZ      0x01
#define     HMC5883L_RATES_3HZ        0x02
#define     HMC5883L_RATES_7_5HZ      0x03
#define     HMC5883L_RATES_15HZ       0x04   // Default
#define     HMC5883L_RATES_30HZ       0x05
#define     HMC5883L_RATES_75HZ       0x06
#define     HMC5883L_RATES_RES        0x07

/* Location CRA1 to CRA0 */
#define     HMC5883L_MEASUREMENT_NORMAL     0x00  // Default
#define     HMC5883L_MEASUREMENT_POSITIVE   0x01
#define     HMC5883L_MEASUREMENT_NEGATIVE   0x02
#define     HMC5883L_MEASUREMENT_RES        0x03

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
 * Mode Register (R/W): 0x02
*/

/* Location MR1 to MR0 */
#define     HMC5883L_MODE_CONTINOUS     0x00      
#define     HMC5883L_MODE_SINGLE        0x01      // Default
#define     HMC5883L_MODE_IDLE_1        0x02
#define     HMC5883L_MODE_IDLE_2        0x03

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
/* Define path name*/
#define DEVICE_PATH     "/dev/hmc5883l_device"
/**
 * Define varialbe
*/
#define     Axis_X      0
#define     Axis_Y      1
#define     Axis_Z      2

#define     GAUSS       0
#define     MILIGAUSS   1

#define     PI          3.141592654


volatile int hmc5883l_Default_Setup(struct hmc5883l_config  config, int check);
volatile int hmc5883l_Adjust_Setup(struct hmc5883l_config  config, int check);
volatile float hmc5883l_Magnetic(int hmc5883l, int axis, int format);
volatile float hmc5883l_Angle(int hmc5883l, int axis);
volatile void hmc5883l_Direction(int hmc5883l, char *direction);
