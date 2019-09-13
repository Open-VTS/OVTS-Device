/**
 @file MPU9250_MPU9250.cpp
 @brief This is a library for the FaBo 9Axis I2C Brick.

   http://fabo.io/202.html

   Released under APACHE LICENSE, VERSION 2.0

   http://www.apache.org/licenses/

 @author FaBo<info@fabo.io>
*/

#include "MPU9250.h"

/**
 @brief Constructor
 @param [in] addr MPU-9250 I2C slave address
*/
MPU9250::MPU9250(I2C *i2c, uint8_t addr) {
  _mpu9250addr = addr;
  _i2c = i2c;
}

/**
 @brief Begin Device
 @retval true normaly done
 @retval false device error
*/
bool MPU9250::begin() {
  if (searchDevice()) {
    configMPU9250();
    configAK8963();
    return true;
  } else {
    return false;
  }
}

/**
 @brief Search Device
 @retval true device connected
 @retval false device error
*/
bool MPU9250::searchDevice() {
  #if 0
  uint8_t whoami;
  readI2c(_mpu9250addr, MPU9250_WHO_AM_I, 1, &whoami);
  printf("%#x \n", whoami);
  if(whoami == 0x73){
    return true;
  } else{
    return false;
  }
  #endif
  uint8_t whoami = readByte(_mpu9250addr, MPU9250_WHO_AM_I); // Read WHO_AM_I register for MPU-9250
  // printf("%#x \n", whoami);
  if (whoami != 0x71) return false;
  return true;
}

/**
 @brief Configure MPU-9250
 @param [in] gfs Gyro Full Scale Select(default:MPU9250_GFS_250[+250dps])
 @param [in] afs Accel Full Scale Select(default:MPU9250_AFS_2G[2g])
*/
void MPU9250::configMPU9250(uint8_t gfs, uint8_t afs) {
  switch(gfs) {
    case MPU9250_GFS_250:
      _gres = 250.0/32768.0;
      break;
    case MPU9250_GFS_500:
      _gres = 500.0/32768.0;
      break;
    case MPU9250_GFS_1000:
      _gres = 1000.0/32768.0;
      break;
    case MPU9250_GFS_2000:
      _gres = 2000.0/32768.0;
      break;
  }
  switch(afs) {
    case MPU9250_AFS_2G:
      _ares = 2.0/32768.0;
      break;
    case MPU9250_AFS_4G:
      _ares = 4.0/32768.0;
      break;
    case MPU9250_AFS_8G:
      _ares = 8.0/32768.0;
      break;
    case MPU9250_AFS_16G:
      _ares = 16.0/32768.0;
      break;
  }
  // sleep off
  writeI2c(_mpu9250addr, MPU9250_PWR_MGMT_1, 0x00);
  wait_ms(100);
  // auto select clock source
  writeI2c(_mpu9250addr, MPU9250_PWR_MGMT_1, 0x01);
  wait_ms(200);
  // DLPF_CFG
  writeI2c(_mpu9250addr, MPU9250_CONFIG, 0x03);
  // sample rate divider
  writeI2c(_mpu9250addr, MPU9250_SMPLRT_DIV, 0x04);
  // gyro full scale select
  writeI2c(_mpu9250addr, MPU9250_GYRO_CONFIG, gfs << 3);
  // accel full scale select
  writeI2c(_mpu9250addr, MPU9250_ACCEL_CONFIG, afs << 3);
  // A_DLPFCFG
  writeI2c(_mpu9250addr, MPU9250_ACCEL_CONFIG_2, 0x03);
  // BYPASS_EN
  writeI2c(_mpu9250addr, MPU9250_INT_PIN_CFG, 0x02);
  wait_ms(100);
}
#if 0
void MPU9250::dumpConfig() {
  uint8_t c;
  readI2c(_mpu9250addr, MPU9250_GYRO_CONFIG, 1, &c);
  Serial.println(c,HEX);
  readI2c(_mpu9250addr, MPU9250_ACCEL_CONFIG, 1, &c);
  Serial.println(c,HEX);
  readI2c(_mpu9250addr, MPU9250_ACCEL_CONFIG_2, 1, &c);
  Serial.println(c,HEX);
  readI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL1, 1, &c);
  Serial.println(c,HEX);
}
#endif
/**
 @brief Configure AK8963
 @param [in] mode Magneto Mode Select(default:AK8963_MODE_C8HZ[Continous 8Hz])
 @param [in] mfs Magneto Scale Select(default:AK8963_BIT_16[16bit])
*/
void MPU9250::configAK8963(uint8_t mode, uint8_t mfs) {
  uint8_t data[3];

  switch(mfs) {
    case AK8963_BIT_14:
      _mres = 4912.0/8190.0;
      break;
    case AK8963_BIT_16:
      _mres = 4912.0/32760.0;
      break;
  }

  // set software reset
//   writeI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL2, 0x01);
//   wait_ms(100);
  // set power down mode
  writeI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL1, 0x00);
  wait_ms(1);
  // set read FuseROM mode
  writeI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL1, 0x0F);
  wait_ms(1);
  // read coef data
  readI2c(AK8963_SLAVE_ADDRESS, AK8963_ASAX, 3, data);
  _magXcoef = (float)(data[0] - 128) / 256.0 + 1.0;
  _magYcoef = (float)(data[1] - 128) / 256.0 + 1.0;
  _magZcoef = (float)(data[2] - 128) / 256.0 + 1.0;
  // set power down mode
  writeI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL1, 0x00);
  wait_ms(1);
  // set scale&continous mode
  writeI2c(AK8963_SLAVE_ADDRESS, AK8963_CNTL1, (mfs<<4|mode));
  wait_ms(1);
}

/**
 @brief Check data ready
 @retval true data is ready
 @retval false data is not ready
*/
#if 0
bool MPU9250::checkDataReady() {
  uint8_t drdy;
  readI2c(_mpu9250addr, MPU9250_INT_STATUS, 1, &drdy);
  if ( drdy & 0x01 ) {
    return true;
  } else {
    return false;
  }
}
#endif
/**
 @brief Read accelerometer
 @param [out] ax X-accel(g)
 @param [out] ay Y-accel(g)
 @param [out] az Z-accel(g)
*/
void MPU9250::readAccelXYZ(float * ax, float * ay, float * az) {
  uint8_t data[6];
  int16_t axc, ayc, azc;
  readI2c(_mpu9250addr, MPU9250_ACCEL_XOUT_H, 6, data);
  axc = ((int16_t)data[0] << 8) | data[1];
  ayc = ((int16_t)data[2] << 8) | data[3];
  azc = ((int16_t)data[4] << 8) | data[5];
  *ax = (float)axc * _ares;
  *ay = (float)ayc * _ares;
  *az = (float)azc * _ares;
}

/**
 @brief Read gyro
 @param [out] gx X-gyro(degrees/sec)
 @param [out] gy Y-gyro(degrees/sec)
 @param [out] gz Z-gyro(degrees/sec)
*/
#if 1
void MPU9250::readGyroXYZ(float * gx, float * gy, float * gz) {
  uint8_t data[6];
  int16_t gxc, gyc, gzc;
  readI2c(_mpu9250addr, MPU9250_GYRO_XOUT_H, 6, data);
  gxc = ((int16_t)data[0] << 8) | data[1];
  gyc = ((int16_t)data[2] << 8) | data[3];
  gzc = ((int16_t)data[4] << 8) | data[5];
  *gx = (float)gxc * _gres;
  *gy = (float)gyc * _gres;
  *gz = (float)gzc * _gres;
}
#endif
/**
 @brief Read magneto
 @param [out] mx X-magneto(uT)
 @param [out] my Y-magneto(uT)
 @param [out] mz Z-magneto(uT)
*/
void MPU9250::readMagnetXYZ(float * mx, float * my, float * mz) {
  uint8_t data[7];
  uint8_t drdy;
  int16_t mxc, myc, mzc;
  readI2c(AK8963_SLAVE_ADDRESS, AK8963_ST1, 1, &drdy);
  if ( drdy & 0x01 ) { // check data ready
    readI2c(AK8963_SLAVE_ADDRESS, AK8963_HXL, 7, data);
    if ( !(data[6] & 0x08) ) { // check overflow
      mxc = ((int16_t)data[1] << 8) | data[0];
      myc = ((int16_t)data[3] << 8) | data[2];
      mzc = ((int16_t)data[5] << 8) | data[4];
      *mx = (float)mxc * _mres * _magXcoef;
      *my = (float)myc * _mres * _magYcoef;
      *mz = (float)mzc * _mres * _magZcoef;
    }
  }
}

/**
 @brief Read temperature
 @param [out] temperature temperature(degrees C)
*/
#if 0
void MPU9250::readTemperature(float * temperature) {
  uint8_t data[2];
  int16_t tmc;
  readI2c(_mpu9250addr, MPU9250_TEMP_OUT_H, 2, data);
  tmc = ((int16_t)data[0] << 8) | data[1];
  *temperature = ((float)tmc) / 333.87 + 21.0;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////


char MPU9250::readByte(uint8_t address, uint8_t register_addr)
{
  char data[1]; // `data` will store the register data
  char data_write[1];
  data_write[0] = register_addr;
  _i2c->write(address, data_write, 1, 1); // no stop
  _i2c->read(address, data, 1, 0);
  return data[0];
}



/**
 @brief Write I2C
 @param [in] address I2C slave address
 @param [in] register_addr register address
 @param [in] data write data
*/
void MPU9250::writeI2c(uint8_t address, uint8_t register_addr, uint8_t data) {
  char data_write[2];
  data_write[0] = register_addr;
  data_write[1] = data;
  _i2c->write(address, data_write, 2, 0);
}

/**
 @brief Read I2C
 @param [in] address I2C slave address
 @param [in] register_addr register address
 @param [in] num read length
 @param [out] buffer read data
*/
void MPU9250::readI2c(uint8_t address, uint8_t register_addr, uint8_t num, uint8_t * buffer) {
  char data[14];
  char data_write[1];
  data_write[0] = register_addr;
  _i2c->write(address, data_write, 1, 1); // no stop
  _i2c->read(address, data, num, 0);
  for (int ii = 0; ii < num; ii++)
  {
    buffer[ii] = data[ii];
  }
}
#if 0
struct Three_float MPU9250::readAccel(){
    struct Three_float output;
    float ax,ay,az;
    readAccelXYZ(&ax,&ay,&az);
    output = {ax, ay, az};
    return output;
}


struct Three_float MPU9250::readGyro(){
    struct Three_float output;
    float gx,gy,gz;
    readGyroXYZ(&gx,&gy,&gz);
    output = {gx, gy, gz};
    return output;
}

struct Three_float MPU9250::readMagnet(){
    struct Three_float output;
    float mx,my,mz;
    readMagnetXYZ(&mx,&my,&mz);
    output = {mx, my, mz};
    return output;
}
#endif
struct imu_ypr_data MPU9250::readYPR(){
  struct imu_ypr_data output;
  double roll , pitch, yaw;
  float accelX, accelY, accelZ, magX, magY, magZ;
  readAccelXYZ(&accelX,&accelY,&accelZ);
  readMagnetXYZ(&magX,&magY,&magZ);
  magX = (float) magX/57.3;
  magY = (float) magY/57.3;
  magZ = (float) magZ/57.3;


  pitch = atan2 (accelY ,( sqrt ((accelX * accelX) + (accelZ * accelZ))));
  roll = atan2(-accelX ,( sqrt((accelY * accelY) + (accelZ * accelZ))));

  // yaw from mag
  float Yh = (magY * cos(roll)) - (magZ * sin(roll));
  float Xh = (magX * cos(pitch))+(magY * sin(roll)*sin(pitch)) + (magZ * cos(roll) * sin(pitch));

  yaw =  atan2(Yh, Xh);
  roll = roll*57.3;
  pitch = pitch*57.3;
  yaw = yaw*57.3;

  output.yaw = yaw;
  output.pitch = pitch;
  output.roll = roll;
  return output;
}


#if 0
float MPU9250::readTemperature(){
    float temp;
    readTemperature(&temp);
    return temp;
}
#endif
