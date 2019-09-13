/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Reports library
*  Author: M.Rahimi <work.rahimi@gmail.com>
*  Reports
*/
#include "reports.h"

SDCard::SDCard(PinName MOSI, PinName MISO, PinName CLK, PinName CS) : sd(MOSI, MISO, CLK, CS), fs("SD")
{
    test_str = "TEST123\n";
}

bool SDCard::mount(void)
{
    int err = fs.mount(&sd);
    if (err == 0)
        return true;
    return false;
}

bool SDCard::unmount(void)
{
    int err = fs.unmount();
    if (err == 0)
        return true;
    return false;
}

bool SDCard::create_dir(const char *dir)
{
    int err = mkdir(dir, 0777);
    if (err == 0)
        return true;
    else
        return false;
}

bool SDCard::mkdir_rec(const char *dir)
{
    char tmp[64];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    int err = mkdir(tmp, S_IRWXU);
    if (err == 0)
        return true;
    else
        return false;
}

bool SDCard::file_exist(const char *filepath)
{
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
}

bool SDCard::dir_exist(const char *path)
{
    DIR *dir = opendir(path);
    if (dir)
    {
        /* Directory exists. */
        closedir(dir);
        return true;
    }
    else
        return false;
}

bool SDCard::remove_file(const char *filepath)
{
    int ret = remove(filepath);
    if (!ret)
        return true;
    else
        return false;
}

bool SDCard::search_file(const char *path, const char *search_term, char *output)
{
    bool result = false;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(path)) != NULL)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir(dir)) != NULL)
        {
            if (strstr(ent->d_name, search_term) != NULL)
            {
                strcpy(output, ent->d_name);
                result = true;
                break;
            }
        }
        closedir(dir);
    }

    return result;
}

long SDCard::file_size(const char *path)
{
    long size = 0;
    FILE *fp = fopen(path, "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END); // seek to end of file
        size = ftell(fp);       // get current file pointer
        fclose(fp);
    }
    return size;
}

int SDCard::read_file_binary(const char *file_path, unsigned char *file_buffer, long size)
{
    FILE *fp = fopen(file_path, "rb");
    int bytesRead = 0;
    if (fp)
    {
        bytesRead = fread(file_buffer, 1, size, fp);
        fclose(fp);
    }
    return bytesRead;
}

bool SDCard::read_file_text(const char *file_path, char *file_buffer, long size)
{
    FILE *fp = fopen(file_path, "r");
    if (fp)
    {
        fgets(file_buffer, size, fp);
        fclose(fp);
        return true;
    }
    return false;
}

bool SDCard::write(const char *file, const char *str, const char *mode)
{
    FILE *fp = fopen(file, mode);
    if (fp)
    {
        fprintf(fp, "%s\n", str);
        fclose(fp);
        return true;
    }
    return false;
}

bool SDCard::check(const char *path)
{
    bool result = false;
    if (not write(path, test_str, "w"))
    {
        return result;
    }
    //read file back
    char *buff = new char[32];
    if (read_file_text(path, buff, 32))
    {
        if (strcmp(test_str, buff) == 0)
        {
            result = true;
        }
    }
    remove_file(path);
    delete[] buff;
    return result;
}

DeviceReports::DeviceReports(SDCard *sd, int deviceid) : _sd(sd)
{
    sprintf(device_main_directory, SD_ROOT, deviceid);
    strcpy(sd_check_filepath, device_main_directory);
    strcat(sd_check_filepath, "/TEST");
}

bool DeviceReports::init()
{
    // mount sd card
    if (_sd->mount())
    {
        prepare_directories();
        if (_sd->check(sd_check_filepath))
            return true;
        else
            _sd->unmount();
    }
    return false;
}

void DeviceReports::prepare_directories(void)
{
    //create device main directory
    if (!_sd->dir_exist(device_main_directory))
    {
        _sd->mkdir_rec(device_main_directory);
    }
}

std::string DeviceReports::file_path_gen(void)
{
    std::string output;
    char *buffer = new char[64]();
    time_t seconds = time(NULL);
    strftime(buffer, 64, REPROT_PREFIX, localtime(&seconds));
    output.append(device_main_directory);
    output.append("/");
    output.append(buffer);
    output.append(UNCOMPRESS_PREFIX);
    delete[] buffer;
    return output;
}

bool DeviceReports::compress_file(const char *src, const char *dest, bool remove_file)
{
    bool result = false;
    int ret = ZLib::compress_file(src, dest);
    if (!ret)
    {
        if (remove_file)
            _sd->remove_file(src);
        // printf("removed file: %s\r\n", src);
        result = true;
    }
    return result;
}

bool DeviceReports::check_missed_files(char *output)
{
    char *file_name = new char[64]();
    char *path = new char[64]();
    char *path_gz = new char[64]();
    bool result = false;
    if (_sd->search_file(device_main_directory, UNCOMPRESS_PREFIX, file_name))
    {
        strcat(path, device_main_directory);
        strcat(path, "/");
        strcat(path, file_name);
        strcat(path_gz, path);
        strcat(path_gz, COMPRESS_PREFIX);
        // remove uncompress_prefix from path_gz
        Utility::removeSubstr(path_gz, UNCOMPRESS_PREFIX);
        // printf("path: %s, file_name: %s, pathgz: %s\r\n", path, file_name, path_gz);
        // choose files that doesnot have '.gz' and relative compressed doesnot exists
        if ((strstr(path, COMPRESS_PREFIX) == NULL) && (!_sd->file_exist(path_gz)))
        {
            if (compress_file(path, path_gz))
            {
                strcpy(output, path_gz);
                result = true;
            }
        }
    }
    delete[] file_name;
    delete[] path;
    delete[] path_gz;
    return result;
}

bool DeviceReports::write_reports(const char *file_path, DeviceData *devicedata)
{
    bool result = false;
    unsigned char *b64_buffer = new unsigned char[MAX_BUFFSIZE]();
    msg_encoded encoded_data = msg_encoded_default;
    if (_deviceproto.devicedata_encode(devicedata, &encoded_data))
    {
        if (Base64::encode(b64_buffer, MAX_BUFFSIZE, encoded_data.msg, encoded_data.length))
        {
            _sd->write(file_path, reinterpret_cast<char *>(b64_buffer));
            result = true;
        }
    }
    delete[] b64_buffer;
    return result;
}