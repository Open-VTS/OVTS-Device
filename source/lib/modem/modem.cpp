/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Quectel MC60 library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#include "modem.h"

#ifdef STAND_ALONE
/*
* Constructor for standalone mode
*/
Modem::Modem(PinName Tx, PinName Rx, PinName GPS_Tx, PinName GPS_Rx, PinName pw, PinName sw, const double hdop_threshold) : modem_serial(Tx, Rx, 115200), parser(&modem_serial), gps_serial(GPS_Tx, GPS_Rx, 115200), _power(pw), _switch(sw)
{
    _hdop_threshold = hdop_threshold;
    // parser.debug_on(1);
    parser.set_delimiter("\r\n");
    parser.set_timeout(default_timeout);
    gps_stream = fdopen(&gps_serial, "w+");
}

#else
/*
* Constructor for All in one mode
*/
Modem::Modem(PinName Tx, PinName Rx, PinName pw, PinName sw) : modem_serial(Tx, Rx), _power(pw), _switch(sw)
{
    _hdop_threshold = hdop_threshold;
    modem_serial.baud(115200);
}
#endif

/*
* Destructor
*/
Modem::~Modem()
{
}

/*
* Power on module
*/
void Modem::power_on(void)
{
    _power = 1;
    wait_ms(100);
    _switch = 1;
    wait_ms(1100);
    _switch = 0;
}

/*
* Power off module
*/
void Modem::power_off(void)
{
    _switch = 1;
    wait_ms(850);
    _switch = 0;
    wait_ms(100);
    _power = 0;
}

/*
* flush modem serial
*/
void Modem::flush()
{
    parser.flush();
}

/*
* Ping modem serial
*/
bool Modem::ping(void)
{
    parser.send("AT");
    return parser.recv("OK");
}

/*
* Check modem network status
*/
bool Modem::check_gsm_status(void)
{
    const float timeout = 20.0f;
    bool res = false;
    std::string respond, temp;
    Timer timer;
    timer.start();
    float start = timer.read();
    while (timer.read() - start < timeout)
    {
        parser.send("AT+QNSTATUS");
        if (check_for_respond("+QNSTATUS: 0\r\nOK"))
        {
            res = true;
            break;
        }
        wait_ms(200);
    }
    timer.stop();
    return res;
}

/*
* Initialize modem
*/
bool Modem::init_modem(void)
{
    if (!check_gsm_status())
        return false;

    //put some delay to make sure op can set
    wait_ms(200);
    if (!set_operator())
    {
        printf("set op failed\r\n");
        return false;
    }
    wait_ms(200);

    if (!set_params())
    {
        printf("set params failed\r\n");
        return false;
    }
    return true;
}

/*
* Set Operator name
*/
bool Modem::set_operator(void)
{
    parser.send("AT+COPS?");
    std::string respond = read_until("OK", 2000);
    // printf("OP: %s\r\n", respond.c_str());
    if (respond.find(OP_MTN) != std::string::npos)
    {
        _current_op = OP_MTN;
        return true;
    }
    else if (respond.find(OP_MCI) != std::string::npos)
    {
        _current_op = OP_MCI;
        return true;
    }
    else
        _current_op = OP_ERR;
    return false;
}

/*
* Set modem basic parameters
*/
bool Modem::set_params(void)
{
    parser.send("AT+CSCS=\"GSM\"");
    if (!check_for_respond())
        return false;
    parser.send("AT+CMGF=1");
    if (!check_for_respond())
        return false;
    parser.send("AT+CSMP=17,167,0,241");
    if (!check_for_respond())
        return false;
    parser.send("AT+CSDH=1");
    if (!check_for_respond())
        return false;
    //setting language to EN for MTN Irancell
    if (_current_op == OP_MTN && (!ussd("*555*4*3*2#").empty()))
    {
        return true;
    }
    else if (_current_op == OP_MCI)
        return true;
    return false;
}

/*
* Check a desirable respond from modem after sending a command
*/
bool Modem::check_for_respond(const char *value, int timeout)
{
    parser.set_timeout(timeout);
    if (parser.recv(value))
    {
        parser.set_timeout(default_timeout);
        return true;
    }
    else
    {
        parser.set_timeout(default_timeout);
        return false;
    }
}

/*
* Check a desirable respond from modem after sending a command (print respond)
*/
bool Modem::check_for_respond_fragmented(const char *value, int timeout)
{
    std::string respond = read_until(value, timeout);
    if (respond.find(value) != std::string::npos)
        return true;
    else
    {
        printf("fragment respond: %s\r\n", respond.c_str());
        return false;
    }
}

/*
* Check std::string ends with another std::string
*/
bool hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

/*
* Read serial until specific std::string
*/
std::string Modem::read_until(const char *value, int timeout)
{
    std::string respond = "";
    char ch[2] = {};
    Timer timer;
    timer.start();
    int start = timer.read_ms();
    while ((timer.read_ms() - start < timeout) && (!hasEnding(respond, value)))
    {
        if (parser.read(ch, 1) > 0)
        {
            ch[1] = '\0';
            respond.append(ch);
        }
    }
    timer.stop();
    return respond;
}

/*
* (There is a bug with Quectel MC60 which fails the http response and answer the failed response for a long time (approx 180 sec))
* Read serial until specific std::string for long time responds ), it also check for error value
*/
int Modem::read_until_forever(const char *value, const char *error, float read_until_forever_timeout)
{
    int result = 0;
    bool timeout = false;
    Timer timer;
    // char buffer[max_buffer_size];
    char *buffer = new char[64]();
    timer.start();
    float start = timer.read();
    while ((timer.read() - start < read_until_forever_timeout))
    {
        if (parser.recv("%[^\n]%*c", buffer))
        {
            if (strstr(buffer, value) != NULL)
            {
                result = 1;
                timeout = true;
                break;
            }
            else if (strstr(buffer, error) != NULL)
            {
                printf("Read Until Forever ERROR: %s\r\n", buffer);
                wait_ms(10);
                result = 0;
                timeout = true;
                break;
            }
            strcat(buffer, "\n");
        }
    }
    timer.stop();
    delete[] buffer;
    flush();
    if (!timeout)
        printf("Read Until Forever TIMEOUT!\r\n");
    return result;
}

/*
* Send SMS
*/
bool Modem::send_sms(const char *number, const char *msg)
{
    char ctrlZ[2] = {0x1a, 0x00};
    parser.send("AT+CMGS=\"%s\"", number);
    wait_ms(100);
    parser.send(msg);
    parser.send(ctrlZ);
    return check_for_respond("OK\r\n", 30000);
}

/*
* Read std::string as double_string type
*/
double_string Modem::read_sms(int index)
{
    double_string out = {"", ""};
    parser.send("AT+CMGR=%d", index);
    std::string _buffer = read_until();
    // printf("Here: %s\r\n", _buffer.c_str());
    //find number
    unsigned int pos1 = _buffer.find("\",\"");
    if (pos1 == std::string::npos)
        return out;
    unsigned int pos2 = _buffer.find("\",\"", pos1 + 1);
    if (pos2 == std::string::npos)
        return out;
    out.a = _buffer.substr(pos1 + 3, pos2 - pos1 - 3);
    //find message
    if ((pos1 = _buffer.find("+CMGR: ")) != std::string::npos)
    {
        pos2 = _buffer.find("\r\n", pos1 + 1);
        //find OK
        pos1 = _buffer.find("\r\nOK", pos2 + 2);
        if ((pos1 != std::string::npos) && (pos2 != std::string::npos))
        {
            pos2 = pos2 + 2;
            pos1 = pos1 - 2;
            out.b = _buffer.substr(pos2, pos1 - pos2);
        }
    }
    return out;
}

/*
* Check for unread messages
*/
int Modem::check_message(int *output, int size)
{
    parser.send("AT+CMGL=\"REC UNREAD\"");
    // parser.send("AT+CMGL=\"REC READ\"");
    std::string resp = read_until("OK\r\n", 5000);
    // printf("resp: %s\r\n", resp.c_str());
    int i = 0, j = 0, cnt = 0;
    std::string::size_type pos;
    while (((pos = resp.find("+CMGL: ", i)) != std::string::npos) && cnt < size)
    {
        j = resp.find(',', pos);
        pos = pos + 7;
        output[cnt] = atoi(resp.substr(pos, j - pos).c_str());
        i = j;
        cnt++;
    }
    return cnt;
}

/*
* Delete all sms messages in inbox
*/
bool Modem::del_sms_all() // delete all sms in storage
{
    parser.send("AT+CMGD=1,4");
    return check_for_respond();
}

/*
* Delete specified sms messages
*/
bool Modem::del_sms(int index) //delete for number
{
    parser.send("AT+CMGD=%d", index);
    return check_for_respond();
}

/*
* check whether inbox is full for specific threshold value
*/
bool Modem::check_inbox_full(int threshold)
{
    parser.send("AT+CPMS?");
    std::string _buffer = read_until();
    unsigned int pos = _buffer.find(",");
    unsigned int pos2 = _buffer.find(",15,\"SM\"");
    ;
    int number = 0;
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        number = atoi(_buffer.substr(pos + 1, pos2 - pos).c_str());
    }
    if (number > threshold)
        return true;
    else
        return false;
}

/*
* check whether inbox is full for specific threshold value
*/
//TODO: add other OP functions
int Modem::balance(void)
{
    int balance = -1;
    // check operator
    if (_current_op == OP_MTN) //Irancell
    {
        std::string respond = ussd("*555*1*2#");
        if (respond.empty())
            return balance;
        unsigned int pos = respond.find("is ");
        unsigned int pos2 = respond.find(" Rials");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
        {
            pos = pos + 3;
            balance = atoi(respond.substr(pos, pos2 - pos).c_str());
            return balance;
        }
    }
    else if (_current_op == OP_MCI) //MCI
    {
    }
    //Rightell

    return balance;
}

/*
* Execute USSD code
*/
std::string Modem::ussd(const char *code, int timeout)
{
    std::string buffer;
    unsigned int pos = 0;
    unsigned int pos2 = 0;
    parser.send("AT+CUSD=1,\"%s\"", code);
    buffer = read_until("\r\nOK", timeout);
    if (buffer.empty())
        return "";
    // printf("buffer: %s\r\n", buffer.c_str());
    //std::string encoding
    if (buffer.find("+CUSD: 2") != std::string::npos)
    {
        pos = buffer.find("+CUSD:") + 10;
        pos2 = buffer.find("\",15");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
            return buffer.substr(pos, pos2 - pos);
    }
    else if (buffer.find("+CUSD: 1") != std::string::npos)
    {
        pos = buffer.find("+CUSD:") + 10;
        pos2 = buffer.find("\",72");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
            return buffer.substr(pos, pos2 - pos);
    }
    return "";
}

/*
* Initialize GPRS network connection
*/
bool Modem::init_network(void)
{
    parser.send("AT+QIFGCNT=0");
    if (!check_for_respond())
        return false;
    wait_ms(100);
    if (!set_apn())
        return false;
    wait_ms(100);
    parser.send("AT+QIREGAPP");
    return check_for_respond("OK", 4000);
}

/*
* Initialize GPRS network connection QIACT
*/
bool Modem::init_network_qiact(int timeout)
{
    parser.send("AT+QIACT");
    return check_for_respond("OK\r\n", timeout);
}

/*
* Initialize GPRS network connection QIDEACT
*/
bool Modem::init_network_qideact(int timeout)
{
    flush();
    for (int i = 0; i < 3; i++)
    {
        parser.send("AT+QIDEACT");
        if (check_for_respond("DEACT OK\r\n", timeout))
            return true;
    }
    return false;
}

/*
* Set APN GPRS
*/
bool Modem::set_apn(void)
{
    char apn[32];
    if (_current_op == OP_MTN)
    {
#ifndef USE_CUSTOM_APN
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MTN_APN);
#else
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MTN_CUSTOM_APN);
#endif
        parser.send(apn);
        return check_for_respond();
    }
    else if (_current_op == OP_MCI)
    {
#ifndef USE_CUSTOM_APN
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MCI_APN);
#else
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MCI_CUSTOM_APN);
#endif
        parser.send(apn);
        return check_for_respond();
    }
    return false;
}

/*
* Return modem signal quality (0 to 5)
*/
int Modem::get_signal_quality(void)
{
    parser.send("AT+CSQ");
    const int max_quality = 31;
    std::string respond = read_until();
    // printf("respond: %s\r\n", respond.c_str());
    if (respond.empty())
        return -1;
    unsigned int pos = respond.find("+CSQ: ");
    unsigned int pos2 = respond.find(",");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 6;
        respond = respond.substr(pos, pos2 - pos);
        int quality = atoi(respond.c_str());
        float temp = static_cast<float>(quality) / static_cast<float>(max_quality);
        quality = roundf(5 * temp);
        return quality;
    }
    return -1;
}

/*
* HTTP GET Request
*/
std::string Modem::http_get(const char *url, int timeout)
{
    std::string respond;
    parser.send("AT+QHTTPURL=%d", strlen(url));
    if (!check_for_respond_fragmented("CONNECT", 10000))
    {
        printf("fragment error\r\n");
        return "";
    }
    parser.send(url);
    if (!check_for_respond("OK", 2000))
    {
        init_network_qideact();
        return "";
    }
    parser.send("AT+QHTTPGET=%d", timeout / 1000);
    if (!read_until_forever("OK"))
    {
        init_network_qideact();
        return "";
    }
    parser.send("AT+QHTTPREAD=%d", timeout / 1000);
    respond = read_until("OK\r\n", timeout);
    //extract respond
    unsigned int pos = respond.find("CONNECT");
    unsigned int pos2 = respond.rfind("\r\nOK\r\n");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 9;
        respond = respond.substr(pos, pos2 - pos);
    }
    init_network_qideact();
    return respond;
}

/*
* HTTP POST Request (with support of large post body)
*/
std::string Modem::http_post(const char *url, const char *body, int blen, int timeout)
{
    parser.send("AT+QHTTPURL=%d", strlen(url));
    if (!check_for_respond_fragmented("CONNECT", 10000))
    {
        printf("fragment error\r\n");
        return "";
    }
    parser.send(url);
    if (!check_for_respond("OK", 2000))
    {
        init_network_qideact();
        return "";
    }
    parser.send("AT+QHTTPPOST=%d,200,%d", ((blen > 0) ? blen : 1), timeout / 1000);
    if (!read_until_forever("CONNECT"))
    {
        init_network_qideact();
        return "";
    }
    if (blen <= HTTP_POST_SIZE_LIMIT)
    {
        if (blen > 0)
            parser.write(body, blen);
        else
            parser.write("\n", 2);
    }
    else
    {
        // buffer is large
        int bytes_written = 0;
        int bb = 0;
        char *buffer = new char[HTTP_POST_CHUNK]();
        int buff_len = HTTP_POST_CHUNK - 1; // doesn't count terminator
        // split body to small chunks
        for (int i = 0; i < blen / buff_len; ++i)
        {
            memcpy(buffer, body + (i * buff_len), buff_len);
            bb = parser.write(buffer, HTTP_POST_CHUNK - 1);
            bytes_written += bb;
            wait_ms(CHUNK_DELAY);
        }
        // if there is anything left over
        if (blen % buff_len)
        {
            strcpy(buffer, body + (blen - blen % buff_len));
            bb = parser.write(buffer, blen % (HTTP_POST_CHUNK - 1));
            bytes_written += bb;
            wait_ms(CHUNK_DELAY);
        }
        delete[] buffer;
    }
    if (!read_until_forever("OK"))
    {
        init_network_qideact();
        return "";
    }
    parser.send("AT+QHTTPREAD=%d", timeout / 1000);
    std::string respond = read_until("OK\r\n", timeout);
    // extract respond
    unsigned int pos = respond.find("CONNECT");
    unsigned int pos2 = respond.rfind("\r\nOK\r\n");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 9;
        respond = respond.substr(pos, pos2 - pos);
    }
    init_network_qideact();
    return respond;
}

/*
* Upload a file to modem
*/
bool Modem::upload_file(const char *name, int size, const char *data)
{
    parser.send("AT+QFUPL=\"%s\",%d", name, size);
    if (!check_for_respond("CONNECT", 3000))
        return false;
    parser.send(data);
    if (!check_for_respond())
        return false;
    return true;
}

/*
* Download a file from modem
*/
std::string Modem::download_file(const char *name)
{
    std::string data;
    parser.send("AT+QFDWL=\"%s\"", name);
    wait_ms(30);
    data = read_until("\nOK");
    if (data.empty())
        return "";
    //extract data
    unsigned int pos = data.find("CONNECT");
    unsigned int pos2 = data.rfind("+QFDWL") - 1;
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 9;
        return data.substr(pos, pos2 - pos);
    }
    return "";
}

/*
* Delete a file in modem
*/
bool Modem::delete_file(const char *name)
{
    parser.send("AT+QFDEL=\"%s\"", name);
    return check_for_respond();
}

/*
* Check file exist in modem
*/
bool Modem::file_exist(const char *name)
{
    parser.send("AT+QFLST");
    std::string resp = read_until("OK\r\n", 3000);
    if (resp.find(name) != std::string::npos)
        return true;
    else
        return false;
}

/*
* Turn on GPS (ONLY for ALL IN ONE)
*/
bool Modem::gps_turn_on(void)
{
    parser.send("AT+QGNSSC=1");
    return check_for_respond();
}

/*
* Turn off GPS (ONLY for ALL IN ONE)
*/
bool Modem::gps_turn_off(void)
{
    parser.send("AT+QGNSSC=0");
    return check_for_respond();
}

/*
* Check timezone for Daylight Saving Time (DST)
*/
bool Modem::IsDST(int month, int day)
{
    //Tehran DST (+1):
    //start 22 march (3)
    //end 22 sep (9)
    //January, february, are out.
    if (month < 3 || month > 9)
        return false;
    //march to spe are in
    if (month > 3 && month < 9)
        return true;

    if (month == 3)
    {
        if (day >= 22)
            return true;
        else
            return false;
    }
    else if (month == 9)
    {
        if (day >= 22)
            return false;
        else
            return true;
    }
    return false;
}

/*
* Time conversion function
*/
time_t Modem::time_convert(time_t t0, char const *tz_value)
{
    char old_tz[64];
    strcpy(old_tz, getenv("TZ"));
    setenv("TZ", tz_value, 1);
    tzset();
    char new_tz[64];
    strcpy(new_tz, getenv("TZ"));
    struct tm *lt = localtime(&t0);
    setenv("TZ", old_tz, 1);
    tzset();
    return mktime(lt);
}

/*
* Get current time from modem
*/
time_t Modem::current_time(void)
{
    time_t epoch = 0;
    parser.send("AT+CCLK?");
    std::string resp = read_until("OK\r\n");
    // printf("Time: %s\r\n", resp.c_str());
    // extract time std::string
    unsigned int pos = resp.find("+CCLK: \"");
    unsigned int pos2 = resp.rfind("\"\r\n");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        std::string time_str = resp.substr(pos + 8, pos2 - pos - 11);
        // printf("time_str: %s\r\n", time_str.c_str());

        //convert to timestamp
        int hh, mm, ss, yy, mon, day;
        struct tm when = {0};

        sscanf(time_str.c_str(), "%d/%d/%d,%d:%d:%d", &yy, &mon, &day, &hh, &mm, &ss);
        // printf("yy:%d, mon:%d, day:%d, hh:%d, mm:%d, ss:%d\r\n", yy, mon, day, hh, mm, ss);
        when.tm_hour = hh;
        when.tm_min = mm;
        when.tm_sec = ss;
        when.tm_year = 2000 + yy - 1900;
        when.tm_mon = mon - 1;
        when.tm_mday = day;
        epoch = mktime(&when);
        if (IsDST(mon, day))
            // epoch = time_convert(epoch, "IST+4:30");
            epoch -= ((3600 * 4) + (30 * 60));
        else
            // epoch = time_convert(epoch, "IST+3:30");
            epoch -= ((3600 * 3) + (30 * 60));
    }
    return epoch;
}

#ifdef STAND_ALONE

/*
* Get gps coordinates (ONLY for STAND ALONE)
*/
gps_data Modem::get_gps_data(int duration)
{
    const int default_value = -1;
    gps_data location = {default_value, default_value, default_value, default_value, default_value, default_value};
    for (int i = 0; i < duration; i++)
    {
        gps.encode(fgetc(gps_stream));
    }

    if (gps.location.isValid())
    {
        // printf("lat: %f, long: %f\n", gps.location.lat(), gps.location.lng());
        // location = {gps.location.lat(), gps.location.lng()};
        location.lat = gps.location.lat();
        location.lng = gps.location.lng();
    }
    // else
    //     printf("GPS chars processed:  %d\n\r", gps.charsProcessed());
    if (gps.speed.isValid())
        location.speed = gps.speed.kmph();
    if (gps.altitude.isValid())
        location.altitude = gps.altitude.meters();
    if (gps.course.isValid())
        location.course = gps.course.deg();
    if (gps.hdop.isValid())
    {
        double hdop = gps.hdop.hdop();
        if (hdop <= 0)
            location.hdop = default_value;
        else if (hdop > _hdop_threshold)
            location.hdop = _hdop_threshold;
        else
            location.hdop = hdop;
    }
    return location;
}

/*
* GPS Speed calculation
*/
double Modem::speed_calc(std::vector<double> speeds)
{
    //minimum value for zeroes
    const float threshold = 0.6;
    int zero_count = 0;
    double speed_sum = 0;
    double zero_threshold = 2;
    //counting zeros
    //printf("speed: ");
    for (std::vector<double>::iterator speed = speeds.begin(); speed != speeds.end(); ++speed)
    {
        //printf("%.3f, ", *speed);
        speed_sum += *speed;
        if (*speed < zero_threshold)
            zero_count++;
    }
    //printf("\r\n");
    // printf("zero_count: %d, speeds_size: %d, zero_count/speeds.size(): %f\r\n", zero_count, speeds.size(), (float) zero_count/(float) speeds.size());
    // decide if zeros is higher than threshold then return zero else get average
    if ((static_cast<float>(zero_count) / static_cast<float>(speeds.size())) >= threshold)
        return 0;
    else
    {
        return speed_sum / (double)speeds.size();
    }
}

/*
* Get gps coordinates (ONLY for STAND ALONE) for specific time (1seconds)
*/
gps_data Modem::get_gps_data_fix(gps_data *loc)
{
    const int count = 5;
    const int duration = 512;
    const int default_value = -1;
    double minimum_hdop = _hdop_threshold;
    std::vector<double> speeds;
    typedef struct
    {
        gps_data location_sum;
        int loc_cnt;
        int speed_cnt;
        int altitude_cnt;
        int course_cnt;
        int hdop_cnt;
    } location_avg;
    gps_data current_location, location = {default_value, default_value, default_value, default_value, default_value, default_value};
    location_avg location_sum = {{0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0};

    for (int i = 0; i < count; i++)
    {
        current_location = get_gps_data(duration);
        if (current_location.hdop == default_value)
            continue;

        if (current_location.hdop < minimum_hdop)
        {
            minimum_hdop = current_location.hdop;
            location_sum.location_sum.lat = current_location.lat;
            location_sum.location_sum.lng = current_location.lng;
            location_sum.loc_cnt = 1;

            location_sum.location_sum.altitude = current_location.altitude;
            location_sum.altitude_cnt = 1;
            location_sum.location_sum.course = current_location.course;
            location_sum.course_cnt = 1;
        }
        else if (current_location.hdop == minimum_hdop)
        {
            location_sum.location_sum.lat += current_location.lat;
            location_sum.location_sum.lng += current_location.lng;
            location_sum.loc_cnt++;

            location_sum.location_sum.altitude += current_location.altitude;
            location_sum.altitude_cnt++;
            location_sum.location_sum.course += current_location.course;
            location_sum.course_cnt++;
        }

        if (current_location.speed != default_value)
        {
            speeds.push_back(current_location.speed);
        }
        // update the values in loc pointer
        if (loc)
        {
            // printf("loc: %f, %f, spd: %f, alt: %f, course: %f, hdop: %f\r\n", current_location.lat, current_location.lng, current_location.speed, current_location.altitude, current_location.course, current_location.hdop);
            loc->lat = current_location.lat;
            loc->lng = current_location.lng;
            loc->altitude = current_location.altitude;
            loc->course = current_location.course;
            loc->speed = current_location.speed;
            loc->hdop = current_location.hdop;
        }
    }

    location.hdop = minimum_hdop;
    location.lat = (location_sum.loc_cnt > 0) ? location_sum.location_sum.lat / location_sum.loc_cnt : default_value;
    location.lng = (location_sum.loc_cnt > 0) ? location_sum.location_sum.lng / location_sum.loc_cnt : default_value;
    location.altitude = (location_sum.altitude_cnt > 0) ? location_sum.location_sum.altitude / location_sum.altitude_cnt : default_value;
    location.course = (location_sum.course_cnt > 0) ? location_sum.location_sum.course / location_sum.course_cnt : default_value;

    if (speeds.size())
        // calculating speed
        location.speed = speed_calc(speeds);
    return location;
}

#else
/*
* Get nmea std::string (ONLY for ALL IN ONE)
*/
std::string Modem::get_nmea(void)
{
    std::string respond;
    parser.send("AT+QGNSSRD?");
    respond = read_until("OK", 1000);
    if (respond.find("ERROR") != std::string::npos)
        return "";
    flush();
    unsigned int pos = respond.find("$GNRMC");
    unsigned int pos2 = respond.find("\nOK") - 1;
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
        return respond.substr(pos, pos2 - pos);
    else
        return ""
}

/*
* Get gps data coordinates (ONLY for ALL IN ONE)
*/
gps_data Modem::get_gps_data(int timeout)
{
    gps_data location = {0, 0};
    // TinyGPSPlus gps;
    //setting up gps
    std::string data;

    wait(1);
    char _data[300];
    int _timeout = 0;
    while (_timeout < timeout * 1000000)
    {
        data = get_nmea();
        // printf("data: %d\r\n", data.length());
        if (data.length() == 0)
            continue;
        // printf("%s\r\n", data.c_str());
        strncpy(_data, data.c_str(), sizeof(_data));
        _data[sizeof(_data) - 1] = 0;
        for (unsigned int i = 0; i < sizeof(_data); i++)
        {
            if (_data[i] != '\0')
                gps.encode(_data[i]);
            // _data[i] = '\0';
        }
        memset(_data, '\0', sizeof(_data));
        // printf("GPS chars processed:  %d\n\r", gps.charsProcessed());
        if (gps.location.isValid())
        {
            if (gps.speed.isValid())
                location.speed = gps.speed.kmph();
            // printf("lat: %f, long: %f\n", gps.location.lat(), gps.location.lng());
            location = {gps.location.lat(), gps.location.lng()};
            return location;
        }
        _timeout++;
    }
    return location;
}
#endif
