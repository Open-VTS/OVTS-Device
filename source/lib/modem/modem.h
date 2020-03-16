/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Quectel MC60 library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#ifndef modem_h
#define modem_h

// set if use standalone mode
#define STAND_ALONE

#include "mbed.h"
#include "string"
#include "TinyGPS++.h"
#include "platform\ATCmdParser.h"
#include "drivers\UARTSerial.h"
#include "vector"

// set to use https instead of http
// #define USE_HTTPS
// set minimum chunk to post large bodies
#define HTTP_POST_SIZE_LIMIT 512
// HTTP chunk size for large buffers
#define HTTP_POST_CHUNK 256
#define CHUNK_DELAY 512
#define max_buffer_size 512       // max buffer size for reading modem serial
#define wait_response wait_ms(30) // maximum wait time for respond a command
#define parser_default_timeout 1500
#define MODEM_ERR "ERROR"
#define MODEM_NETWORK_ERROR "+CME ERROR"
#define MODEM_SMS_ERROR "+CMS ERROR"

// Irancell
#define OP_MTN "MTN Irancell"
#define OP_MTN_APN "mtnirancell"
#define OP_MTN_CUSTOM_APN "myCustomAPN"

// IR MCI
#define OP_MCI "MCI"
#define OP_MCI_APN "mcinet"
#define OP_MCI_CUSTOM_APN ""

// OK resp
#define OK "OK\r\n"

// OP undefined
#define OP_ERR ""

typedef struct
{
    std::string a;
    std::string b;
} double_string;

typedef struct
{
    double lat;
    double lng;
    double speed;
    double altitude;
    double course;
    double hdop;
} gps_data;

class Modem
{
public:
#ifdef STAND_ALONE
    Modem(PinName Tx, PinName Rx, PinName GPS_Tx, PinName GPS_Rx, PinName pw, PinName sw, const double hdop_threshold);
#else
    Modem(PinName Tx, PinName Rx, PinName power, PinName sw, const float hdop_threshold);
#endif
    ~Modem();
    void power_on(void);
    void power_off(void);
    void reboot(void);
    void soft_reboot(void);
    std::string readSerial(void);
    void flush(void);
    bool set_params(void);
    double_string read_sms(int index);
    bool check_inbox_full(int threshold = 10);
    int check_message(int *output, int size);
    bool ping(void);
    bool del_sms_all(void);
    bool del_sms(int index);
    bool send_sms(const char *number, const char *msg);
    std::string ussd(const char *code, int timeout = 5000);
    int balance(void);
    std::string read_imsi(void);

    bool set_apn(void);
    // this command wait for modem until get sms ready
    bool init(void);
    bool reinit(void);
    bool set_operator(void);

    //check gsm network status
    bool check_gsm_status();
    bool check_sim_status(void);
    bool init_network(void);
    bool init_network_qideact(int timeout = 15000);
    int get_signal_quality(void);
    bool config_https(void);
    //HTTP
    std::string http_get(const char *url, int timeout);
    std::string http_post(const char *url, const char *body, int blen, int timeout);

    //File
    bool upload_file(const char *name, int size, const char *data);
    std::string download_file(const char *name);
    bool delete_file(const char *name);
    bool file_exist(const char *name);

    //GPS
    bool gps_turn_on(void);
    bool gps_turn_off(void);
    gps_data get_gps_data(int timeout);
    gps_data get_gps_data_fix(gps_data *loc = NULL);
    std::string get_nmea(void);

    std::string read_until(const char *value = OK, int timeout = 1000);
    int check_for_respond_w_err(const char *resp = OK, const char *error = MODEM_ERR, int timeout = 2000);
    bool check_for_respond(const char *resp = OK, int timeut = 2000);
    bool check_for_respond(int timeout) { return check_for_respond(OK, timeout); }
    void test(void);
    void test_socket(void);
    // get gsm time
    time_t current_time(void);

//Stand Alone
#ifdef STAND_ALONE
    std::string readSerialGPS(void);
    std::string read_until_GPS(const char *value, int timeout);
#endif
private:
    bool hasEnding(std::string const &fullString, std::string const &ending);
    // current operator name
    // maximum value for acceptable hdop
    double _hdop_threshold;
    std::string _current_op;
    UARTSerial modem_serial;
    ATCmdParser parser;
    TinyGPSPlus gps;
#ifdef STAND_ALONE
    FILE *gps_stream;
    UARTSerial gps_serial;
#endif
    DigitalOut _power;
    DigitalOut _switch;
    void _strInput(char str[], int nchars);
    double speed_calc(std::vector<double> speeds);
    bool IsDST(int month, int day);
    time_t time_convert(time_t t0, char const *tz_value);
};

#endif
