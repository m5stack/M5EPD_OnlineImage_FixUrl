#ifndef _GLOBAL_SETTING_H_
#define _GLOBAL_SETTING_H_

#include <M5EPD.h>
#include <nvs.h>

enum
{
    MESSAGE_INFO = 0,
    MESSAGE_WARN,
    MESSAGE_ERR,
    MESSAGE_OK
};

esp_err_t LoadFromExt(void);
String getMACString(void);
String getImageUrl(void);
String GetWifiSSID(void);
String GetWifiPassword(void);
int getWakeTime(void);
void LoadingAnime_32x32_Start(uint16_t x, uint16_t y);
void LoadingAnime_32x32_Stop();
void UserMessage(int level, uint32_t timeout, String title, String msg, String end);

#endif //_GLOBAL_SETTING_H_