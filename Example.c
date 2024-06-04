#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hmc5883l.h"

int main(void)
{   
    /* Khởi tạo struct cấu hình*/
    struct hmc5883l_config  config = {
        .MEASUREMENT  = HMC5883L_MEASUREMENT_NORMAL,    // Chế độ đo thông thường
        .RATE         = HMC5883L_RATES_15HZ,            // Tốc độ lấy mẫu 15Hz
        .SAMPLES      = HMC5883L_SAMPLES_8,             // Số lần lấy mẫu 8 lần
        .GAIN         = HMC5883L_RANGE_2_5GA,           // Độ lợi (Gain) 2.5GA
        .MODE         = HMC5883L_MODE_CONTINOUS         // Chế độ đo liên tục
    };

    // Hàm cấu hình tùy chỉnh
    int hmc5883l = hmc5883l_Adjust_Setup(config, 0);

    while(1){
        float Magnetic_X = hmc5883l_Magnetic(hmc5883l, Axis_X, MILIGAUSS);  // Từ trường trục X

        float Angle_Y = hmc5883l_Angle(hmc5883l, Axis_Y);                   // Góc xoay quanh trục Y       
        printf("Angle Axis Y: %0.3f\n", Angle_Y);

        /**
         * Khi sử dụng hai hàm bên dưới cần Import thư viện "math.h"
        */
        float Angle_compass = hmc5883l_Angle(hmc5883l, COMPASS);            // Góc địa lý       
        printf("Angle Compass: %0.3f\n", Angle_compass);

        char DIRECTION[10];
        hmc5883l_Direction(hmc5883l, DIRECTION);                            // La bàn số
        printf("Direction: %s\n", DIRECTION);

        sleep(1);                                                           // Delay 1s
    }
    close(hmc5883l);                                                        // Đóng thiết bị

    return 0;
}
    