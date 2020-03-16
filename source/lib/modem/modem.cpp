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
Modem::Modem(PinName Tx, PinName Rx, PinName GPS_Tx, PinName GPS_Rx, PinName pw, PinName sw, const double hdop_threshold) : modem_serial(Tx, Rx, 115200), parser(&modem_serial, "\r\n"), gps_serial(GPS_Tx, GPS_Rx, 115200), _power(pw), _switch(sw)
{
    _hdop_threshold = hdop_threshold;
    // parser.debug_on(1);
    parser.set_timeout(parser_default_timeout);
    gps_stream = fdopen(&gps_serial, "w+");
}

#else
/*
* Destructor
*/
Modem::Modem(PinName Tx, PinName Rx, PinName pw, PinName sw) : modem_serial(Tx, Rx), _power(pw), _switch(sw)
{
    _hdop_threshold = hdop_threshold;
    modem_serial.baud(115200);
}
#endif

/*
* Constructor for All in one mode
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
    wait_ms(1000);
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
* Reboot Modem
*/
void Modem::reboot(void)
{
    power_off();
    wait_ms(100);
    power_on();
}

void Modem::soft_reboot(void)
{
    parser.send("AT+QPOWD=0");
    check_for_respond();
    wait_ms(200);
    _switch = 1;
    wait_ms(1100);
    _switch = 0;
    wait_ms(1000);
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
/*
bool Modem::check_gsm_status(int count)
{
    for (int i=0;i<count; i++)
    {
        parser.send("AT+QNSTATUS");
        if (check_for_respond("+QNSTATUS: 0\r\nOK", 100))
        {
            return true;
        }
        wait_ms(1000);
    }
    return false;
}
*/

bool Modem::check_gsm_status(void)
{
    // check default home network
    parser.send("AT+CGREG?");
    std::string resp = read_until();
    if ((resp.find("+CGREG: 0,1") != std::string::npos) || (resp.find("+CGREG: 0,5") != std::string::npos))
        return true;
}

bool Modem::check_sim_status(void)
{
    parser.send("AT+CPIN?");
    return check_for_respond(5000);
}

/*
* Reinitialize modem
*/
bool Modem::reinit(void)
{
    // soft_reboot();
    reboot();
    while (!ping())
        ;
    while (!init())
        ;
    while (!init_network())
        ;
    return true;
}

/*
* Initialize modem
*/
bool Modem::init(void)
{
    if (!check_gsm_status())
    {
        // printf("GSM Status Failed\r\n");
        return false;
    }
    //put some delay to make sure op can set
    wait_ms(200);
    if (!set_operator())
    {
        return false;
    }
    wait_ms(200);
    if (!set_params())
    {
        printf("set params failed\r\n");
        // return false;
    }
    wait_ms(200);
    return true;
}

/*
* Set Operator name
*/
bool Modem::set_operator(void)
{
    parser.send("AT+COPS?");
    std::string respond = read_until();
    // printf("OP: %s|\r\n", respond.c_str());
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
    //setting language to EN for MCI
    else if (_current_op == OP_MCI && (!ussd("*198*2#").empty()))
        return true;
    return false;
}

/*
* Check a desirable respond from modem after sending a command
*/
bool Modem::check_for_respond(const char *value, int timeout)
{
    parser.set_timeout(timeout);
    bool res = parser.recv(value);
    parser.set_timeout(parser_default_timeout);
    return res;
}

int Modem::check_for_respond_w_err(const char *resp, const char *error, int timeout)
{
    Timer timer;
    int res = 0;
    int ch = 0;
    timer.start();
    std::string respond = "";
    float start = timer.read();
    while (timer.read_ms() - start < timeout)
    {
        if (respond.find(resp) != std::string::npos)
        {
            res = 1;
            break;
        }
        else if (respond.find(error) != std::string::npos)
        {
            // continue reading error
            while ((ch = parser.getc()) != EOF)
                respond.push_back(ch);
            printf("CHECK FOR RESPOND ERROR: %s\r\n", respond.c_str());
            res = -1;
            break;
        }
        if ((ch = parser.getc()) != EOF)
            respond.push_back(ch);
    }
    if (res == 0)
        printf("CHECK FOR RESPOND TIMEOUT! \"%s\"\r\n", respond.c_str());
    timer.stop();
    flush();
    return res;
}

void Modem::test_socket(void)
{
    printf("264\r\n");
    // init_network();
    parser.send("AT+QIMUX=1");
    printf("267: %d\r\n", check_for_respond(2000));
    parser.send("AT+QIOPEN=1,\"TCP\",\"X.X.X.X\",8080");
    printf("269: %d\r\n", check_for_respond(OK, 20000));
    wait_ms(500);
    // parser.send("AT+QISEND=1,5");
    // printf("272: %s\r\n", check_for_respond_w_err("1, CONNECT OK", "ERROR", 40000));
    parser.send("AT+QISEND");
    wait(1);
    flush();
    parser.send("hello");
    char ctrlZ[2] = {0x1a, 0x00};
    parser.send(ctrlZ);
    printf("273: %d\r\n", check_for_respond("CONNECT OK", 40000));
    parser.send("AT+QICLOSE=1");
}

void Modem::test()
{
    while (!init())
        ;
    while (!init_network())
        ;
    printf("res: %s\r\n", http_get("https://185.153.185.219", 5000).c_str());
    printf("Test Done\r\n");
}

/*
* Check std::string ends with another std::string
*/
bool Modem::hasEnding(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    return false;
}

/*
* Read serial until specific std::string
*/

std::string Modem::read_until(const char *value, int timeout)
{
    std::string respond = "";
    int ch = 0;
    Timer timer;
    timer.start();
    int start = timer.read_ms();
    while ((timer.read_ms() - start < timeout) && (!hasEnding(respond, value)))
    {
        if ((ch = parser.getc()) != EOF)
            respond.push_back(ch);
        wait_us(100);
    }
    timer.stop();
    flush();
    if (hasEnding(respond, value))
        return respond;
    else if (respond.length())
        printf("Failed read until: \"%s\"\r\n", respond.c_str());
    return "";
}

/*
std::string Modem::read_until(const char *value, int timeout)
{
    std::string respond = "";
    Timer timer;
    timer.start();
    int start = timer.read_ms();
    char *buffer = new char[max_buffer_size]();
    while ((timer.read_ms() - start < timeout) && (!hasEnding(respond, value)))
    {
        if (parser.recv("%[^\n]%*c", buffer))
        {
            respond.append(buffer);
            respond.append("\r\n");
        }
    }
    flush();
    timer.stop();
    delete[] buffer;
    return respond;
}
*/

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
    return check_for_respond(30000);
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
    size_t pos1 = _buffer.find("\",\"");
    if (pos1 == std::string::npos)
        return out;
    size_t pos2 = _buffer.find("\",\"", pos1 + 1);
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
    std::string resp = read_until(OK, 5000);
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
    size_t pos = _buffer.find(",");
    size_t pos2 = _buffer.find(",15,\"SM\"");
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
    //check operator
    if (_current_op == OP_MTN) //Irancell
    {
        std::string respond = ussd("*555*1*2#");
        if (respond.empty())
            return balance;
        size_t pos = respond.find("is ");
        size_t pos2 = respond.find(" Rials");
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
        std::string respond = ussd("*140*11#");
        if (respond.empty())
            return balance;
        size_t pos = respond.find("is ");
        size_t pos2 = respond.find(" Rial");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
        {
            pos = pos + 3;
            balance = atoi(respond.substr(pos, pos2 - pos).c_str());
            return balance;
        }
    }
    //Rightell

    return balance;
}

/*
* Read IMSI of SIMCard
*/
std::string Modem::read_imsi(void)
{
    std::string respond = "";
    parser.send("AT+CIMI");
    respond = read_until();
    size_t pos = respond.find("AT+CIMI");
    size_t pos2 = respond.find("\r\nOK");
    // printf("pos: %d, pos2: %d\r\n", pos, pos2);
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 10;
        return respond.substr(pos, pos2 - pos - 1);
    }
    return "";
}

/*
* Execute USSD code
*/
std::string Modem::ussd(const char *code, int timeout)
{
    std::string buffer;
    size_t pos = 0;
    size_t pos2 = 0;
    parser.send("AT+CUSD=1,\"%s\"", code);
    buffer = read_until("\r\nOK", timeout);
    if (buffer.empty())
        return "";
    // printf("buffer: %s\r\n", buffer.c_str());
    if (buffer.find("+CUSD: 2") != std::string::npos)
    {
        pos = buffer.find("+CUSD:");
        pos2 = buffer.find("\",15");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
        {
            pos = pos + 10;
            return buffer.substr(pos, pos2 - pos);
        }
    }
    else if (buffer.find("+CUSD: 1") != std::string::npos)
    {
        pos = buffer.find("+CUSD:");
        pos2 = buffer.find("\",72");
        // printf("pos: %d, pos2: %d\r\n", pos, pos2);
        if ((pos != std::string::npos) && (pos2 != std::string::npos))
        {
            pos = pos + 10;
            return buffer.substr(pos, pos2 - pos);
        }
    }
    return "";
}

/*
* Initialize GPRS network connection
*/
bool Modem::init_network(void)
{
    init_network_qideact();
    parser.send("AT+QIFGCNT=0");
    if (!check_for_respond())
        return false;

    parser.send("AT+QICSGP=1");
    if (!check_for_respond())
        return false;

    parser.send("AT+CGDCONT=1");
    if (!check_for_respond())
        return false;

    parser.send("AT+QIMODE=0");
    if (!check_for_respond())
        return false;

    //TODO:check this
    parser.send("AT+QIMUX=1");
    if (!check_for_respond())
        return false;

    parser.send("AT+QIREGAPP");
    if (!check_for_respond(5000))
        return false;

    parser.send("AT+QIACT");
    if (!check_for_respond(5000))
        return false;

#ifdef USE_HTTPS
    return config_https();
#endif
    return true;
}

/*
* Initialize GPRS network connection QIDEACT
*/
bool Modem::init_network_qideact(int timeout)
{
    parser.send("AT+QIDEACT");
    return check_for_respond("DEACT OK\r\n", timeout);
}

/*
* Set APN GPRS
*/
bool Modem::set_apn(void)
{
    char apn[32];
    if (_current_op == OP_MTN)
    {
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MTN_APN);
        parser.send(apn);
        return check_for_respond();
    }
    else if (_current_op == OP_MCI)
    {
        sprintf(apn, "AT+QICSGP=1,\"%s\"", OP_MCI_APN);
        parser.send(apn);
        return check_for_respond();
    }
    return false;
}

/*
* Setup HTTPS as default connection
*/
bool Modem::config_https(void)
{
    parser.send("AT+QSSLCFG=\"sslversion\",1,4");
    if (!check_for_respond())
        return false;

    parser.send("AT+QSSLCFG=\"seclevel\",1,0");
    if (!check_for_respond())
        return false;

    parser.send("AT+QSSLCFG=\"ignorertctime\",1");
    if (!check_for_respond())
        return false;

    parser.send("AT+QSSLCFG=\"https\",1");
    if (!check_for_respond())
        return false;

    parser.send("AT+QSSLCFG=\"httpsctxi\",1");
    if (!check_for_respond())
        return false;

    return true;
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
    size_t pos = respond.find("+CSQ: ");
    size_t pos2 = respond.find(",");
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
    int temp = 0;
    std::string respond;
    parser.send("AT+QHTTPURL=%d,12", strlen(url));
    if ((temp = check_for_respond_w_err("CONNECT", MODEM_NETWORK_ERROR, 180 * 1000)) <= 0)
    {
        printf("CONNECT ERROR!\r\n");
        if (temp == 0)
        {
            printf("Rebooting...\r\n");
            reinit();
            printf("Done\r\n");
        }
        return "";
    }
    wait_ms(200);
    parser.send("%s", url);
    if (!check_for_respond("OK", 2000))
    {
        return "";
    }
    parser.send("AT+QHTTPGET=%d", timeout / 1000);
    if ((temp = check_for_respond_w_err(OK, MODEM_NETWORK_ERROR, 180 * 1000)) <= 0)
    {
        return "";
    }
    parser.send("AT+QHTTPREAD=%d", timeout / 1000);
    respond = read_until(OK, timeout);
    //extract respond
    size_t pos = respond.find("CONNECT");
    size_t pos2 = respond.rfind("\r\nOK\r\n");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 9;
        respond = respond.substr(pos, pos2 - pos);
    }
    else
        respond = "";
    return respond;
}

/*
* HTTP POST Request (with support of large post body)
*/
std::string Modem::http_post(const char *url, const char *body, int blen, int timeout)
{
    // check if blen is below 29696 byte (MODEM DOESNT SUPPORT LARGER DATA)
    if (blen > 29696)
    {
        printf("HTTP POST SIZE LIMIT REACHED!\r\n");
        return "";
    }
    int temp = 0;
    parser.send("AT+QHTTPURL=%d", strlen(url));
    // if (!check_for_respond("CONNECT", MODEM_NETWORK_ERROR, timeout))
    if ((temp = check_for_respond_w_err("CONNECT", MODEM_NETWORK_ERROR, 180 * 1000)) <= 0)
    {
        printf("CONNECT ERROR!\r\n");
        if (temp == 0)
        {
            printf("Rebooting...\r\n");
            reinit();
            printf("Done\r\n");
        }
        return "";
    }
    wait_ms(200);
    parser.send("%s", url);
    if (!check_for_respond("OK", 4000))
    {
        printf("URL ERROR!\r\n");
        return "";
    }
    parser.send("AT+QHTTPPOST=%d,%d,%d", ((blen > 0) ? blen : 1), (timeout / 1000) + 2, timeout / 1000);
    printf("619\r\n");
    // if (!read_until_forever("CONNECT"))
    if ((temp = check_for_respond_w_err("CONNECT", MODEM_NETWORK_ERROR, 180 * 1000)) <= 0)
    {
        if (temp == 0)
        {
            printf("Rebooting...\r\n");
            reinit();
            printf("Done\r\n");
        }
        return "";
    }
    wait_ms(200);
    if (blen <= HTTP_POST_SIZE_LIMIT)
    {
        if (blen > 0)
            parser.write(body, blen);
        else
            parser.write("\n", 2);
    }
    else
    {
        //buffer is large
        int bytes_written = 0;
        int bb = 0;
        char *buffer = new char[HTTP_POST_CHUNK]();
        // printf("input body_length: %d\r\n", blen);
        // printf("FULL BODY: %d, %s\r\n", strlen(body), body);
        int buff_len = HTTP_POST_CHUNK - 1; // doesn't count terminator
        // printf("Body: ");
        //split body to small chunks
        for (int i = 0; i < blen / buff_len; ++i)
        {
            memcpy(buffer, body + (i * buff_len), buff_len);
            bb = parser.write(buffer, HTTP_POST_CHUNK - 1);
            bytes_written += bb;
            wait_ms(CHUNK_DELAY);
            // printf("%s\r\n", buffer);
        }
        // if there is anything left over
        if (blen % buff_len)
        {
            strcpy(buffer, body + (blen - blen % buff_len));
            // bb = parser.printf("%s", buffer);
            bb = parser.write(buffer, blen % (HTTP_POST_CHUNK - 1));
            bytes_written += bb;
            wait_ms(CHUNK_DELAY);
            // printf("%s\r\n", buffer);
            // parser.send(buffer);
        }
        delete[] buffer;
        // printf("bytes_written: %d\r\n", bytes_written);
    }
    printf("665\r\n");
    wait_ms(200);
    // if (!read_until_forever("OK"))
    if ((temp = check_for_respond_w_err("OK", MODEM_NETWORK_ERROR, 180 * 1000)) <= 0)
    {
        if (temp == 0)
        {
            printf("Rebooting...\r\n");
            reinit();
            printf("Done\r\n");
        }
        return "";
    }
    parser.send("AT+QHTTPREAD=%d", timeout / 1000);
    std::string respond = read_until(OK, timeout);
    // extract respond
    size_t pos = respond.find("CONNECT");
    size_t pos2 = respond.rfind("\r\nOK\r\n");
    if ((pos != std::string::npos) && (pos2 != std::string::npos))
    {
        pos = pos + 9;
        respond = respond.substr(pos, pos2 - pos);
    }
    return respond;
}

/*
* Upload a file to modem
*/
bool Modem::upload_file(const char *name, int size, const char *data)
{
    parser.send("AT+QFUPL=\"%s\",%d", name, size);
    if (!check_for_respond("CONNECT", 3000))
    {
        return false;
    }
    parser.send(data);
    return check_for_respond();
}

/*
* Download a file from modem
*/
std::string Modem::download_file(const char *name)
{
    std::string data;
    parser.send("AT+QFDWL=\"%s\"", name);
    wait_response;
    data = read_until();
    // printf("HELLO::\r\n");
    // printf("here: %s\r\n", data.c_str());
    if (data.empty())
        return "";
    //extract data
    size_t pos = data.find("CONNECT");
    size_t pos2 = data.rfind("+QFDWL") - 1;
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
    std::string resp = read_until(OK, 3000);
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
* CHECK timezone for Daylight Saving Time (DST)
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
    std::string resp = read_until();
    // printf("Time: %s\r\n", resp.c_str());
    // extract time std::string
    size_t pos = resp.find("+CCLK: \"");
    size_t pos2 = resp.rfind("\"\r\n");
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
    const int count = 3;
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
    respond = read_until();
    if (respond.find("ERROR") != std::string::npos)
        return "";
    flush();
    size_t pos = respond.find("$GNRMC");
    size_t pos2 = respond.find("\nOK") - 1;
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
