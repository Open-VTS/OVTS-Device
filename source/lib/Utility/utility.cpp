/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Utility library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/
#include "utility.h"

void Utility::remove_string(std::string &str, const char *to_delete)
{
    std::string::size_type n = strlen(to_delete);
    for (std::string::size_type i = str.find(to_delete);
         i != std::string::npos;
         i = str.find(to_delete))
        str.erase(i, n);
}

void Utility::removeSubstr(char *s, const char *toremove)
{
    while ((s = strstr(s, toremove)))
        memmove(s, s + strlen(toremove), 1 + strlen(s + strlen(toremove)));
}

std::string Utility::read_between_quotes(std::string input)
{
    unsigned int pos = input.find("\"");
    if (pos == std::string::npos)
        return "";
    unsigned int pos2 = input.find("\"", pos + 1);
    if (pos2 == std::string::npos)
        return "";
    return input.substr(pos + 1, pos2 - pos - 1);
}

void Utility::str_prepend(unsigned char *s, const char *t)
{
    size_t len = strlen(t);
    size_t i;

    memmove(s + len, s, strlen(reinterpret_cast<const char *>(s)) + 1);

    for (i = 0; i < len; ++i)
    {
        s[i] = t[i];
    }
}

/*
std::vector<std::string> &Utility::split_helper(const std::string &s, const char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> Utility::split(const std::string &s, const char delim) {
	std::vector<std::string> elems;
	split_helper(s, delim, elems);
	return elems;
}
*/

Base64::Base64(void)
{
}
int Base64::encode(unsigned char *buffer, int buffer_size, const unsigned char *message, int message_length)
{
    size_t bytes_written = 0;
    int result = mbedtls_base64_encode(buffer, buffer_size, &bytes_written, message, static_cast<size_t>(message_length));
    if (result == 0 && bytes_written > 0)
        return 1;
    else
        return 0;
}

int Base64::decode(unsigned char *buffer, int buffer_size, const unsigned char *message)
{
    // printf("message: %s, length: %d\r\n", message, strlen((char *)message));
    size_t bytes_written = 0;
    int result = mbedtls_base64_decode(buffer, buffer_size, &bytes_written, message, strlen(reinterpret_cast<const char *>(message)));
    // printf("result: %d, bytes_written: %d\r\n", result, bytes_written);
    if (result == 0 && bytes_written > 0)
        return 1;
    else
    {
        printf("Base64 Decode Failed\r\n");
        return 0;
    }
}

int Base64::encode_size(int input_size)
{
    return ((input_size * 4) / 3) + (input_size / 96) + 6;
}
