/* Automatically generated nanopb header */
/* Generated by nanopb-0.3.9.2 at Wed Aug 28 23:37:53 2019. */

#ifndef PB_OVTSPROTO_PB_H_INCLUDED
#define PB_OVTSPROTO_PB_H_INCLUDED
#include <pb.h>

/* @@protoc_insertion_point(includes) */
#if PB_PROTO_HEADER_VERSION != 30
#error Regenerate this file with the current version of nanopb generator.
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Struct definitions */
typedef struct _CenterCommands_report_struct {
    int64_t start;
    int64_t end;
/* @@protoc_insertion_point(struct:CenterCommands_report_struct) */
} CenterCommands_report_struct;

typedef struct _CenterCommands_sleep_struct {
    int64_t start;
    int64_t end;
/* @@protoc_insertion_point(struct:CenterCommands_sleep_struct) */
} CenterCommands_sleep_struct;

typedef struct _DeviceData_battery_struct {
    float capacity;
    int32_t plugged;
/* @@protoc_insertion_point(struct:DeviceData_battery_struct) */
} DeviceData_battery_struct;

typedef struct _DeviceData_gps_data_struct {
    float lat;
    float lng;
    double speed;
    double altitude;
    double course;
    double hdop;
/* @@protoc_insertion_point(struct:DeviceData_gps_data_struct) */
} DeviceData_gps_data_struct;

typedef struct _DeviceData_imu_data_struct {
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float mx;
    float my;
    float mz;
/* @@protoc_insertion_point(struct:DeviceData_imu_data_struct) */
} DeviceData_imu_data_struct;

typedef struct _center_address_struct {
    char device_data_url[128];
    char center_commands_url[128];
    char device_report_url[128];
    char ping_url[128];
/* @@protoc_insertion_point(struct:center_address_struct) */
} center_address_struct;

typedef struct _custom_struct {
    char key[64];
    char value[256];
/* @@protoc_insertion_point(struct:custom_struct) */
} custom_struct;

typedef struct _device_data_select_struct {
    int32_t gps_enable;
    int32_t imu_enable;
    int32_t relay_enable;
    int32_t input_sensor_enable;
    int32_t sim_balance_enable;
    int32_t battery_enable;
    int32_t signal_quality_enable;
/* @@protoc_insertion_point(struct:device_data_select_struct) */
} device_data_select_struct;

typedef struct _device_timer_struct {
    int32_t network_send;
    int32_t network_receive;
    int32_t sms_send;
/* @@protoc_insertion_point(struct:device_timer_struct) */
} device_timer_struct;

typedef struct _center_params_struct {
    bool has_center_address;
    center_address_struct center_address;
    char number1[16];
    bool has_number2;
    char number2[16];
    bool has_number3;
    char number3[16];
/* @@protoc_insertion_point(struct:center_params_struct) */
} center_params_struct;

typedef struct _device_params_struct {
    device_timer_struct timer;
    device_data_select_struct data_select;
    int32_t connection_type;
/* @@protoc_insertion_point(struct:device_params_struct) */
} device_params_struct;

typedef struct _CenterCommands_set_center_params_struct {
    char key[128];
    center_params_struct center_params;
/* @@protoc_insertion_point(struct:CenterCommands_set_center_params_struct) */
} CenterCommands_set_center_params_struct;

typedef struct _DeviceData {
    int32_t device_id;
    int64_t time;
    bool has_gps_data;
    DeviceData_gps_data_struct gps_data;
    bool has_imu_data;
    DeviceData_imu_data_struct imu_data;
    bool has_relay;
    int32_t relay;
    bool has_input_sensor;
    int32_t input_sensor;
    bool has_sim_balance;
    int32_t sim_balance;
    bool has_battery_data;
    DeviceData_battery_struct battery_data;
    bool has_signal_quality;
    int32_t signal_quality;
    bool has_center_params;
    center_params_struct center_params;
    bool has_device_params;
    device_params_struct device_params;
    pb_size_t custom_field_count;
    custom_struct custom_field[10];
/* @@protoc_insertion_point(struct:DeviceData) */
} DeviceData;

typedef struct _CenterCommands {
    int32_t device_id;
    bool has_set_center_params;
    CenterCommands_set_center_params_struct set_center_params;
    bool has_device_params;
    device_params_struct device_params;
    bool has_device_report;
    CenterCommands_report_struct device_report;
    bool has_device_sleep;
    CenterCommands_sleep_struct device_sleep;
    bool has_relay;
    int32_t relay;
    pb_size_t custom_field_count;
    custom_struct custom_field[10];
/* @@protoc_insertion_point(struct:CenterCommands) */
} CenterCommands;

/* Default values for struct fields */

/* Initializer values for message structs */
#define custom_struct_init_default               {"", ""}
#define device_timer_struct_init_default         {0, 0, 0}
#define device_data_select_struct_init_default   {0, 0, 0, 0, 0, 0, 0}
#define device_params_struct_init_default        {device_timer_struct_init_default, device_data_select_struct_init_default, 0}
#define center_address_struct_init_default       {"", "", "", ""}
#define center_params_struct_init_default        {false, center_address_struct_init_default, "", false, "", false, ""}
#define DeviceData_init_default                  {0, 0, false, DeviceData_gps_data_struct_init_default, false, DeviceData_imu_data_struct_init_default, false, 0, false, 0, false, 0, false, DeviceData_battery_struct_init_default, false, 0, false, center_params_struct_init_default, false, device_params_struct_init_default, 0, {custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default}}
#define DeviceData_imu_data_struct_init_default  {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define DeviceData_gps_data_struct_init_default  {0, 0, 0, 0, 0, 0}
#define DeviceData_battery_struct_init_default   {0, 0}
#define CenterCommands_init_default              {0, false, CenterCommands_set_center_params_struct_init_default, false, device_params_struct_init_default, false, CenterCommands_report_struct_init_default, false, CenterCommands_sleep_struct_init_default, false, 0, 0, {custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default, custom_struct_init_default}}
#define CenterCommands_report_struct_init_default {0, 0}
#define CenterCommands_sleep_struct_init_default {0, 0}
#define CenterCommands_set_center_params_struct_init_default {"", center_params_struct_init_default}
#define custom_struct_init_zero                  {"", ""}
#define device_timer_struct_init_zero            {0, 0, 0}
#define device_data_select_struct_init_zero      {0, 0, 0, 0, 0, 0, 0}
#define device_params_struct_init_zero           {device_timer_struct_init_zero, device_data_select_struct_init_zero, 0}
#define center_address_struct_init_zero          {"", "", "", ""}
#define center_params_struct_init_zero           {false, center_address_struct_init_zero, "", false, "", false, ""}
#define DeviceData_init_zero                     {0, 0, false, DeviceData_gps_data_struct_init_zero, false, DeviceData_imu_data_struct_init_zero, false, 0, false, 0, false, 0, false, DeviceData_battery_struct_init_zero, false, 0, false, center_params_struct_init_zero, false, device_params_struct_init_zero, 0, {custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero}}
#define DeviceData_imu_data_struct_init_zero     {0, 0, 0, 0, 0, 0, 0, 0, 0}
#define DeviceData_gps_data_struct_init_zero     {0, 0, 0, 0, 0, 0}
#define DeviceData_battery_struct_init_zero      {0, 0}
#define CenterCommands_init_zero                 {0, false, CenterCommands_set_center_params_struct_init_zero, false, device_params_struct_init_zero, false, CenterCommands_report_struct_init_zero, false, CenterCommands_sleep_struct_init_zero, false, 0, 0, {custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero, custom_struct_init_zero}}
#define CenterCommands_report_struct_init_zero   {0, 0}
#define CenterCommands_sleep_struct_init_zero    {0, 0}
#define CenterCommands_set_center_params_struct_init_zero {"", center_params_struct_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define CenterCommands_report_struct_start_tag   1
#define CenterCommands_report_struct_end_tag     2
#define CenterCommands_sleep_struct_start_tag    1
#define CenterCommands_sleep_struct_end_tag      2
#define DeviceData_battery_struct_capacity_tag   1
#define DeviceData_battery_struct_plugged_tag    2
#define DeviceData_gps_data_struct_lat_tag       1
#define DeviceData_gps_data_struct_lng_tag       2
#define DeviceData_gps_data_struct_speed_tag     3
#define DeviceData_gps_data_struct_altitude_tag  4
#define DeviceData_gps_data_struct_course_tag    5
#define DeviceData_gps_data_struct_hdop_tag      6
#define DeviceData_imu_data_struct_ax_tag        1
#define DeviceData_imu_data_struct_ay_tag        2
#define DeviceData_imu_data_struct_az_tag        3
#define DeviceData_imu_data_struct_gx_tag        4
#define DeviceData_imu_data_struct_gy_tag        5
#define DeviceData_imu_data_struct_gz_tag        6
#define DeviceData_imu_data_struct_mx_tag        7
#define DeviceData_imu_data_struct_my_tag        8
#define DeviceData_imu_data_struct_mz_tag        9
#define center_address_struct_device_data_url_tag 1
#define center_address_struct_center_commands_url_tag 2
#define center_address_struct_device_report_url_tag 3
#define center_address_struct_ping_url_tag       4
#define custom_struct_key_tag                    1
#define custom_struct_value_tag                  2
#define device_data_select_struct_gps_enable_tag 1
#define device_data_select_struct_imu_enable_tag 2
#define device_data_select_struct_relay_enable_tag 3
#define device_data_select_struct_input_sensor_enable_tag 4
#define device_data_select_struct_sim_balance_enable_tag 5
#define device_data_select_struct_battery_enable_tag 6
#define device_data_select_struct_signal_quality_enable_tag 7
#define device_timer_struct_network_send_tag     1
#define device_timer_struct_network_receive_tag  2
#define device_timer_struct_sms_send_tag         3
#define center_params_struct_center_address_tag  1
#define center_params_struct_number1_tag         2
#define center_params_struct_number2_tag         3
#define center_params_struct_number3_tag         4
#define device_params_struct_timer_tag           1
#define device_params_struct_data_select_tag     2
#define device_params_struct_connection_type_tag 3
#define CenterCommands_set_center_params_struct_key_tag 1
#define CenterCommands_set_center_params_struct_center_params_tag 2
#define DeviceData_device_id_tag                 1
#define DeviceData_time_tag                      2
#define DeviceData_gps_data_tag                  3
#define DeviceData_imu_data_tag                  4
#define DeviceData_relay_tag                     5
#define DeviceData_input_sensor_tag              6
#define DeviceData_sim_balance_tag               7
#define DeviceData_battery_data_tag              8
#define DeviceData_signal_quality_tag            9
#define DeviceData_center_params_tag             10
#define DeviceData_device_params_tag             11
#define DeviceData_custom_field_tag              12
#define CenterCommands_device_id_tag             1
#define CenterCommands_set_center_params_tag     2
#define CenterCommands_device_params_tag         3
#define CenterCommands_device_report_tag         4
#define CenterCommands_device_sleep_tag          5
#define CenterCommands_relay_tag                 6
#define CenterCommands_custom_field_tag          7

/* Struct field encoding specification for nanopb */
extern const pb_field_t custom_struct_fields[3];
extern const pb_field_t device_timer_struct_fields[4];
extern const pb_field_t device_data_select_struct_fields[8];
extern const pb_field_t device_params_struct_fields[4];
extern const pb_field_t center_address_struct_fields[5];
extern const pb_field_t center_params_struct_fields[5];
extern const pb_field_t DeviceData_fields[13];
extern const pb_field_t DeviceData_imu_data_struct_fields[10];
extern const pb_field_t DeviceData_gps_data_struct_fields[7];
extern const pb_field_t DeviceData_battery_struct_fields[3];
extern const pb_field_t CenterCommands_fields[8];
extern const pb_field_t CenterCommands_report_struct_fields[3];
extern const pb_field_t CenterCommands_sleep_struct_fields[3];
extern const pb_field_t CenterCommands_set_center_params_struct_fields[3];

/* Maximum encoded size of messages (where known) */
#define custom_struct_size                       325
#define device_timer_struct_size                 33
#define device_data_select_struct_size           77
#define device_params_struct_size                125
#define center_address_struct_size               524
#define center_params_struct_size                581
#define DeviceData_size                          4170
#define DeviceData_imu_data_struct_size          45
#define DeviceData_gps_data_struct_size          46
#define DeviceData_battery_struct_size           16
#define CenterCommands_size                      4195
#define CenterCommands_report_struct_size        22
#define CenterCommands_sleep_struct_size         22
#define CenterCommands_set_center_params_struct_size 715

/* Message IDs (where set with "msgid" option) */
#ifdef PB_MSGID

#define OVTSPROTO_MESSAGES \


#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
/* @@protoc_insertion_point(eof) */

#endif
