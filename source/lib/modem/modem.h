/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Quectel MC60 library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#ifndef modem_h
#define modem_h

//set if use standalone mode
#define STAND_ALONE

#include "mbed.h"
#include "string"
#include "vector"
#include "TinyGPS++.h"
#include "platform\ATCmdParser.h"
#include "drivers\UARTSerial.h"

// set minimum chunk to post large bodies
#define HTTP_POST_SIZE_LIMIT 512
// HTTP chunk size for large buffers
#define HTTP_POST_CHUNK 256
#define CHUNK_DELAY 512
#define receive_timeout 10  //timeout for receiving answer from modem_serial module seconds
#define max_buffer_size 512 //max buffer size for reading modem serial
#define default_timeout 2000
#define MODEM_ERR "ERROR"
#define MODEM_NETWORK_ERROR "+CME ERROR"
#define MODEM_SMS_ERROR "+CMS ERROR"

//irancell
#define OP_MTN "MTN Irancell"
#define OP_MTN_APN "mtnirancell"
#define OP_MTN_CUSTOM_APN "myCustomAPN"

// mci
#define OP_MCI "MCI"
#define OP_MCI_APN "mcinet"
#define OP_MCI_CUSTOM_APN ""

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
    bool set_apn(void);
    // this command wait for modem until get sms ready
    bool init_modem(void);
    bool set_operator(void);

    //check gsm network status
    bool check_gsm_status(void);
    bool init_network(void);
    bool init_network_qiact(int timeout = 5000);
    bool init_network_qideact(int timeout = 10000);
    int get_signal_quality(void);
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

    std::string read_until(const char *value, int timeout);
    inline std::string read_until() { return read_until("OK", 1000); }
    inline std::string read_until(const char *value) { return read_until(value, 1000); }

    bool check_for_respond(const char *resp, int timeout);
    inline bool check_for_respond(const char *resp) { return check_for_respond(resp, 1000); }
    inline bool check_for_respond() { return check_for_respond("OK", 1000); }

    bool check_for_respond_fragmented(const char *resp, int timeout);
    int read_until_forever(const char *value, const char *error = MODEM_NETWORK_ERROR, float read_until_forever_timeout = 180.0f);

    // get gsm time
    time_t current_time(void);

//Stand Alone
#ifdef STAND_ALONE
    std::string readSerialGPS(void);
    std::string read_until_GPS(const char *value, int timeout);
#endif
private:
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
