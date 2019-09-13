/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  MPU9250 library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#ifndef imu_h
#define imu_h


#include "mbed.h"
#include "MPU9250.h"

typedef struct{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float mx;
    float my;
    float mz;
} imu_data;

class IMU
{
public:
    IMU(PinName I2C_SDA, PinName I2C_SCL, int frequency=400000);
    bool init(void);
    imu_data read(void);
    int read_heading(imu_data* data);
private:
    I2C i2c;
    MPU9250 mpu9250;
};

#endif
