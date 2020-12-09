#include "global_setting.h"
#include "./resources/ImageResource.h"
#include "esp32-hal-log.h"
#include <WiFi.h>
#include "ext_info.h"

#define DEFAULT_WALLPAPER 2
SemaphoreHandle_t _xSemaphore_LoadingAnime = NULL;
static uint8_t _loading_anime_eixt_flag = false;
esp_err_t __espret__;
#define NVS_CHECK(x)              \
    __espret__ = x;           \
    if (__espret__ != ESP_OK) \
    {                         \
        nvs_close(nvs_arg);   \
        log_e("Check Err");   \
        return __espret__;    \
    }

const uint8_t *kIMGLoading[16] = {
    ImageResource_item_loading_01_32x32,
    ImageResource_item_loading_02_32x32,
    ImageResource_item_loading_03_32x32,
    ImageResource_item_loading_04_32x32,
    ImageResource_item_loading_05_32x32,
    ImageResource_item_loading_06_32x32,
    ImageResource_item_loading_07_32x32,
    ImageResource_item_loading_08_32x32,
    ImageResource_item_loading_09_32x32,
    ImageResource_item_loading_10_32x32,
    ImageResource_item_loading_11_32x32,
    ImageResource_item_loading_12_32x32,
    ImageResource_item_loading_13_32x32,
    ImageResource_item_loading_14_32x32,
    ImageResource_item_loading_15_32x32,
    ImageResource_item_loading_16_32x32
};

String global_wifi_ssid;
String global_wifi_password;
String global_mac;
String global_image_url;
int global_wake_time = 60;
M5EPD_Canvas loadinginfo(&M5.EPD);
M5EPD_Canvas loading(&M5.EPD);

String GetWifiSSID(void)
{
    return global_wifi_ssid;
}

String GetWifiPassword(void)
{
    return global_wifi_password;
}

String getMACString(void)
{
    if(global_mac.length() == 0)
    {
        uint64_t mac = ESP.getEfuseMac();
        uint32_t msb = mac >> 32;
        uint32_t lsb = mac & 0xFFFFFFFF;
        char mac_address[20];
        sprintf(mac_address, "%08X%08X", msb, lsb);
        global_mac = String(mac_address);
        log_d("MAC = %s", mac_address);
    }
    return global_mac;
}

int getWakeTime()
{
    return global_wake_time;
}

String getImageUrl()
{
    return global_image_url;
}

esp_err_t LoadFromExt(void)
{
    ExtInfoInitAddr(EXTINFO_DEFAULT_FLASH_ADDR);

    char* ssid = NULL;
    char* pwd = NULL;
    char* url = NULL;
    int wake_time = 60;

    esp_err_t ret;
    ret = ExtInfoGetString("ssid", &ssid);
    ret |= ExtInfoGetString("pwd", &pwd);
    ret |= ExtInfoGetString("url", &url);
    ret |= ExtInfoGetInt("wake_time", &wake_time);

    if(ret == ESP_OK)
    {
        global_wifi_ssid = String(ssid);
        global_wifi_password = String(pwd);
        global_image_url = String(url);
        global_wake_time = wake_time;
    }

    return ret;
}

const uint8_t* GetLoadingIMG_32x32(uint8_t id)
{
    return kIMGLoading[id];
}

void __LoadingAnime_32x32(void *pargs)
{
    uint16_t *args = (uint16_t *)pargs;
    uint16_t x = args[0];
    uint16_t y = args[1];
    free(pargs);

    loading.fillCanvas(0);
    loading.pushCanvas(x, y, UPDATE_MODE_GL16);
    int anime_cnt = 0;
    uint32_t time = 0;
    uint32_t timeout = millis();
    bool updated = false;
    loadinginfo.pushCanvas(70, y + 45, UPDATE_MODE_GL16);
    while (1)
    {
        if(millis() - time > 300)
        {
            time = millis();
            loading.pushImage(0, 0, 32, 32, GetLoadingIMG_32x32(anime_cnt));
            loading.pushCanvas(x, y, UPDATE_MODE_DU4);
            anime_cnt++;
            if(anime_cnt == 16)
            {
                anime_cnt = 0;
            }
        }

        if((updated == false) && (millis() - timeout > 60 * 1000))
        {
            updated = true;
            log_e("Loading timeout, system restart.");
            delay(1000);
            ESP.restart();
        }

        // xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
        if(_loading_anime_eixt_flag == true)
        {
            // xSemaphoreGive(_xSemaphore_LoadingAnime);
            break;
        }
        // xSemaphoreGive(_xSemaphore_LoadingAnime);
    }
    vTaskDelete(NULL);
}

void LoadingAnime_32x32_Start(uint16_t x, uint16_t y)
{
    return;
    if(_xSemaphore_LoadingAnime == NULL)
    {
        _xSemaphore_LoadingAnime = xSemaphoreCreateMutex();
        loadinginfo.createCanvas(400, 32);
        loadinginfo.fillCanvas(0);
        loadinginfo.setTextSize(26);
        loadinginfo.setTextDatum(CC_DATUM);
        loadinginfo.setTextColor(15);
        loadinginfo.drawString("Loading...", 200, 16);
        loading.createCanvas(32, 32);
        loading.fillCanvas(0);
    }
    _loading_anime_eixt_flag = false;
    uint16_t *pos = (uint16_t*)calloc(2, sizeof(uint16_t));
    pos[0] = x;
    pos[1] = y;
    xTaskCreatePinnedToCore(__LoadingAnime_32x32, "__LoadingAnime_32x32", 16 * 1024, pos, 1, NULL, 0);
}

void LoadingAnime_32x32_Stop()
{
    return;
    // xSemaphoreTake(_xSemaphore_LoadingAnime, portMAX_DELAY);
    _loading_anime_eixt_flag = true;
    // xSemaphoreGive(_xSemaphore_LoadingAnime);
    delay(200);
}

void UserMessage(int level, uint32_t timeout, String title, String msg, String end)
{
    M5.EPD.Clear();
    M5EPD_Canvas canvas(&M5.EPD);
    canvas.createCanvas(460, 460);
    canvas.fillCanvas(0);

    switch(level)
    {
        case MESSAGE_INFO:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_info_96x96);
            break;
        }
        case MESSAGE_WARN:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_warn_96x96);
            break;
        }
        case MESSAGE_ERR:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_error_96x96);
            break;
        }
        case MESSAGE_OK:
        {
            canvas.pushImage(182, 0, 96, 96, ImageResource_message_level_success_96x96);
            break;
        }
    }

    const uint16_t kTitleY = 96 + 35;
    const uint16_t kMsgY = kTitleY + 50;
    const uint16_t kEndY = 430;

    canvas.setTextDatum(TC_DATUM);
    canvas.setTextSize(36);
    canvas.drawString(title, 230, kTitleY);

    if(msg.length())
    {
        canvas.setTextSize(26);
        canvas.setTextColor(8);
        if(msg.length() < 40)
        {
            canvas.setTextDatum(TC_DATUM);
            canvas.drawString(msg, 230, kMsgY);
        }
        else
        {
            canvas.setTextArea(0, kMsgY, 460, 200);
            canvas.print(msg);
        }
    }
    
    if(end.length())
    {
        canvas.setTextDatum(TC_DATUM);
        canvas.setTextSize(26);
        canvas.setTextColor(8);
        canvas.drawString(end, 230, kEndY);
    }
    
    canvas.pushCanvas(40, 250, UPDATE_MODE_NONE);
    M5.EPD.UpdateFull(UPDATE_MODE_GC16);

    while(M5.TP.avaliable());
    M5.TP.flush();
    uint32_t t = millis();
    while(1)
    {
        if(M5.TP.avaliable())
        {
            M5.TP.update();
            if(M5.TP.getFingerNum() != 0)
            {
                break;
            }
        }
        if(millis() - t > timeout)
        {
            break;
        }
    }
}
