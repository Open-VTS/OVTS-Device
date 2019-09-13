/*
*  OVTS Device Project
*  https://github.com/Open-VTS
*  Main Code
*  Author: M.Rahimi <work.rahimi@gmail.com>
*/

#include "mbed.h"
#include "core.h"

Serial pc(PA_9, PA_10); // tx, rx

DeviceCore DeviceCore;

void timetime()
{
    time_t seconds = time(NULL);
    printf("Local Time: %s", ctime(&seconds));
}

void mbed_error_reboot_callback(mbed_error_ctx *error_context)
{
    printf("\nmbed_error_reboot_callback: System rebooting, reboot error callback received");
    mbed_get_reboot_error_info(&DeviceCore::error_ctx);
    mbed_reset_reboot_error_info();
}

void generate_bus_fault_unaligned_access()
{
    SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;
    uint32_t inv_addr = 0xAAA3;
    uint32_t val = *(uint32_t *)inv_addr;
    printf("\nval= %X", val);
    return;
}

int main()
{
    pc.baud(115200);
    printf("\r\n");
    wait(2);
    DeviceCore::system_check();
    printf("ID: %d\nVersion: %s\nUpload Time: %s %s\n", DeviceID, SOFTWARE_VER, DATE, TIME);
    timetime();
    if (DeviceCore.init() <= 0)
    {
        wait_ms(100);
        printf("Core init Failed Rebooting\r\n");
        DeviceCore.reboot();
    }
    printf("Core init success\r\n");

    DeviceCore.run();
}
