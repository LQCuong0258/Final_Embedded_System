#include "hmc5883l.h"

/**
 * Hàm có chức năng cấu hình mặc định cho HMC5883L
*/
volatile int hmc5883l_Default_Setup(int check)
{
    int hmc5883l = open(DEVICE_PATH, O_RDWR);
    if (hmc5883l < 0) {
        printf("Can't open I2C\n");
        exit(1);
        return errno;
    }

    struct hmc5883l_config  defaul = {                          // Struct được cấu hình mặc định
        .MEASUREMENT  = HMC5883L_MEASUREMENT_NORMAL,
        .RATE         = HMC5883L_RATES_15HZ,
        .SAMPLES      = HMC5883L_SAMPLES_1,
        .GAIN         = HMC5883L_RANGE_1_3GA,
        .MODE         = HMC5883L_MODE_SINGLE
    };

    if (ioctl(hmc5883l, HMC5883L_IOCTL_CONFIG, &defaul) < 0) {  // Gửi struct cấu hình xuống Kernel
        perror("Failed to Configuration HMC5883L");
        close(hmc5883l);
        exit(1);
        return errno;
    }

    if(check == 1) {printf("Configuration successful!\n");}     // Thông báo thành công

    return hmc5883l;
}

/**
 * Hàm có chức năng cấu hình tùy chỉnh theo người dùng
*/
volatile int hmc5883l_Adjust_Setup(struct hmc5883l_config  config, int check)
{
    int hmc5883l = open(DEVICE_PATH, O_RDWR);                   // Mở file thiết bị I2C
    if (hmc5883l < 0) {
        printf("Can't open I2C\n");
        exit(1);
        return errno;
    }

    if (ioctl(hmc5883l, HMC5883L_IOCTL_CONFIG, &config) < 0) {  // Gửi struct cấu hình xuống Kernel
        perror("Failed to Configuration HMC5883L");
        close(hmc5883l);
        exit(1);
        return errno;
    }

    if(check == 1) {printf("Configuration successful!\n");}     // Thông báo thành công

    return hmc5883l;
}

/**
 * Hàm có chức năng trả về giá trị của từ trường theo mỗi trục
*/
volatile float hmc5883l_Magnetic(int hmc5883l, int axis, int format)
{
    float CurMagnetic;
    int gain;
    int16_t magnetic;

    switch (axis)
    {
    case Axis_X:    // Từ trường trục X
        if(ioctl(hmc5883l, HMC5883L_IOCTL_MAGNETIC_X, &magnetic) < 0) {printf("error magnetic Axis X\n");}
        break;
    case Axis_Y:    // Từ trường trục Y
        if(ioctl(hmc5883l, HMC5883L_IOCTL_MAGNETIC_Y, &magnetic) < 0) {printf("error magnetic Axis Y\n");}
        break;
    case Axis_Z:    // Từ trường trục Z
        if(ioctl(hmc5883l, HMC5883L_IOCTL_MAGNETIC_Z, &magnetic) < 0) {printf("error magnetic Axis Z\n");}
        break;
    default:        // Tham số nhập vào không hợp lệ
        printf("Invalid parameter error!!\n");
        break;
    }

    /* Lựa chọn đơn vị */
    if(format == GAUSS){    
        if(ioctl(hmc5883l, HMC5883L_IOCTL_GAUSSGAIN, &gain) < 0) {printf("error gauss Gain\n");}
        CurMagnetic = (magnetic*1.0) / (gain*1.0);      // Tính toán ra đơn vị Gauss
    }
    else if(format == MILIGAUSS){
        if(ioctl(hmc5883l, HMC5883L_IOCTL_MILIGAUSSGAIN, &gain) < 0) {printf("error mili gauss Gain\n");}
        CurMagnetic = (magnetic*1.0) * (gain/100.0);    // Tính tóa ra đơn vị miliGauss
    }

    return CurMagnetic;
}

/**
 * Hàm có chức năng trả về giá trị góc quanh trục
*/
volatile float hmc5883l_Angle(int hmc5883l, int axis)
{
    float angle, mag_X, mag_Y, mag_Z;

    switch (axis)
    {
    case Axis_X:
        mag_Y = hmc5883l_Magnetic(hmc5883l, Axis_Y, MILIGAUSS);
        mag_Z = hmc5883l_Magnetic(hmc5883l, Axis_Z, MILIGAUSS);
        angle = atan2(mag_Y, mag_Z);
        break;
    case Axis_Y:
        mag_X = hmc5883l_Magnetic(hmc5883l, Axis_X, MILIGAUSS);
        mag_Z = hmc5883l_Magnetic(hmc5883l, Axis_Z, MILIGAUSS);
        angle = atan2(mag_X, mag_Z);

        break;
    case Axis_Z:
        mag_X = hmc5883l_Magnetic(hmc5883l, Axis_X, MILIGAUSS);
        mag_Y = hmc5883l_Magnetic(hmc5883l, Axis_Y, MILIGAUSS);
        angle = atan2(mag_X, mag_Y);
        break;
    default:
        printf("Invalid parameter error!!\n");
        break;
    }


    angle -= (PI/6.0);              // Hiệu chỉnh lượng góc lệch nhau giữa trục từ trường và góc địa lý
    if(angle < 0) angle += 2*PI;    // Giới hạn giá trị lại để góc đo được thuộc khoảng [0; 360]
    angle = (angle*180.0 / PI);     // Chuyển đổi góc về đơn vị độ

    return angle;
}

/**
 * Hàm có chức năng trả về hướng đo hiện tại
*/
volatile void hmc5883l_Direction(int hmc5883l, char *direction)
{
    float angle = hmc5883l_Angle(hmc5883l, Axis_Z);

    if((angle >= 345.0) || (angle < 15.0)){         // Hướng Bắc
        strcpy(direction, "North");
    }
    else if((angle >= 15.0) && (angle < 75.0)){     // Hướng Đông-Bắc
        strcpy(direction, "North_East");
    }
    else if((angle >= 75.0) && (angle < 105.0)){    // Hướng Đông
        strcpy(direction, "East");
    }
    else if((angle >= 105.0) && (angle < 165.0)){   // Hướng Đông-Nam
        strcpy(direction, "South_East");
    }
    else if((angle >= 165.0) && (angle <= 195.0)){  // Hướng Nam
        strcpy(direction, "South");
    }
    else if((angle >= 195.0) && (angle <= 255.0)){  // Hướng Tây-Nam
        strcpy(direction, "South_West");
    }
    else if((angle >= 255.0) && (angle <= 285.0)){  // Hướng Tây
        strcpy(direction, "West");
    }
    else{
        strcpy(direction, "North_West");            // Hướng Tây-Bắc
    }

}