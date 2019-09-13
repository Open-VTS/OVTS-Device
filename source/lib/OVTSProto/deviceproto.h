/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Device Proto Library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#ifndef deviceproto_h
#define deviceproto_h

#include "mbed.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "OVTSProto.pb.h"


#undef printf
#define printf(a, ...) ser.printf(a, ##__VA_ARGS__)
extern Serial ser; // tx, rx

#define PROTO_MAX_BUF_SIZE 512

typedef struct
{
    unsigned char msg[PROTO_MAX_BUF_SIZE];
    int length;
} msg_encoded;

extern msg_encoded msg_encoded_default;

class DeviceProto
{
public:
    bool devicedata_encode(DeviceData *devicedata, msg_encoded *output);
    bool devicedata_decode(msg_encoded *input, DeviceData *output);
    bool center_commands_encode(CenterCommands *center_commands, msg_encoded *output);
    bool center_commands_decode(msg_encoded *input, CenterCommands *output);

private:
    void devicedata_en_tag(DeviceData *input);
    void center_commands_tag(CenterCommands *input);
    

};


#endif