/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Core library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#ifndef core_h
#define core_h

#include "mbed.h"
#include "mbedtls/platform.h"
#include "string"
#include "mbed_mem_trace.h"
#include "stm32f4xx_ll_rcc.h"
#include "mbed_error.h"
#include "mbed_fault_handler.h"

// #include "mbedtls/sha256.h"
// #include "mbedtls/aes.h"

#include "IMU.h"
#include "modem.h"
#include "connection.h"
#include "reports.h"

// preprocessor to convert deviceid to str
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DEVICEID 1
#define SOFTWARE_VER "1.1.0"
#define DATE __DATE__
#define TIME __TIME__

// Set to 1 if you want to always overwrite DEFAULT_SERVER_PARAMS
// #define SET_DEFAULT_PARAMS

// set fake gps data
// #define FAKE_GPS

#ifdef FAKE_GPS
#warning "FAKE GPS ENABLED!!!"
#endif

//del reports from sd for sent reports
// #define DELETE_COMPLETE_REPORTS

//del sd config file after read
#define DELETE_SD_CONFIG

//enable to set debug for memory
// #define DEBUG_MEM

#ifdef DEBUG_MEM
// debug memory interval
#define DEBUG_MEM_INTERVAL 6
// max memory Utilization percentage
#define MAX_MEM_UTIL 85
#endif

//SD FILE CONFIG FILE
#define SD_CONFIG_FILE "config"

// modem file names
#define CENTER_PARAMS_FILE "centerparams.txt"

// connection_type
// 1:Network, 2:sms, 3:dual
#define CONNECTION_TYPE 1

#define DEFAULT_KEY "MY_KEY"

#define DEFAULT_CENTER_ADDRESS                                                                                     \
  {                                                                                                                \
    \                                                                                                                                 
  "http://X.X.X.X:8080/device/data/",                                                                              \
        "http://X.X.X.X:8080/device/commands/", "http://X.X.X.X:8080/device/reports/", "http://X.X.X.X:8080/ping/" \
  }

#define TIME_API_URL "http://X.X.X.X:8080/device/time/"

#define DEFAULT_CENTER_PARAMS                                                       \
  {                                                                                 \
    DEFAULT_KEY, { true, DEFAULT_CENTER_ADDRESS, "+123456789", true, "", true, "" } \
  }

#define DEFAULT_GENERAL_PARAMS                                                                        \
  {                                                                                                   \
    {SEND_RB_INTERVAL, GET_CC_INTERVAL, SEND_RB_SMS_INTERVAL}, {1, 1, 1, 1, 1, 1, 1}, CONNECTION_TYPE \
  }

#define RESET_STR "RESET"
#define SET_TIME_STR "SETTIME"
#define USSD_STR "USSD"
#define USSD_MAX_LENGTH 512
#define ERASE_SD_STR "ERASESD"
#define READ_IMSI_STR "IMSI"
#define ACK_KEY "PARAMS"
#define ACK_OK "OK"
#define ACK_ERR "ERR"
#define SAMPLE_FLAG (1UL << 0)

// write reports interval
// minimum is 1 (seconds)
#define REPORTS_WRITE_INTERVAL 1
// send reports counter (based on write interval)
#define REPORTS_SEND_COUNTER 300

#define MAX_BUFFSIZE PROTO_MAX_BUF_SIZE //maximum b64 buffersize

// max timeout for init if timer reached and init failed it would reboot system
#define INIT_TIMEOUT 180

#define ERR_WAIT 5 //wait error for led indiactor

#define MODEM_PARAMS_INTERVAL 900 //set wait time to get modem params like sim balance,signal quality,etc (s)
#define SD_LOG_INTERVAL 1         // interval for sd logs

//modem sync time interval
#define MODEM_SYNC_TIME_INTERVAL 3600

//online communication ratio calculate based on GET_CC_INTERVAL/SEND_RB_INTERVAL
#define GET_CC_INTERVAL 5  //get center commands interval
#define SEND_RB_INTERVAL 5 //send devicedata interval

#define SEND_RB_SMS_INTERVAL 60

#define INIT_MAX_RETRIES 20 //max retries to check connection to server

// max retries that ends to a reboot (a Fix for CME ERROR 3815)
#define COMMUNICATE_MAX_FAILS 10

// switch to SMS on Max failed request
#define SWITCH_ON_FAILURE

#define GET_GPS_COUNT 5       //multiplier to get average on each get_gps_data_optimal
#define GET_GPS_DURATION 512  //duration for feed to get gps data (ms)
#define MAX_HDOP_THRESHOLD 20 //max acceptable hdop

// minimum sleep time allowed in seconds
#define SLEEP_THRESHOLD 60

//definitions for threads stack size

#define UPDATE_DEVICEDATA_THREAD_STACK 1296
#define COMMUNICATE_THREAD_STACK 3584
#define GPS_THREAD_STACK 2048
#define REPORTS_THREAD_STACK 3584
// #define CHECK_MEMORY_THREAD_STACK 5192
/*
typedef struct
{
  std::string device_data_url;
  std::string device_report_url;
  std::string center_commands_url;
  std::string ping_url;
  std::string number1;
  std::string number2;
  std::string number3;
} ServerParams;
*/
// param for sleep
typedef struct
{
  int64_t start;
  int64_t end;
} SleepStruct;

typedef struct
{
  float capacity;
  int plugged;
} BatteryStruct;

// class that contains each pin names
class DevicePins
{
public:
  static PinName modem_tx;
  static PinName modem_rx;
  static PinName modem_gps_tx;
  static PinName modem_gps_rx;
  static PinName modem_power;
  static PinName modem_switch;
  static PinName I2C_SDA;
  static PinName I2C_SCL;
  static PinName relay;
  static PinName input_sensor;
  static PinName battery_data;
  static PinName plugged;
  static PinName status_error;
  static PinName status_network;
  static PinName status_gps;
  static PinName sd_mosi;
  static PinName sd_miso;
  static PinName sd_clk;
  static PinName sd_cs;
};

class DeviceMainParams
{
public:
  DeviceMainParams(void);
  CenterCommands_set_center_params_struct center_params;
  device_params_struct general_params;
  int relay;
};

class Peripherals
{
public:
  Peripherals(void);
  BatteryStruct read_battery(void);
  void set_relay(bool value);
  bool read_input(void);
  time_t current_time(void);
  void set_error(bool value);
  void set_network(bool value);
  void set_gps(bool value);
  void show_err(int count);

private:
  float read_battery_cap(void);
  DevicePins devicepins;
  DigitalOut _relay;
  DigitalIn _input;
  AnalogIn _battery;
  DigitalIn _plugged;
  DigitalOut _network;
  DigitalOut _gps;
  DigitalOut _error;
};

class DeviceCore
{
public:
  DeviceCore(void);
  ~DeviceCore(void);
  // force reset system
  void reboot(void);

  time_t current_time_api(void);

  int init(void);
  void run(void);
  // fill devicedata with latest sensor values and IOs
  void update_devicedata(void);
  // get latest center commands from server
  void get_center_commands(void);
  // send devicedata to server
  void send_device_data(void);

  //thread for handling reports
  void reports_handler(void);
  static void print_memory_info(void);
  static void system_check();
  static mbed_error_ctx error_ctx;

private:
  void sync_time(void);
  void test();
  bool upload_report(const char *file_path);
  void get_gps_data(void);
  void get_modem_params(void);
  void status_blink(void);
  // retrieve fixed params from files in modem
  void retrieve_params(void);
  // write center params
  bool write_center_params(DeviceMainParams *deviceparams);
  int read_center_params(DeviceMainParams *deviceparams);

  //sleep functions
  void set_sleep();
  void sleep_helper(int64_t seconds);
  static void clock_source(void);
  static void rtc_clock_source(void);
  static void clock_check(void);
  static void check_error();
  static mbed_fault_context_t fault_ctx;

  gps_data _gps_data;
  //gps location per second
  gps_data *current_location;

  DeviceMainParams deviceparams;
  DeviceProto deviceproto;
  static DevicePins devicepins;
  static Modem modem;
  static Connection connection;
  static Peripherals peripherals;
  static IMU imu;
  static SDCard sd;
  static DeviceReports devicereports;
  Utility utility;

  // Thread for updating device_data
  Thread update_devicedata_thread;
  // enabler for update_devicedata_thread
  bool update_devicedata_thread_en;

  Thread communication_thread;

  // a signal that controls threads trying to access modem
  // Thread for updating gps_data
  Thread update_gps_data_thread;
  bool update_gps_data_thread_en;

  //Thread for reports_handler
  Thread reports_handler_thread;
  bool reports_handler_thread_en;

  // Thread for status blink
  LowPowerTicker status_thicker;
  bool network_led_status;

#ifdef DEBUG_MEM
  // Thread for debug memory
  Thread debug_memory_thread;
  bool debug_mem_thread_en;
#endif

  void debug_memory(void);

  // check failed requests to reboot
  void check_failed_requests(void);
  // sleep flag event
  EventFlags sleep_event;
  SleepStruct sleep_params;

  Mutex modem_ready_mutex;

  LowPowerTimeout init_timeout;
  void init_timeout_func(void);
  bool init_success;

  int signal_quality;
  int sim_balance;

  bool sd_ready;
  bool imu_ready;
  EventQueue *modem_queue;
  int fail_counter;
  DeviceData _devicedata;
};

#endif
