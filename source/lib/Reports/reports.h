/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Reports library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*  Reports
*/
#ifndef reports_h
#define reports_h
#include "mbed.h"
#include "string"
#include "vector"
#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "deviceproto.h"
#include "mbedZlib.h"
#include "utility.h"

#define SD_ROOT "/SD/%d"

#define COMPRESS_PREFIX ".gz"
#define UNCOMPRESS_PREFIX ".txt"
//devicereport prefix to save files
// #define REPROT_PREFIX "%Y-%m-%d-%H-%M"
#define REPROT_PREFIX "%Y-%m-%d-%H-%M-%S"

#define MAX_BUFFSIZE PROTO_MAX_BUF_SIZE //maximum b64 buffersize

class SDCard
{
public:
  SDCard(PinName MOSI, PinName MISO, PinName CLK, PinName CS);
  bool mount(void);
  bool unmount(void);
  bool create_dir(const char *dir);
  bool mkdir_rec(const char *dir);
  bool file_exist(const char *filepath);
  bool dir_exist(const char *path);
  bool remove_file(const char *filepath);
  bool remove_files_in_dir(const char *dir);
  std::vector<std::string> list_dir(const char *path);
  long file_size(const char *path);
  bool search_file(const char *path, const char *search_term, char *ouput);

  int read_file_binary(const char *file_path, unsigned char *file_buffer, long size);
  bool read_file_text(const char *file_path, char *file_buffer, long size);
  bool write(const char *file, const char *str, const char *mode = "a");
  bool write_center_commands(CenterCommands *center_commands);
  bool check(const char *path);

private:
  const char *test_str;
  SDBlockDevice sd;
  // File system declaration
  FATFileSystem fs;
};

class DeviceReports
{
public:
  DeviceReports(SDCard *sd, int deviceid);
  bool init();
  bool check_missed_files(char *output);
  bool compress_file(const char *src, const char *dest, bool remove_file = true);
  bool write_reports(const char *file_path, DeviceData *devicedata);
  bool remove_reports(void);
  std::string file_path_gen(void);

private:
  void prepare_directories(void);
  DeviceProto _deviceproto;
  //return today time stamp X/X/X 00:00:00
  SDCard *_sd;
  char device_main_directory[32];
  char sd_check_filepath[32];
};

#endif
