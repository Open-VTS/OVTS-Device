/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Core library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/
#include "core.h"

PinName DevicePins::modem_tx = PA_2;
PinName DevicePins::modem_rx = PA_3;
PinName DevicePins::modem_gps_tx = PD_8;
PinName DevicePins::modem_gps_rx = PD_9;
PinName DevicePins::modem_power = PB_5;
PinName DevicePins::modem_switch = PB_9;
PinName DevicePins::I2C_SDA = PB_7;
PinName DevicePins::I2C_SCL = PB_6;
PinName DevicePins::relay = PC_2;
PinName DevicePins::input_sensor = PB_12;
PinName DevicePins::battery_data = PB_1;
PinName DevicePins::plugged = PA_8;
PinName DevicePins::status_network = PB_14;
PinName DevicePins::status_error = PB_13;
PinName DevicePins::status_gps = PB_15;
PinName DevicePins::sd_mosi = PA_7;
PinName DevicePins::sd_miso = PA_6;
PinName DevicePins::sd_clk = PA_5;
PinName DevicePins::sd_cs = PA_4;

CenterCommands_set_center_params_struct default_center_params = DEFAULT_CENTER_PARAMS;
device_params_struct default_general_params = DEFAULT_GENERAL_PARAMS;

int default_relay = -1;

DeviceMainParams::DeviceMainParams()
{
    center_params = (CenterCommands_set_center_params_struct)default_center_params;
    general_params = (device_params_struct)default_general_params;
    relay = default_relay;
}

Peripherals::Peripherals(void) : _relay(devicepins.relay), _input(devicepins.input_sensor), _battery(devicepins.battery_data), _plugged(devicepins.plugged),
                                 _network(devicepins.status_network), _gps(devicepins.status_gps), _error(devicepins.status_error)
{
    _relay = 1;
    _plugged.mode(PullNone);
    _input.mode(PullNone);
}

void Peripherals::set_error(bool value)
{
    _error = value;
}

void Peripherals::show_err(int count)
{
    bool val = true;
    for (int i = 0; i < 2 * count; i++)
    {
        set_error(val);
        val = !val;
        wait_ms(300);
    }
}

void Peripherals::set_network(bool value)
{
    _network = value;
}

void Peripherals::set_gps(bool value)
{
    _gps = value;
}

float Peripherals::read_battery_cap(void)
{
    double sum = 0;                 // sum of samples taken
    float voltage = 0.0;            // calculated voltage
    float output = 0.0;             //output value
    const float battery_max = 4.20; //maximum voltage of battery
    const float battery_min = 3.0;  //minimum voltage of battery before shutdown
    float division = 4.3;
    for (int i = 0; i < 1000; i++)
    {
        sum += _battery.read();
        wait_us(10);
    }
    voltage = sum / 1000;
    // printf("voltage_pin: %f\n", voltage);
    voltage = (voltage * 3.3);

    voltage = voltage * division; // battery = (r1+r2)/r2  * Vmeasure
    output = ((voltage - battery_min) / (battery_max - battery_min)) * 100;
    if (output > 0 && output <= 100)
        return output;
    else if (output > 100)
        return 100;
    else
        return 0;
}

BatteryStruct Peripherals::read_battery(void)
{
    BatteryStruct output;
    if (_plugged.read() > 0)
    {
        output.capacity = 100;
        output.plugged = 1;
    }
    else
    {
        float battery_percentage = read_battery_cap();
        output.capacity = roundf(battery_percentage * 100) / 100;
        output.plugged = -1;
    }
    return output;
}

void Peripherals::set_relay(bool value)
{
    _relay = !value;
}

bool Peripherals::read_input(void)
{
    if (_input.read())
        return false;
    else
        return true;
}

time_t Peripherals::current_time(void)
{
    time_t seconds = time(NULL);
    return seconds;
}

Modem DeviceCore::modem(devicepins.modem_tx, devicepins.modem_rx, devicepins.modem_gps_tx, devicepins.modem_gps_rx, devicepins.modem_power, devicepins.modem_switch, MAX_HDOP_THRESHOLD);
Connection DeviceCore::connection(&modem, DEVICEID);
IMU DeviceCore::imu(devicepins.I2C_SDA, devicepins.I2C_SCL);
SDCard DeviceCore::sd(devicepins.sd_mosi, devicepins.sd_miso, devicepins.sd_clk, devicepins.sd_cs);
DeviceReports DeviceCore::devicereports(&sd, DEVICEID);
Peripherals DeviceCore::peripherals;
mbed_error_ctx DeviceCore::error_ctx;
mbed_fault_context_t DeviceCore::fault_ctx;

DeviceCore::DeviceCore(void) : update_devicedata_thread(osPriorityNormal, UPDATE_DEVICEDATA_THREAD_STACK),
                               communication_thread(osPriorityNormal, COMMUNICATE_THREAD_STACK),
                               update_gps_data_thread(osPriorityNormal, GPS_THREAD_STACK),
                               reports_handler_thread(osPriorityNormal, REPORTS_THREAD_STACK)
{
    _gps_data = gps_data();
    sim_balance = signal_quality = 0;
    imu_ready = false;
    sd_ready = false;
    update_devicedata_thread_en = true;
    update_gps_data_thread_en = true;
    reports_handler_thread_en = true;
    network_led_status = true;
    init_success = false;
#ifdef DEBUG_MEM
    debug_mem_thread_en = 1;
#endif
    sleep_params = (SleepStruct){0, 0};
    current_location = new gps_data();
    modem_queue = mbed_event_queue();
    fail_counter = 0;
}

DeviceCore::~DeviceCore(void)
{
    delete current_location;
}

void DeviceCore::reboot(void)
{
    // NVIC_SystemReset();
    system_reset();
    wait(1);
}

void DeviceCore::init_timeout_func()
{
    if (!init_success)
        reboot();
}

void DeviceCore::test()
{
    // modem time
    /*
    time_t tt = modem.current_time();
    struct tm *when;
    when = localtime(&tt);
    char buf[100];
    strftime(buf, sizeof(buf), "%y/%m/%d,%H:%M:%S", when);
    printf("Time: %s\r\n", buf);
    */
    printf("Relay On\r\n");
    peripherals.set_relay(true);
    wait(2);
    printf("Relay Off\r\n");
    peripherals.set_relay(false);
    printf("Input IO: %d\r\n", peripherals.read_input());
}

int DeviceCore::init(void)
{
#ifdef FAKE_GPS
    printf("FAKE GPS ENABLED!!!\r\n");
#endif
#ifdef DEBUG_MEM
    debug_memory_thread.start(callback(this, &DeviceCore::debug_memory));
#endif
    init_timeout.attach(callback(this, &DeviceCore::init_timeout_func), INIT_TIMEOUT * 1000);
    //setting event flags
    sleep_event.clear(SAMPLE_FLAG);
    if (!imu.init() && !imu.init())
    {
        imu_ready = false;
        printf("IMU ERROR\r\n");
        peripherals.show_err(1);
    }
    else
        imu_ready = true;

    // init devicereport
    if (!devicereports.init())
    {
        printf("SD Mount/Write Failed, No Reports!\r\n");
        peripherals.show_err(6);
        wait(ERR_WAIT);
    }
    else
        sd_ready = 1;

    // initializing modem
    modem.power_on();
    // modem.test();
    // while (1)
    //     ;

    for (int i = 0; i < INIT_MAX_RETRIES; i++)
    {
        if (modem.init())
            break;
        if (i == INIT_MAX_RETRIES - 1)
        {
            printf("Modem Initialization Failed\r\n");
            peripherals.show_err(2);
            return -2;
        }
        wait(1);
    }
    // retrieve params from sd/modem
    retrieve_params();

    for (int i = 0; i < INIT_MAX_RETRIES; i++)
    {
        if (deviceparams.general_params.connection_type == 2)
        {
            //SMS MODE
            //TODO: do proper work to alert center that Device turned on!
        }
        else
        {
            if (modem.init_network())
            {
                break;
            }
            if (i == INIT_MAX_RETRIES - 1)
            {
                printf("Setting Modem Network Failed\r\n");
                peripherals.show_err(3);
                return -4;
            }
            wait(1);
        }
    }

    if (deviceparams.general_params.connection_type != 2)
    {
        printf("Checking Server...\r\n");
        for (int j = 0; j < INIT_MAX_RETRIES; j++)
        {
            if (connection.check_connection(deviceparams.center_params.center_params.center_address.ping_url))
                break;
            if (j == INIT_MAX_RETRIES - 1)
            {
                printf("Server Connection ERROR\r\n");
                peripherals.show_err(5);
                return -5;
            }
            wait(5);
        }
    }

    // delete inbox
    modem.del_sms_all();

    //sync time
    sync_time();

    // update modem params
    get_modem_params();

    if (sd_ready)
    {
        //TODO: Fix this, if the directory get large it cant scan missed file
        char *missed_file = new char[64]();
        if (devicereports.check_missed_files(missed_file))
        {
            if (deviceparams.general_params.connection_type != 2)
            {
                printf("Uploading missed file: %s\r\n", missed_file);
                if (upload_report(missed_file))
                {
                    printf("Report Upload OK!\r\n");
#ifdef DELETE_COMPLETE_REPORTS
                    sd.remove_file(missed_file);
#endif
                }
                else
                    printf("Missed File: %s Upload Failed!\r\n", missed_file);
            }
        }
        delete[] missed_file;
    }
    //starting threads
    update_devicedata_thread.start(callback(this, &DeviceCore::update_devicedata));
    update_gps_data_thread.start(callback(this, &DeviceCore::get_gps_data));
    status_thicker.attach(callback(this, &DeviceCore::status_blink), 1);
    modem_queue->call_every(MODEM_PARAMS_INTERVAL * 1000, this, &DeviceCore::get_modem_params);
    //set connection_type
    switch (deviceparams.general_params.connection_type)
    {
    case 1:
        printf("Network Mode!\r\n");
        modem_queue->call_every(deviceparams.general_params.timer.network_send * 1000, this, &DeviceCore::send_device_data);
        break;
    case 2:
        printf("SMS Mode!\r\n");
        modem_queue->call_every(deviceparams.general_params.timer.sms_send * 1000, this, &DeviceCore::send_device_data);
        break;
    case 3:
        printf("DUAL Mode!\r\n");
        modem_queue->call_every(deviceparams.general_params.timer.network_send * 1000, this, &DeviceCore::send_device_data);
        modem_queue->call_every(deviceparams.general_params.timer.sms_send * 1000, this, &DeviceCore::send_device_data);
        break;
    default:
        printf("INVALID CONNECTION_TYPE: %ld\r\n", deviceparams.general_params.connection_type);
        break;
    }
    modem_queue->call_every(deviceparams.general_params.timer.network_receive * 1000, this, &DeviceCore::get_center_commands);
    modem_queue->call_every(1000, this, &DeviceCore::check_failed_requests);
    //sync local time with modem
    modem_queue->call_every(MODEM_SYNC_TIME_INTERVAL * 1000, this, &DeviceCore::sync_time);
    communication_thread.start(callback(modem_queue, &EventQueue::dispatch_forever));
    if (sd_ready)
        reports_handler_thread.start(callback(this, &DeviceCore::reports_handler));
    init_success = true;
    return 1;
}

void DeviceCore::run(void)
{
    set_sleep();
}

int DeviceCore::read_center_params(DeviceMainParams *deviceparams)
{
    // 0: failed, 1: modem_data, 2: sd_data
    CenterCommands *center_commandsP = new CenterCommands();
    int result = 0;
    bool decode_result = false;
    // a flag to check if sd_config file exist
    bool sd_config_exist = false;
    std::string data = "";
    // check sd file config exists
    char *sd_config_path = new char[32];
    char *sd_config_file_data = new char[MAX_BUFFSIZE];
    sprintf(sd_config_path, SD_ROOT, DEVICEID);
    strcat(sd_config_path, "/");
    strcat(sd_config_path, SD_CONFIG_FILE);
    if ((sd_ready) && (sd.file_exist(sd_config_path)) && (sd.read_file_text(sd_config_path, sd_config_file_data, MAX_BUFFSIZE)))
    {
        data.assign(sd_config_file_data, strlen(sd_config_file_data));
        sd_config_exist = true;
#ifdef DELETE_SD_CONFIG
        sd.remove_file(sd_config_path);
#endif
    }
    // no valid file found in SD, check modem
    else if (modem.file_exist(CENTER_PARAMS_FILE))
    {
        data = modem.download_file(CENTER_PARAMS_FILE);
    }

    msg_encoded encoded_data = msg_encoded_default;
    encoded_data.length = sizeof(encoded_data.msg);
    if (!data.empty())
    {
        data = Utility::read_between_quotes(data);
        // printf("data: %s\r\n", data.c_str());
        if ((Base64::decode(encoded_data.msg, encoded_data.length, reinterpret_cast<const unsigned char *>(data.c_str()))) &&
            (deviceproto.center_commands_decode(&encoded_data, center_commandsP)))
        {
            //TODO: add key for validation
            // check validation of read data
            if (center_commandsP->device_id == DEVICEID)
            {
                decode_result = true;
                // assign to center_params
                if (center_commandsP->has_set_center_params)
                {
                    strcpy(deviceparams->center_params.key, center_commandsP->set_center_params.key);
                    strcpy(deviceparams->center_params.center_params.center_address.device_data_url, center_commandsP->set_center_params.center_params.center_address.device_data_url);
                    strcpy(deviceparams->center_params.center_params.center_address.center_commands_url, center_commandsP->set_center_params.center_params.center_address.center_commands_url);
                    strcpy(deviceparams->center_params.center_params.center_address.device_report_url, center_commandsP->set_center_params.center_params.center_address.device_report_url);
                    strcpy(deviceparams->center_params.center_params.center_address.ping_url, center_commandsP->set_center_params.center_params.center_address.ping_url);

                    strcpy(deviceparams->center_params.center_params.number1, center_commandsP->set_center_params.center_params.number1);
                    if (center_commandsP->set_center_params.center_params.has_number2)
                        strcpy(deviceparams->center_params.center_params.number2, center_commandsP->set_center_params.center_params.number2);
                    if (center_commandsP->set_center_params.center_params.has_number3)
                        strcpy(deviceparams->center_params.center_params.number3, center_commandsP->set_center_params.center_params.number3);
                    // printf("%s\r\n", deviceparams->center_params.center_params.number3);
                }
                // assign deviceparams
                if (center_commandsP->has_device_params)
                    deviceparams->general_params = center_commandsP->device_params;
            }
            else
                printf("Key or DEVICEID in config is not valid!!!, DEVICEID Saved: %ld, Current: %d\r\n", center_commandsP->device_id, DEVICEID);
        }
    }

    if (sd_config_exist)
    {
        if (decode_result)
        {
            result = 2;
        }
        if (!decode_result)
        {
            printf("SD File Corrupted, Check your config file in %s\r\n", sd_config_path);
            sd.remove_file(sd_config_path);
        }
    }
    // modem file retrieved successfully
    else if (decode_result)
        result = 1;

    delete[] sd_config_file_data;
    delete[] sd_config_path;
    delete center_commandsP;
    return result;
}

bool DeviceCore::write_center_params(DeviceMainParams *deviceparams)
{
    bool result = false;
    CenterCommands *center_commandsP = new CenterCommands();
    // prepare center_commandsP
    center_commandsP->device_id = DEVICEID;
    strcpy(center_commandsP->set_center_params.key, deviceparams->center_params.key);
    strcpy(center_commandsP->set_center_params.center_params.center_address.device_data_url, deviceparams->center_params.center_params.center_address.device_data_url);
    strcpy(center_commandsP->set_center_params.center_params.center_address.center_commands_url, deviceparams->center_params.center_params.center_address.center_commands_url);
    strcpy(center_commandsP->set_center_params.center_params.center_address.device_report_url, deviceparams->center_params.center_params.center_address.device_report_url);
    strcpy(center_commandsP->set_center_params.center_params.center_address.ping_url, deviceparams->center_params.center_params.center_address.ping_url);
    strcpy(center_commandsP->set_center_params.center_params.number1, deviceparams->center_params.center_params.number1);
    strcpy(center_commandsP->set_center_params.center_params.number2, deviceparams->center_params.center_params.number2);
    strcpy(center_commandsP->set_center_params.center_params.number3, deviceparams->center_params.center_params.number3);

    center_commandsP->device_params = deviceparams->general_params;

    msg_encoded encoded_data = msg_encoded_default;
    if (deviceproto.center_commands_encode(center_commandsP, &encoded_data))
    {
        unsigned char *b64_buffer = new unsigned char[MAX_BUFFSIZE]();
        if (Base64::encode(b64_buffer, MAX_BUFFSIZE, encoded_data.msg, encoded_data.length))
        {
            std::string data = "\"";
            data.append(reinterpret_cast<const char *>(b64_buffer));
            data.append("\"");
            // delete old file if exists
            // if (modem.file_exist(CENTER_PARAMS_FILE))
            //     modem.delete_file(CENTER_PARAMS_FILE);
            result = modem.upload_file(CENTER_PARAMS_FILE, data.length(), data.c_str());
        }
        delete[] b64_buffer;
    }
    delete center_commandsP;
    return result;
}

void DeviceCore::retrieve_params(void)
{
#ifdef SET_DEFAULT_PARAMS
    // enable to always use default params
    int result = 0;
#else
    // check if config file exist in modem or sd
    int result = read_center_params(&deviceparams);
#endif
    switch (result)
    {
    case 1:
        printf("DeviceMainParams Retrieved from modem!\r\n");
        break;
    case 2:
        printf("DeviceMainParams Retrieved from SD!\r\n");
        // printf("%s, %s, %s\r\n", deviceparams.center_params.center_params.number1);
        for (int i = 0; i < 5; i++)
        {
            peripherals.set_error(1);
            peripherals.set_network(1);
            peripherals.set_gps(1);
            wait_ms(300);
            peripherals.set_error(0);
            peripherals.set_network(0);
            peripherals.set_gps(0);
            wait_ms(300);
        }
        if (modem.file_exist(CENTER_PARAMS_FILE))
            modem.delete_file(CENTER_PARAMS_FILE);
        write_center_params(&deviceparams);
        break;
    default:
        printf("No valid DeviceMainParams found in Modem/SD, using default params!\r\n");
        if (modem.file_exist(CENTER_PARAMS_FILE))
            modem.delete_file(CENTER_PARAMS_FILE);
        write_center_params(&deviceparams);
        break;
    }
}

void DeviceCore::get_modem_params(void)
{
    modem_ready_mutex.lock();
    sim_balance = modem.balance();
    signal_quality = modem.get_signal_quality();
    modem_ready_mutex.unlock();
    printf("sim_balance: %d, signal_quality: %d\r\n", sim_balance, signal_quality);
}

void DeviceCore::reports_handler(void)
{
    //report file path
    std::string report_file = devicereports.file_path_gen();
    int64_t last_time = 0;
    while (reports_handler_thread_en)
    {
        // reports counter
        int write_counter = 0;
        while (write_counter < REPORTS_SEND_COUNTER)
        {
            // wait for devicedata to become ready
            // DeviceData *_reportsP = new DeviceData(_devicedata);
            // create a small part data
            DeviceData *_reportsP = new DeviceData();
            _reportsP->device_id = _devicedata.device_id;
            _reportsP->time = _devicedata.time;

            if (_reportsP->time - last_time >= REPORTS_WRITE_INTERVAL)
            {
                last_time = _reportsP->time;
                //fill location data
                _reportsP->gps_data.lat = current_location->lat;
                _reportsP->gps_data.lng = current_location->lng;
                _reportsP->gps_data.speed = current_location->speed;
                _reportsP->gps_data.altitude = current_location->altitude;
                _reportsP->gps_data.course = current_location->course;
                _reportsP->gps_data.hdop = current_location->hdop;
                // printf("***WriteReport: %d\r\n", _reportsP->time);
                if (devicereports.write_reports(report_file.c_str(), _reportsP))
                    write_counter++;
            }
            delete _reportsP;
        }
        //wait for modem to become available
        modem_ready_mutex.lock();
        printf("Compressing and Sending file: %s\r\n", report_file.c_str());
        char *report_file_cmp = new char[64]();
        strcpy(report_file_cmp, report_file.c_str());
        strcat(report_file_cmp, COMPRESS_PREFIX);
        Utility::removeSubstr(report_file_cmp, UNCOMPRESS_PREFIX);
        if (devicereports.compress_file(report_file.c_str(), report_file_cmp))
        {
            printf("Compressed to: %s\r\n", report_file_cmp);
            // check if sms mode is not active
            if (deviceparams.general_params.connection_type != 2)
            {
                if (upload_report(report_file_cmp))
                {
                    printf("Report Upload OK!\r\n");
#ifdef DELETE_COMPLETE_REPORTS
                    sd.remove_file(report_file_cmp);
#endif
                }
                else
                    printf("Report Upload failed!\r\n");
            }
        }
        else
            printf("Failed to compress\r\n");
        delete[] report_file_cmp;
        // release modem event
        modem_ready_mutex.unlock();
        //generate new report file
        report_file = devicereports.file_path_gen();
    }
}

void DeviceCore::update_devicedata(void)
{
    //update devicedata
    while (update_devicedata_thread_en)
    {
        // printf("preparing devicedata\r\n");
        //clear device data event flag to command waiting for others
        //setting default deviceid
        DeviceData *devicedataP = new DeviceData();
        devicedataP->device_id = DEVICEID;
        devicedataP->time = static_cast<int64_t>(peripherals.current_time());
        // fill general datas
        //server params

        strcpy(devicedataP->center_params.center_address.device_data_url, deviceparams.center_params.center_params.center_address.device_data_url);
        strcpy(devicedataP->center_params.center_address.center_commands_url, deviceparams.center_params.center_params.center_address.center_commands_url);
        strcpy(devicedataP->center_params.center_address.ping_url, deviceparams.center_params.center_params.center_address.ping_url);
        strcpy(devicedataP->center_params.center_address.device_report_url, deviceparams.center_params.center_params.center_address.device_report_url);
        // phonenumbers
        strcpy(devicedataP->center_params.number1, deviceparams.center_params.center_params.number1);
        strcpy(devicedataP->center_params.number2, deviceparams.center_params.center_params.number2);
        strcpy(devicedataP->center_params.number3, deviceparams.center_params.center_params.number3);
        //timers
        devicedataP->device_params.timer.network_receive = deviceparams.general_params.timer.network_receive;
        devicedataP->device_params.timer.network_send = deviceparams.general_params.timer.network_send;
        devicedataP->device_params.timer.sms_send = deviceparams.general_params.timer.sms_send;

        // data select
        devicedataP->device_params.data_select.battery_enable = deviceparams.general_params.data_select.battery_enable;
        devicedataP->device_params.data_select.gps_enable = deviceparams.general_params.data_select.gps_enable;
        devicedataP->device_params.data_select.imu_enable = deviceparams.general_params.data_select.imu_enable;
        devicedataP->device_params.data_select.input_sensor_enable = deviceparams.general_params.data_select.input_sensor_enable;
        devicedataP->device_params.data_select.relay_enable = deviceparams.general_params.data_select.relay_enable;
        devicedataP->device_params.data_select.sim_balance_enable = deviceparams.general_params.data_select.sim_balance_enable;
        devicedataP->device_params.data_select.signal_quality_enable = deviceparams.general_params.data_select.signal_quality_enable;
        // connection type
        devicedataP->device_params.connection_type = deviceparams.general_params.connection_type;

        devicedataP->gps_data.lat = _gps_data.lat;
        devicedataP->gps_data.lng = _gps_data.lng;
        devicedataP->gps_data.speed = _gps_data.speed;
        devicedataP->gps_data.altitude = _gps_data.altitude;
        devicedataP->gps_data.course = _gps_data.course;
        devicedataP->gps_data.hdop = _gps_data.hdop;

        devicedataP->relay = deviceparams.relay;

        devicedataP->input_sensor = peripherals.read_input();

        // check if sim_balance is 0 then set it to -2
        devicedataP->sim_balance = (sim_balance != 0) ? sim_balance : -2;

        devicedataP->signal_quality = (signal_quality > 0) ? signal_quality : -1;

        if (imu_ready == true)
        {
            imu_data _imu_data = imu.read();
            // printf("ax: %f, ay: %f, az: %f\r\n", _imu_data.ax, _imu_data.ay, _imu_data.az);
            // printf("gx: %f, gy: %f, gz: %f\r\n", _imu_data.gx, _imu_data.gy, _imu_data.gz);
            // printf("mx: %f, my: %f, mz: %f\r\n", _imu_data.mx, _imu_data.my, _imu_data.mz);
            devicedataP->imu_data.ax = _imu_data.ax;
            devicedataP->imu_data.ay = _imu_data.ay;
            devicedataP->imu_data.az = _imu_data.az;
            devicedataP->imu_data.gx = _imu_data.gx;
            devicedataP->imu_data.gy = _imu_data.gy;
            devicedataP->imu_data.gz = _imu_data.gz;
            devicedataP->imu_data.mx = _imu_data.mx;
            devicedataP->imu_data.my = _imu_data.my;
            devicedataP->imu_data.mz = _imu_data.mz;
        }
        else
        {
            devicedataP->imu_data.ax = devicedataP->imu_data.ay = devicedataP->imu_data.az =
                devicedataP->imu_data.mx = devicedataP->imu_data.my = devicedataP->imu_data.mz =
                    devicedataP->imu_data.gx = devicedataP->imu_data.gy = devicedataP->imu_data.gz = -1;
        }

        BatteryStruct *battery_data = new BatteryStruct(peripherals.read_battery());
        devicedataP->battery_data.capacity = battery_data->capacity;
        devicedataP->battery_data.plugged = battery_data->plugged;
        delete battery_data;

        _devicedata = (*devicedataP);
        delete devicedataP;
    }
}

#ifdef DEBUG_MEM
void DeviceCore::print_memory_info()
{
    // allocate enough room for every thread's stack statistics
    int cnt = osThreadGetCount();
    mbed_stats_stack_t *stats = (mbed_stats_stack_t *)malloc(cnt * sizeof(mbed_stats_stack_t));

    cnt = mbed_stats_stack_get_each(stats, cnt);
    for (int i = 0; i < cnt; i++)
    {
        float utilization = ((float)stats[i].max_size / (float)stats[i].reserved_size) * 100;
        // printf("Thread: 0x%lX, Stack size: %lu / %lu, %.1f\r\n", stats[i].thread_id, stats[i].max_size, stats[i].reserved_size, utilization);
        if (utilization > MAX_MEM_UTIL)
            printf("Thread Utilization Reached: 0x%lX, Stack size: %lu / %lu, %.1f\r\n", stats[i].thread_id, stats[i].max_size, stats[i].reserved_size, utilization);
    }
    free(stats);

    // Grab the heap statistics
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    printf("Heap size: %lu / %lu bytes\r\n", heap_stats.current_size, heap_stats.reserved_size);
}
#endif

bool DeviceCore::upload_report(const char *file_path)
{
    bool result = false;
    long size = sd.file_size(file_path);
    if (!size)
        return result;
    const int b64_size = Base64::encode_size(size);
    const int http_params_size = 32;
    unsigned char *file_buffer = new unsigned char[size]();
    unsigned char *file_buffer_base64 = new unsigned char[b64_size + http_params_size]();
    int bytesRead = sd.read_file_binary(file_path, file_buffer, size);
    // read up to sizeof(buffer) bytes
    if (bytesRead)
    {
        // printf("file size: %ld, b64_size: %d\r\n", size, b64_size);
        // generate b64 from buffer
        if (Base64::encode(file_buffer_base64, b64_size, file_buffer, size))
        {
            // printf("base64: %s\r\n", file_buffer_base64);
            // printf("size: %d\r\n", b64_size);
            //send file
            if (connection.upload_report(deviceparams.center_params.center_params.center_address.device_report_url, file_buffer_base64))
                result = true;
        }
        else
        {
            printf("Send Report B64 Fail\r\n");
        }
    }
    delete[] file_buffer_base64;
    delete[] file_buffer;
    return result;
}

void DeviceCore::send_device_data(void)
{
    //TODO: add multiple sms numbers
    modem_ready_mutex.lock();
    // set dataselect
    DeviceData *devicedataP = new DeviceData(_devicedata);

    if (deviceparams.general_params.data_select.gps_enable == 0)
    {
        devicedataP->gps_data.lat = 0;
        devicedataP->gps_data.lng = 0;
        devicedataP->gps_data.speed = 0;
        devicedataP->gps_data.altitude = 0;
        devicedataP->gps_data.course = 0;
        devicedataP->gps_data.hdop = 0;
    }
    if (deviceparams.general_params.data_select.relay_enable == 0)
        devicedataP->relay = 0;

    if (deviceparams.general_params.data_select.input_sensor_enable == 0)
        devicedataP->input_sensor = 0;

    if (devicedataP->device_params.data_select.sim_balance_enable == 0)
        // check if sim_balance is 0 then set it to -2
        devicedataP->sim_balance = 0;

    if (devicedataP->device_params.data_select.signal_quality_enable == 0)
        devicedataP->signal_quality = 0;

    if (deviceparams.general_params.data_select.imu_enable == 0)
    {
        devicedataP->imu_data.ax = devicedataP->imu_data.ay = devicedataP->imu_data.az =
            devicedataP->imu_data.mx = devicedataP->imu_data.my = devicedataP->imu_data.mz =
                devicedataP->imu_data.gx = devicedataP->imu_data.gy = devicedataP->imu_data.gz = 0;
    }
    if (deviceparams.general_params.data_select.battery_enable == 0)
    {
        devicedataP->battery_data.capacity = 0;
        devicedataP->battery_data.plugged = 0;
    }

    // printf("sending devicedata: %s\r\n", deviceparams.center_params.center_params.center_address.device_data_url);
    if (connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                    deviceparams.center_params.center_params.number1,
                                    deviceparams.general_params.connection_type, devicedataP))
    {
        fail_counter = 0;
        printf("loc: %f, %f, spd: %f, alt: %f, course: %f, hdop: %f\r\n", devicedataP->gps_data.lat, devicedataP->gps_data.lng, devicedataP->gps_data.speed, devicedataP->gps_data.altitude, devicedataP->gps_data.course, devicedataP->gps_data.hdop);
        printf("DeviceData Sent\r\n");
        peripherals.set_error(0);
    }
    else
    {
        printf("DeviceData Failed!\r\n");
        peripherals.set_error(1);
        wait(ERR_WAIT);
        fail_counter++;
    }
    delete devicedataP;
    modem_ready_mutex.unlock();
}

void DeviceCore::get_center_commands(void)
{
    // a flag to check if we have permenant deviceparsm then save it on modem
    bool has_new_device_params = false;
    // a flag to hold result of new params
    bool set_new_params_ok = false;
    modem_ready_mutex.lock();
    //wait for modem to become available
    // printf("getting center_commands\r\n");
    CenterCommands *center_commandsP = new CenterCommands();
    int conn_result = connection.get_center_commands(
        deviceparams.center_params.center_params.center_address.center_commands_url,
        deviceparams.center_params.center_params.number1,
        deviceparams.general_params.connection_type,
        center_commandsP);
    // release modem flag
    if (conn_result > 0)
    {
        fail_counter = 0;
        printf("New CenterCommand Received\r\n");
        peripherals.set_error(0);
        // printf("Center command get OK\r\n");
        //TODO: do proper on invalid deviceid
        if (center_commandsP->device_id != DEVICEID)
            printf("DEVICE ID IS NOT VALID\r\n");
        //set robparams for new _center_commands
        if (center_commandsP->has_device_params)
        {
            has_new_device_params = set_new_params_ok = true;
            printf("New device params\r\n");
            // printf("deviceparams.general_params.connection_type:%d\r\n", center_commandsP->device_params.connection_type);
            // printf("center_commandsP->device_params.data_select.battery_enable:%d\r\n", center_commandsP->device_params.data_select.battery_enable);
            // printf("center_commandsP->device_params.data_select.signal_quality_enable:%d\r\n", center_commandsP->device_params.data_select.signal_quality_enable);

            // printf("center_commandsP->device_params.timer.network_receive:%d\r\n", center_commandsP->device_params.timer.network_receive);
            // printf("center_commandsP->device_params.timer.network_send:%d\r\n", center_commandsP->device_params.timer.network_send);
            // printf("center_commandsP->device_params.timer.sms_send:%d\r\n", center_commandsP->device_params.timer.sms_send);

            deviceparams.general_params.connection_type = center_commandsP->device_params.connection_type;
            deviceparams.general_params.data_select.battery_enable = center_commandsP->device_params.data_select.battery_enable;
            deviceparams.general_params.data_select.gps_enable = center_commandsP->device_params.data_select.gps_enable;
            deviceparams.general_params.data_select.imu_enable = center_commandsP->device_params.data_select.imu_enable;
            deviceparams.general_params.data_select.input_sensor_enable = center_commandsP->device_params.data_select.input_sensor_enable;
            deviceparams.general_params.data_select.relay_enable = center_commandsP->device_params.data_select.relay_enable;
            deviceparams.general_params.data_select.sim_balance_enable = center_commandsP->device_params.data_select.sim_balance_enable;
            deviceparams.general_params.data_select.signal_quality_enable = center_commandsP->device_params.data_select.signal_quality_enable;

            deviceparams.general_params.timer.network_receive = center_commandsP->device_params.timer.network_receive;
            deviceparams.general_params.timer.network_send = center_commandsP->device_params.timer.network_send;
            deviceparams.general_params.timer.sms_send = center_commandsP->device_params.timer.sms_send;
        }
        // printf("relay: %d\r\n", center_commandsP->has_relay);
        if ((center_commandsP->has_relay) && (deviceparams.relay != center_commandsP->relay))
        {
            printf("New relay value: %ld\r\n", center_commandsP->relay);
            peripherals.set_relay((center_commandsP->relay == 1) ? true : false);
            deviceparams.relay = center_commandsP->relay;

            DeviceData *devicedataP = new DeviceData();
            devicedataP->device_id = DEVICEID;
            devicedataP->time = static_cast<int64_t>(peripherals.current_time());
            strcpy(devicedataP->custom_field[0].key, ACK_KEY);
            strcpy(devicedataP->custom_field[0].value, ACK_OK);
            if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                             deviceparams.center_params.center_params.number1,
                                             deviceparams.general_params.connection_type, devicedataP))
                printf("Send set new params failed!\r\n");
            delete devicedataP;
        }

        // printf("has_set_center_params: %d\r\n", center_commandsP->has_set_center_params);
        if (center_commandsP->has_set_center_params)
        {
            has_new_device_params = true;
            printf("New center_params\r\n");
            // printf("has_server_address: %d\r\n", center_commandsP->set_center_params.center_params.has_server_address);
            strcpy(deviceparams.center_params.key, center_commandsP->set_center_params.key);
            // read number1
            strcpy(deviceparams.center_params.center_params.number1, center_commandsP->set_center_params.center_params.number1);
            if (center_commandsP->set_center_params.center_params.has_number2)
                strcpy(deviceparams.center_params.center_params.number2, center_commandsP->set_center_params.center_params.number2);
            if (center_commandsP->set_center_params.center_params.has_number3)
                strcpy(deviceparams.center_params.center_params.number3, center_commandsP->set_center_params.center_params.number3);
            if (center_commandsP->set_center_params.center_params.has_center_address)
            {
                printf("Checking New Server: %s\r\n", center_commandsP->set_center_params.center_params.center_address.ping_url);
                if (connection.check_connection(center_commandsP->set_center_params.center_params.center_address.ping_url))
                {
                    strcpy(deviceparams.center_params.center_params.center_address.device_data_url, center_commandsP->set_center_params.center_params.center_address.device_data_url);
                    strcpy(deviceparams.center_params.center_params.center_address.center_commands_url, center_commandsP->set_center_params.center_params.center_address.center_commands_url);
                    strcpy(deviceparams.center_params.center_params.center_address.ping_url, center_commandsP->set_center_params.center_params.center_address.ping_url);
                    strcpy(deviceparams.center_params.center_params.center_address.device_report_url, center_commandsP->set_center_params.center_params.center_address.device_report_url);
                    printf("Server Address changed!\r\n");
                    // printf("Here: %s\r\n", deviceparams.center_params.center_params.center_address.device_data_url.c_str());
                    set_new_params_ok = true;
                }
                else
                    printf("Failed to change server address to: %s\r\n", center_commandsP->set_center_params.center_params.center_address.ping_url);
            }
        }
        // Handle custom commands
        if (center_commandsP->custom_field_count > 0)
        {
            printf("center_commandsP->custom_field_count: %d\r\n", center_commandsP->custom_field_count);
            for (int i = 0; i < center_commandsP->custom_field_count; i++)
            {
                // if we have reset command
                if (strcmp(center_commandsP->custom_field[i].key, RESET_STR) == 0)
                {
                    printf("Reseting System...\r\n");
                    DeviceData *devicedataP = new DeviceData();
                    devicedataP->device_id = DEVICEID;
                    devicedataP->time = static_cast<int64_t>(peripherals.current_time());
                    strcpy(devicedataP->custom_field[0].key, ACK_KEY);
                    strcpy(devicedataP->custom_field[0].value, ACK_OK);
                    if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                     deviceparams.center_params.center_params.number1,
                                                     deviceparams.general_params.connection_type, devicedataP))
                        printf("Send set new params failed!\r\n");
                    delete devicedataP;
                    wait(1);
                    reboot();
                }
                // set time
                else if (strcmp(center_commandsP->custom_field[i].key, SET_TIME_STR) == 0)
                {
                    long set_time_value = strtol(center_commandsP->custom_field[i].value, NULL, 0);
                    printf("Setting System Time to: %ld\r\n", set_time_value);
                    set_time(static_cast<time_t>(set_time_value));

                    // prepare devicedata
                    DeviceData *devicedataP = new DeviceData();
                    devicedataP->device_id = DEVICEID;
                    devicedataP->time = static_cast<int64_t>(peripherals.current_time());
                    strcpy(devicedataP->custom_field[0].key, SET_TIME_STR);
                    strcpy(devicedataP->custom_field[0].value, ACK_OK);
                    if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                     deviceparams.center_params.center_params.number1,
                                                     deviceparams.general_params.connection_type, devicedataP))
                        printf("USSD Result Send failed!\r\n");
                    delete devicedataP;
                }
                // execute ussd code
                else if (strcmp(center_commandsP->custom_field[i].key, USSD_STR) == 0)
                {
                    printf("New USSD Command: %s\r\n", center_commandsP->custom_field[i].value);
                    // wait for modem to become available
                    //execute ussd code
                    std::string res = modem.ussd(center_commandsP->custom_field[i].value);
                    printf("USSD result: %s\r\n", res.c_str());
                    // if result is empty fill with error
                    if (res.empty() || (res.length() > USSD_MAX_LENGTH))
                        res = ACK_ERR;
                    // prepare devicedata
                    DeviceData *devicedataP = new DeviceData();
                    devicedataP->device_id = DEVICEID;
                    devicedataP->time = static_cast<int64_t>(peripherals.current_time());
                    strcpy(devicedataP->custom_field[0].key, USSD_STR);
                    strcpy(devicedataP->custom_field[0].value, res.c_str());
                    if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                     deviceparams.center_params.center_params.number1,
                                                     deviceparams.general_params.connection_type, devicedataP))
                        printf("USSD Result Send failed!\r\n");
                    delete devicedataP;
                }
                // remove all reports in SD
                else if (strcmp(center_commandsP->custom_field[i].key, ERASE_SD_STR) == 0)
                {
                    printf("Removing all reports in SD\r\n");
                    DeviceData *devicedataP = new DeviceData();
                    devicedataP->device_id = DEVICEID;
                    devicedataP->time = static_cast<int64_t>(peripherals.current_time());
                    strcpy(devicedataP->custom_field[0].key, ERASE_SD_STR);
                    strcpy(devicedataP->custom_field[0].value, ((sd_ready && devicereports.remove_reports()) == true) ? ACK_OK : ACK_ERR);
                    if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                     deviceparams.center_params.center_params.number1,
                                                     deviceparams.general_params.connection_type, devicedataP))
                        printf("Remove Reports Result Send failed!\r\n");
                    delete devicedataP;
                }
                // read IMSI
                else if (strcmp(center_commandsP->custom_field[i].key, READ_IMSI_STR) == 0)
                {
                    printf("Reading IMSI\r\n");
                    DeviceData *devicedataP = new DeviceData();
                    devicedataP->device_id = DEVICEID;
                    devicedataP->time = static_cast<int64_t>(peripherals.current_time());
                    strcpy(devicedataP->custom_field[0].key, READ_IMSI_STR);
                    strcpy(devicedataP->custom_field[0].value, modem.read_imsi().c_str());
                    if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                     deviceparams.center_params.center_params.number1,
                                                     deviceparams.general_params.connection_type, devicedataP))
                        printf("IMSI Result Send failed!\r\n");
                    delete devicedataP;
                }
            }
        }
        //check sleep command
        if (center_commandsP->has_device_sleep)
        {
            printf("Sleep Command Received!\r\n");
            // printf("Start: %d\r\n",  center_commandsP->device_sleep.start);
            // printf("end: %d\r\n",  center_commandsP->device_sleep.end);
            int interval = center_commandsP->device_sleep.end - center_commandsP->device_sleep.start;

            DeviceData *devicedataP = new DeviceData();
            devicedataP->device_id = DEVICEID;
            devicedataP->time = static_cast<int64_t>(peripherals.current_time());
            strcpy(devicedataP->custom_field[0].key, ACK_KEY);

            if (interval > SLEEP_THRESHOLD)
            {
                sleep_params.start = center_commandsP->device_sleep.start;
                sleep_params.end = center_commandsP->device_sleep.end;

                strcpy(devicedataP->custom_field[0].value, ACK_OK);
                if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                 deviceparams.center_params.center_params.number1,
                                                 deviceparams.general_params.connection_type, devicedataP))
                    printf("Send set new params failed!\r\n");
                delete devicedataP;

                // set event flag
                sleep_event.set(SAMPLE_FLAG);
            }
            else
            {
                strcpy(devicedataP->custom_field[0].value, ACK_ERR);
                if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                                 deviceparams.center_params.center_params.number1,
                                                 deviceparams.general_params.connection_type, devicedataP))
                    printf("Send set new params failed!\r\n");
                delete devicedataP;
            }
        }
        //check new_device_params
        if (has_new_device_params)
        {
            printf("Saving new params to modem!\r\n");
            //delete old file
            modem.delete_file(CENTER_PARAMS_FILE);
            //save new params to modem
            write_center_params(&deviceparams);
            // send ACK to Server

            DeviceData *devicedataP = new DeviceData();
            devicedataP->device_id = DEVICEID;
            devicedataP->time = static_cast<int64_t>(peripherals.current_time());
            strcpy(devicedataP->custom_field[0].key, ACK_KEY);
            strcpy(devicedataP->custom_field[0].value, (set_new_params_ok == true) ? ACK_OK : ACK_ERR);
            if (!connection.send_device_data(deviceparams.center_params.center_params.center_address.device_data_url,
                                             deviceparams.center_params.center_params.number1,
                                             deviceparams.general_params.connection_type, devicedataP))
                printf("Send set new params failed!\r\n");
            delete devicedataP;

            wait(1);
            reboot();
        }
    }
    else if (conn_result < 0)
    {
        fail_counter = 0;
        printf("No Center Command\r\n");
        peripherals.set_error(0);
    }
    else
    {
        printf("Center Commands Failed\r\n");
        peripherals.set_error(1);
        wait(ERR_WAIT);
        fail_counter++;
    }
    delete center_commandsP;
    modem_ready_mutex.unlock();
    // return result;
    // printf("get center commands done\r\n");
    // printf("deviceparams.general_params.timer.network_receive: %d\r\n", deviceparams.general_params.timer.network_receive);
}

void DeviceCore::get_gps_data(void)
{
#ifdef FAKE_GPS
    srand(time(NULL));
    size_t counter = 0;
    std::vector<std::vector<float>> fake_locations = {{35.693054, 51.403663}, {35.692133, 51.403520}, {35.691601, 51.403407}, {35.691514, 51.404142}, {35.691479, 51.404786}, {35.692085, 51.404850}, {35.692773, 51.404888}, {35.693187, 51.404909}, {35.693213, 51.404292}, {35.693257, 51.403739}};
#endif

    while (update_gps_data_thread_en)
    {
#ifndef FAKE_GPS
        const int default_value = -1;
        // loc = modem.get_gps_data_optimal(GET_GPS_DURATION, GET_GPS_COUNT);
        // const gps_data loc = modem.get_gps_data_fix(current_location);
        const gps_data loc = modem.get_gps_data(GET_GPS_DURATION);
        *current_location = loc;
        if ((loc.lat != default_value) || (loc.lng != default_value) ||
            (loc.speed != default_value) || (loc.altitude != default_value) ||
            (loc.course != default_value))
        {
            peripherals.set_gps(1);
            //round values
            // loc.lat = roundf(loc.lat * 100000) / 100000;
            // loc.lng = roundf(loc.lng * 100000) / 100000;
            // loc.speed = roundf(loc.speed * 100000) / 100000;
            // loc.altitude = roundf(loc.altitude * 100000) / 100000;
            // loc.course = roundf(loc.course * 100000) / 100000;
            // loc.hdop = roundf(loc.hdop * 100000) / 100000;
            // printf("GPS DATA READY!\r\n");
            // printf("loc: %f, %f, spd: %f, alt: %f, course: %f, hdop: %f\r\n", loc.lat, loc.lng, loc.speed, loc.altitude, loc.course, loc.hdop);
            _gps_data = loc;
        }
        else
        {
            _gps_data = gps_data();
            peripherals.set_gps(0);
        }
#else
        peripherals.set_gps(1);
        // int r = rand() % 10000;
        // float random = (float)r / 1000000.0;
        // printf("random: %d, %.6f\r\n", r, random);
        // _gps_data.lat = 35.681068 + random;
        // _gps_data.lng = 51.401755 + random;
        // _gps_data.speed = r / 100;
        // _gps_data.altitude = 1200;
        // _gps_data.course = 180;
        // _gps_data.hdop = 1.05;

        if (counter >= fake_locations.size())
            counter = 0;

        current_location->lat = _gps_data.lat = fake_locations[counter][0];
        current_location->lng = _gps_data.lng = fake_locations[counter][1];

        current_location->speed = _gps_data.speed = 50;
        current_location->altitude = _gps_data.altitude = 1200;
        current_location->course = _gps_data.course = 180;
        current_location->hdop = _gps_data.hdop = 1.05;
        counter++;
        wait(5);

#endif
    }
}

void DeviceCore::status_blink(void)
{
    peripherals.set_network(network_led_status);
    network_led_status = !network_led_status;
}

void DeviceCore::clock_source(void)
{
    switch (__HAL_RCC_GET_SYSCLK_SOURCE())
    {
    case RCC_SYSCLKSOURCE_STATUS_HSI:
        printf("HSI used as system clock!\r\n");
        break;
    case RCC_SYSCLKSOURCE_STATUS_HSE:
        printf("HSE used as system clock!\r\n");
        break;
    case RCC_SYSCLKSOURCE_STATUS_PLLCLK:
        printf("HSE oscillator clock used as system clock!\r\n");
        break;
    }
}

void DeviceCore::rtc_clock_source(void)
{
    switch (LL_RCC_GetRTCClockSource())
    {
    case LL_RCC_RTC_CLKSOURCE_NONE:
        printf("RTC is not configured!\r\n");
        break;
    case LL_RCC_RTC_CLKSOURCE_LSE:
        printf("LSE is configured!\r\n");
        break;
    case LL_RCC_RTC_CLKSOURCE_LSI:
        printf("LSI is configured!\r\n");
        break;
    default:
        printf("RTC ????\r\n");
        break;
    }
}

void DeviceCore::clock_check(void)
{
    // printf("SystemCoreClock is %d Hz\r\n", SystemCoreClock);
    printf("HSE_VALUE %d Hz\r\n", HSE_VALUE);
    clock_source();
    rtc_clock_source();
}

void DeviceCore::set_sleep(void)
{
    // wait on sleep flag
    time_t sleep_seconds = 0;
    printf("Waiting for any sleep\r\n");
    sleep_event.wait_any(SAMPLE_FLAG);
    time_t start = sleep_params.start;
    time_t end = sleep_params.end;
    // printf("New Sleep from %d\r\n", , );
    printf("New Sleep from %s", ctime(&start));
    printf("to %s\r\n", ctime(&end));
    // wait until start time reach
    while (peripherals.current_time() < sleep_params.start)
    {
        wait_ms(20);
    }
    sleep_seconds = sleep_params.end - sleep_params.start;
    // disabling all threads
    update_gps_data_thread_en = 0;
    reports_handler_thread_en = 0;
#ifdef DEBUG_MEM
    debug_mem_thread_en = 0;
#endif
    //terminating all threads
    modem_queue->break_dispatch();
    communication_thread.terminate();
    modem_ready_mutex.lock();
    modem_ready_mutex.unlock();
    update_devicedata_thread.terminate();
    update_gps_data_thread.terminate();
    reports_handler_thread.terminate();
    // check_memory_thread.terminate();
#ifdef DEBUG_MEM
    debug_memory_thread.terminate();
#endif
    // powering off peripherals
    modem.power_off();
    peripherals.set_relay(0);

    // finally sleep
    printf("Sleep for %lld seconds\r\n", (long long)sleep_seconds);
    sleep_helper(static_cast<int64_t>(sleep_seconds));
    // sleep is over reset system
    printf("Sleep over\r\n");
    wait_ms(100);
    system_reset();
}

void DeviceCore::sleep_helper(int64_t seconds)
{
    int i = 0;
    const int timeout = seconds * 50;
    while (i < timeout)
    {
        wait_ms(20);
        i++;
    }
}

#ifdef DEBUG_MEM
void DeviceCore::debug_memory(void)
{
    while (debug_mem_thread_en)
    {
        print_memory_info();
        wait(DEBUG_MEM_INTERVAL);
    }
}
#endif

void DeviceCore::check_failed_requests(void)
{
    if (fail_counter > COMMUNICATE_MAX_FAILS - 1)
    {
#ifdef SWITCH_ON_FAILURE
        printf("Switching to SMS Mode caused by %d failed requests!\r\n", COMMUNICATE_MAX_FAILS);
        deviceparams.general_params.connection_type = 2;
        modem.delete_file(CENTER_PARAMS_FILE);
        //save new params to modem
        write_center_params(&deviceparams);
#else
        printf("Rebooting caused by %d failed requests!\r\n", COMMUNICATE_MAX_FAILS);
#endif
        wait(1);
        reboot();
    }
}

time_t DeviceCore::current_time_api(void)
{
    long set_time_value;
    std::string resp = modem.http_get(TIME_API_URL, 5000);
    if (!resp.empty())
    {
        set_time_value = strtol(resp.c_str(), NULL, 0);
        return static_cast<time_t>(set_time_value);
    }
    return 0;
}

void DeviceCore::sync_time(void)
{
    time_t now = 0;
    modem_ready_mutex.lock();
    if (deviceparams.general_params.connection_type == 2)
    {
    }
    // simcard time
    // now = modem.current_time();
    else
        // api time
        now = current_time_api();
    modem_ready_mutex.unlock();
    if (now > 0)
    {
        set_time(now);
        printf("time synced to: %lld, %s", (long long)now, ctime(&now));
    }
}

void DeviceCore::check_error()
{
    if (error_ctx.error_status < 0)
    {
        printf("\nSuccessfully read reboot info ==> \n");
        printf("\n  error status: 0x%08lX", (uint32_t)error_ctx.error_status);
        printf("\n  error value: 0x%08lX", (uint32_t)error_ctx.error_value);
        printf("\n  error address: 0x%08lX", (uint32_t)error_ctx.error_address);
        printf("\n  error reboot count: 0x%08lX", (uint32_t)error_ctx.error_reboot_count);
        printf("\n  error crc: 0x%08lX\n", (uint32_t)error_ctx.crc_error_ctx);

        //Read fault context
        if (error_ctx.error_status == MBED_ERROR_HARDFAULT_EXCEPTION)
        {
            mbed_get_reboot_fault_context(&fault_ctx);

            printf("\nCrash Report data captured:\n");
            for (int i = 0; i < 16; i++)
            {
                printf("[%d]: 0x%08X\n", i, ((uint32_t *)(&fault_ctx))[i]);
            }
        }
    }
}

void DeviceCore::system_check()
{
    mbed_reset_reboot_count();
    check_error();
    clock_check();
#ifndef MBED_TICKLESS
    printf("MBED_TICKLESS NOT DEFINED!\r\n");
#endif
}
