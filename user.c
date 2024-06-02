#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "hmc5883l.h"

int main(void)
{   
    struct hmc5883l_config  config = {
        .MEASUREMENT  = HMC5883L_MEASUREMENT_NORMAL,
        .RATE         = HMC5883L_RATES_15HZ,
        .SAMPLES      = HMC5883L_SAMPLES_8,
        .GAIN         = HMC5883L_RANGE_2_5GA,
        .MODE         = HMC5883L_MODE_CONTINOUS
    };

    // Configuration
    int hmc5883l = hmc5883l_Adjust_Setup(config, 0);

    // float X = hmc5883l_Magnetic(Axis_X, NORMALIZE);

    while(1){

        float Magnetic_X = hmc5883l_Magnetic(hmc5883l, Axis_X, MILIGAUSS);

        float Angle_Z = hmc5883l_Angle(hmc5883l, Axis_Z);       // Muốn sử dụng phải import math.h
        printf("Angle: %0.3f____", Angle_Z);
        char DIRECTION[10];
        hmc5883l_Direction(hmc5883l, DIRECTION);
        printf("Direction %s\n", DIRECTION);

        usleep(50000);
    }
    close(hmc5883l);

    return 0;
}
    