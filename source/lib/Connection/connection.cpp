/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Connection library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*  Connection
*/
#include "connection.h"

Connection::Connection(Modem *modem, const int device_id) : _modem(modem)
{
    char *temp = new char[10];
    sprintf(temp, "%d", device_id);
    strcat(_base_request_body, HTTP_DEVICEID_PREFIX);
    strcat(_base_request_body, temp);
    delete[] temp;
}

std::string Connection::get_center_commands_sms(const char *number)
{
    std::string result = "";
    int *new_msg_nums = new int[16]();
    if (_modem->check_message(new_msg_nums, 16) > 0)
    {
        for (int i = 15; i >= 0; i--)
        {
            if (new_msg_nums[i] != 0)
            {
                double_string msg = _modem->read_sms(new_msg_nums[i]);
                // delete sms
                _modem->del_sms(new_msg_nums[i]);
                printf("#: %d, Number: %s, Message: %s\r\n", new_msg_nums[i], msg.a.c_str(), msg.b.c_str());
                // check if number is equal
                if (msg.a == number)
                {
                    result = msg.b;
                    break;
                }
            }
        }
    }
    delete[] new_msg_nums;
    return result;
}

std::string Connection::get_center_commands_http(const char *url, int timeout)
{
    return _modem->http_post(url, _base_request_body, strlen(_base_request_body), timeout);
}

int Connection::get_center_commands(const char *url, const char *number, int connection_type, CenterCommands *center_command, int timeout)
{
    std::string respond = "";
    // choose type of connection
    // 1:Network, 2:SMS, 3:Both
    switch (connection_type)
    {
    case 1:
        if (strcmp(url, "") != 0)
            respond = get_center_commands_http(url, timeout);
        break;
    case 2:
        if (strcmp(number, "") != 0)
        {
            respond = get_center_commands_sms(number);
            // if respond is empty fill it as no commands
            if (respond.empty())
                return -1;
        }
        break;
    case 3:
        if (strcmp(number, "") != 0)
        {
            respond = get_center_commands_sms(number);
            if (!respond.empty())
                break;
        }
        if (strcmp(url, "") != 0)
            respond = get_center_commands_http(url, timeout);
        break;
    default:
        break;
    }

    // std::string respond = _modem->http_post(url, _base_request_body, strlen(_base_request_body), timeout);
    if (respond.empty())
    {
        return 0;
    }

    respond = Utility::read_between_quotes(respond);
    if ((respond == HTTP_NOT_FOUND) || (respond == HTTP_SAVE_ERROR))
        return -1;
    // printf("RESPOND: %s\r\n", respond.c_str());

    msg_encoded encoded_data = msg_encoded_default;
    encoded_data.length = sizeof(encoded_data.msg);
    if (!(Base64::decode(encoded_data.msg, encoded_data.length, reinterpret_cast<const unsigned char *>(respond.c_str()))))
    {
        // printf("58\r\n");
        return 0;
    }
#if 0
    printf("encoded_data.msg: ");
    for (int i = 0; i < sizeof(encoded_data.msg); i++)
    {
        printf("%c", encoded_data.msg[i]);
    }
    printf("\r\n");
#endif
    // strcpy((char*) encoded_data.msg, respond.c_str());

    // std::string encoded_data_str(reinterpret_cast<char const *>(encoded_data.msg), sizeof(encoded_data.msg));
    // encoded_data.length = encoded_data_str.length();

    // printf("length: %d\r\n", encoded_data.length);
    int result = _deviceproto.center_commands_decode(&encoded_data, center_command);
    if (result)
    {
        // printf("CenterCommands Decoded\r\n");
        // printf("CenterCommands ID: %d\r\n", center_command->id);
        return 1;
    }
    else
    {
        printf("CenterCommands Decode Failed\r\n");
    }
    return 0;
}

//generate proper body for devicedata
bool Connection::devicedata_generator(DeviceData *device_data, char *output)
{
    bool result = false;
    //encode devicedata
    msg_encoded encoded_data = msg_encoded_default;
    // unsigned char b64_buffer[MAX_BUFFSIZE];
    unsigned char *b64_buffer = new unsigned char[MAX_BUFFSIZE]();
    // add deviceid
    strcat(output, _base_request_body);
    strcat(output, "&");

    result = _deviceproto.devicedata_encode(device_data, &encoded_data);
    if (result)
    {
        //prepare b64 from device_data
        if (Base64::encode(b64_buffer, MAX_BUFFSIZE, encoded_data.msg, encoded_data.length))
        {
            // printf("b64_buffer: %s, b64_buffer: %d\r\n", b64_buffer, strlen(reinterpret_cast<const char*>((b64_buffer))));
            strcat(output, HTTP_DATA_PREFIX);
            strcat(output, reinterpret_cast<const char *>((b64_buffer)));
            // printf("output: %s, size: %d\r\n", output, strlen(reinterpret_cast<const char*>((output))));
            result = true;
        }
    }
    delete[] b64_buffer;
    return result;
}

int Connection::send_device_data_sms(const char *number, const char *body)
{
    if (_modem->send_sms(number, body))
        return 1;
    return 0;
}

int Connection::send_device_data_http(const char *url, const char *body, int timeout)
{
    std::string respond = _modem->http_post(url, body, strlen(body), timeout);
    // printf("respond: %s\r\n", respond.c_str());
    if (respond == HTTP_OK_RESPOND)
        return 1;
    else
    {
        printf("Failed Response: %s\r\n", respond.c_str());
        return 0;
    }
}

int Connection::send_device_data(const char *url, const char *number, int connection_type, DeviceData *device_data, int timeout)
{
    int result = 0;
    char *body = new char[MAX_BUFFSIZE]();
    if (devicedata_generator(device_data, body))
    {
        // printf("body: %s\r\n", body);
        // printf("body_length: %d\r\n", strlen(body));
        // choose type of connection
        // 1:Network, 2:SMS, 3:Both
        switch (connection_type)
        {
        case 1:
        {
            if (strcmp(url, "") != 0)
                result = send_device_data_http(url, body, timeout);
            break;
        }
        case 2:
        {
            if (strcmp(number, "") != 0)
                result = send_device_data_sms(number, body);
            break;
        }
        case 3:
        {
            //TODO: fill proper value for both connection mode
            int res1 = 0, res2 = 0;
            if (strcmp(url, "") != 0)
                res1 = send_device_data_http(url, body, timeout);
            if (strcmp(number, "") != 0)
                res2 = send_device_data_sms(number, body);
            // return smallest value
            result = res1 < res2 ? res1 : res2;
            break;
        }
        default:
            break;
        }
    }
    delete[] body;
    return result;
}

int Connection::upload_report(const char *url, unsigned char *base64_file, int timeout)
{
    int result = 0;
    char *temp = new char[32]();
    strcat(temp, _base_request_body);
    strcat(temp, (const char *)"&file=\"");
    Utility::str_prepend(base64_file, temp);
    strcat((char *)base64_file, (const char *)"\"");
    // printf("body: %s\r\n", base64_file);
    // printf("body_length: %d\r\n", strlen((const char*)base64_file));
    for (int i = 0; i < REPORTS_MAX_RETRIES; i++)
    {
        std::string respond = _modem->http_post(url, (const char *)base64_file, strlen((const char *)base64_file), timeout);
        // printf("upload respond: %s\r\n", respond.c_str());
        if (respond.find(HTTP_OK_RESPOND) != std::string::npos)
        {
            result = 1;
            break;
        }
    }
    delete[] temp;
    return result;
}

int Connection::check_connection(const char *url, int timeout)
{
    int result = 0;
    char *body = new char[64]();
    strcat(body, _base_request_body);
    strcat(body, "&");
    strcat(body, HTTP_DATA_PREFIX);
    strcat(body, HTTP_PING);
    // printf("PING url: %s, Body: %s\r\n", url, body);
    // std::string respond = _modem->http_post(url, body, strlen(body), timeout);
    std::string respond = _modem->http_get(url, timeout);
    // printf("PING Respond: %s\r\n------\r\n", respond.c_str());
    if (respond == HTTP_OK_RESPOND)
        result = 1;
    else
        result = 0;
    delete[] body;
    return result;
}