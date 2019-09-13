/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Device Proto Library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#include "deviceproto.h"

Serial ser(PA_9, PA_10); // tx, rx


msg_encoded msg_encoded_default = {{0}, 0};

// This function set has_field to true on each element if it is exist
// we only check a single element from submessages
//TODO: If a field added it should be checked on this function

void DeviceProto::devicedata_en_tag(DeviceData *input)
{
    // DeviceData devicedata = input;

    //check loaction:
    if (input->gps_data.lat != 0 || input->gps_data.lng != 0 ||
        input-> gps_data.speed != 0 || input-> gps_data.altitude != 0 ||
        input-> gps_data.course !=0 || input -> gps_data.hdop !=0)
        input->has_gps_data = true;
    // imu
    if (input->imu_data.ax != 0 || input->imu_data.ay != 0 || input->imu_data.az != 0)
        input->has_imu_data = true;
    // relay
    if (input->relay != 0)
        input->has_relay = true;
    // input
    if (input->input_sensor != 0)
        input->has_input_sensor = true;
    // charge
    if (input->sim_balance != 0)
        input->has_sim_balance = true;
    // battery_data
    if (input->battery_data.capacity != 0.0 || input->battery_data.plugged != 0)
        input->has_battery_data = true;
    // signal quality
    if (input->signal_quality != 0)
        input->has_signal_quality = true;
    // center params
    if (strcmp(input->center_params.number1, ""))
        input->has_center_params = true;
    // center params center address
    if (strcmp(input->center_params.center_address.device_data_url, "") ||
        strcmp(input->center_params.center_address.center_commands_url, "") ||
        strcmp(input->center_params.center_address.ping_url, ""))
        input->center_params.has_center_address = true;
    // center params center number2
    if (strcmp(input->center_params.number2, ""))
        input->center_params.has_number2 = true;
    // center params center number3
    if (strcmp(input->center_params.number3, ""))
        input->center_params.has_number3 = true;
    // device_params
    if (input->device_params.connection_type != 0)
        input->has_device_params = true;
}

bool DeviceProto::devicedata_encode(DeviceData *devicedata, msg_encoded *output)
{

    //Main encoder field
    devicedata_en_tag(devicedata);

    // msg_encoded output;

    // pb_ostream_t stream = pb_ostream_from_buffer(output->msg, sizeof(output->msg));
    pb_ostream_t stream = pb_ostream_from_buffer(output->msg, PROTO_MAX_BUF_SIZE);

    int custom_field_size = 0;
    int custom_field_step = sizeof(devicedata->custom_field) / sizeof(devicedata->custom_field[0]);
    for (int i = 0; i < custom_field_step; i++)
    {
        if (strcmp(devicedata->custom_field[i].key, "") || strcmp(devicedata->custom_field[i].value, ""))
        {
            custom_field_size++;
            // manual way for repeated fields
            /*
            //This is only for repeated fields
            if (!pb_encode_tag(&stream, PB_WT_STRING, DeviceData_custom_field_tag))
                return false;

            if (!pb_encode_submessage(&stream, custom_struct_fields, &devicedata->custom_field[0]))
            {
                printf("pb_encode_submessage Failed: %s\n", PB_GET_ERROR(&stream));
                return false;
            }
            //...
            */

        }
    }
    //setting param for custom_field (repeated field)
    devicedata->custom_field_count = custom_field_size;

    if (!pb_encode(&stream, DeviceData_fields, devicedata))
    {
        printf("pb_encode Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    output->length = stream.bytes_written;
    // printf("buffer2: ");
    // for (int i = 0; i < output->length; i++)
    // {
    //     if (output->msg[i])
    //         printf("%d, ", output->msg[i]);
    // }
    // printf("\nmessage_length: %d\n", output->length);
    return true;
}

bool DeviceProto::devicedata_decode(msg_encoded *input, DeviceData *output){
    pb_istream_t stream = pb_istream_from_buffer(input->msg, input->length);
    if (!pb_decode(&stream, DeviceData_fields, output))
    {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    return true;
}

void DeviceProto::center_commands_tag(CenterCommands *input){
    // device_params
    if (input->device_params.connection_type != 0)
        input->has_device_params = true;

    // center_params
    if (strcmp(input->set_center_params.key, ""))
        input->has_set_center_params = true;

    // center_address
    if (strcmp(input->set_center_params.center_params.center_address.ping_url, ""))
        input->set_center_params.center_params.has_center_address = true;

    // number2
    if (strcmp(input->set_center_params.center_params.number2, ""))
        input->set_center_params.center_params.has_number2 = true;
    //number3
    if (strcmp(input->set_center_params.center_params.number3, ""))
        input->set_center_params.center_params.has_number3 = true;

    // device report
    if (input->device_report.start !=0 || input->device_report.end != 0)
        input->has_device_report = true;
    
    // device sleep
    if (input->device_sleep.start !=0 || input->device_sleep.end != 0)
        input->has_device_sleep = true;
    
    // relay
    if (input->relay != 0)
        input->has_relay = true;
}


// Encode center commands
bool DeviceProto::center_commands_encode(CenterCommands *center_commands, msg_encoded *output)
{
    center_commands_tag(center_commands);

    // msg_encoded output;

    pb_ostream_t stream = pb_ostream_from_buffer(output->msg, sizeof(output->msg));

    int custom_field_size = 0;
    int custom_field_step = sizeof(center_commands->custom_field) / sizeof(center_commands->custom_field[0]);
    for (int i = 0; i < custom_field_step; i++)
    {
        if (strcmp(center_commands->custom_field[i].key, "") || strcmp(center_commands->custom_field[i].value, ""))
        {
            custom_field_size++;
        }
    }
    //setting param for custom_field (repeated field)
    center_commands->custom_field_count = custom_field_size;

    if (!pb_encode(&stream, CenterCommands_fields, center_commands))
    {
        printf("pb_encode Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    output->length = stream.bytes_written;
    // printf("buffer2: ");
    // for (int i = 0; i < output->length; i++)
    // {
    //     if (output->msg[i])
    //         printf("%d, ", output->msg[i]);
    // }
    // printf("\nmessage_length: %d\n", output->length);
    return true;
}

bool DeviceProto::center_commands_decode(msg_encoded *input, CenterCommands *output){
    pb_istream_t stream = pb_istream_from_buffer(input->msg, input->length);
    if (!pb_decode(&stream, CenterCommands_fields, output))
    {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream));
        return false;
    }
    return true;
}