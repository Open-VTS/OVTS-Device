/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Zip Compression library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/
#ifndef gzip_h
#define gzip_h

#include "mbed.h"
#include "zlib.h"


class ZLib
{
public:
    static int zip(unsigned char *dst, unsigned long *dst_length, unsigned char *src, unsigned long src_length);
    static int unzip(unsigned char *dst, unsigned long *dst_length, unsigned char *src, unsigned long src_length);
    static int compress_file(const char* src, const char* dest);
    static int decompress_file(const char* src, const char* dest);
private:
};




























#endif