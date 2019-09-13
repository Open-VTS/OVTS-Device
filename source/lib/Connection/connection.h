/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Connection library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*  Connection
*/
#ifndef connection_h
#define connection_h
#include "mbed.h"
#include "string"
#include "modem.h"
#include "deviceproto.h"
#include "utility.h"

// max retires to send reports to server if failed
#define REPORTS_MAX_RETRIES 2
#define CHECK_CONNECTION_TIMEOUT 5000
#define CONNECTION_REQUEST_TIMEOUT 15000

#define HTTP_DATA_PREFIX "data="          //data key for post request
#define HTTP_DeviceID_PREFIX "device_id=" //deviceid key for post request
#define HTTP_OK_RESPOND "\"T0s=\""
#define HTTP_NOT_FOUND "Tm90Rm91bmQ="
#define HTTP_SAVE_ERROR "U2F2ZUVycm9y"
#define HTTP_PING "UElORw=="

#define MAX_BUFFSIZE 512 //maximum b64 buffersize

class Connection
{
public:
  Connection(Modem *modem, const int device_id);
  int start(void);
  int check_connection(const char *url, int timeout = CHECK_CONNECTION_TIMEOUT);
  int send_device_data(const char *url, const char *number, int connection_type, DeviceData *device_data, int timeout = CONNECTION_REQUEST_TIMEOUT);
  int upload_report(const char *url, unsigned char *base64_file, long size, int timeout = CONNECTION_REQUEST_TIMEOUT);
  int get_center_commands(const char *url, const char *number, int connection_type, CenterCommands *center_command, int timeout = CONNECTION_REQUEST_TIMEOUT);

private:
  bool devicedata_generator(DeviceData *device_data, char *output);
  int send_device_data_http(const char *url, const char *body, int timeout);
  int send_device_data_sms(const char *number, const char *body);
  std::string get_center_commands_http(const char *url, int timeout);
  std::string get_center_commands_sms(const char *number);
  Modem *_modem;

  DeviceProto _deviceproto;
  // std::string contains base http request body
  char _base_request_body[20];
};

#endif
