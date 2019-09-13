/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  MPU9250 library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#include "IMU.h"

IMU::IMU(PinName I2C_SDA, PinName I2C_SCL, int frequency): i2c(I2C_SDA, I2C_SCL), mpu9250(&i2c)
{
    i2c.frequency(frequency);
    // give a moment to i2c to setup
    wait_ms(10);
}

bool IMU::init(void)
{
    return mpu9250.begin();
}

imu_data IMU::read(void)
{
    imu_data output;
    mpu9250.readAccelXYZ(&output.ax, &output.ay, &output.az);
    mpu9250.readGyroXYZ(&output.gx, &output.gy, &output.gz);
    mpu9250.readMagnetXYZ(&output.mx, &output.my, &output.mz);
    return output;
}

int IMU::read_heading(imu_data* data)
{
    int heading = atan2(data->my, data->mx) / 0.0174532925; //Calculate the degree using X and Y parameters with this formulae
    //Convert result into 0 to 360
    if (heading < 0)
        heading += 360;
    heading = 360 - heading;
    return heading;
}