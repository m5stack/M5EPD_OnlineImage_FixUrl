#include <M5EPD.h>
#include <WiFi.h>
#include "global_setting.h"
#include "resources/binaryttf.h"
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPClient.h>

// Usage:
// Upload your image to https://sm.ms/ (global) or https://imgchr.com/ (china)
// and copy Image URL like
// https://s3.ax1x.com/2020/12/01/DWORL6.jpg
// https://i.loli.net/2020/12/01/rTFVGimzWAjChY3.jpg

M5EPD_Canvas* _canvas;
M5EPD_Canvas* _canvas_download;

esp_err_t connectWiFi()
{
    if ((GetWifiSSID().length() < 2) || (GetWifiPassword().length() < 8))
    {
        log_d("no valid wifi config");
        return ESP_FAIL;
    }
    WiFi.mode(WIFI_STA);
    WiFi.begin(GetWifiSSID().c_str(), GetWifiPassword().c_str());
    log_d("Connect to %s", GetWifiSSID().c_str());
    uint32_t t = millis();
    while (1)
    {
        if (millis() - t > 8000)
        {
            WiFi.disconnect();
            return ESP_FAIL;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            return ESP_OK;
        }
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}


uint8_t* download(String url, uint32_t *psize)
{
    log_d("download from %s", url.c_str());
    HTTPClient http;
    char buf[50];

    _canvas_download->fillCanvas(0);
    _canvas_download->drawString("Connecting...", 270, 40);
    _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);

    _canvas_download->fillCanvas(0);

    if (WiFi.status() != WL_CONNECTED)
    {
        log_e("Not connected");
        _canvas_download->drawString("Wifi not connected.", 270, 40);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        return NULL;
    }

    http.begin(url);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        log_e("HTTP ERROR: %d\n", httpCode);
        http.end();
        sprintf(buf, "HTTP ERROR: %d\n", httpCode);
        _canvas_download->drawString(buf, 270, 40);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        http.end();
        return NULL;
    }

    size_t size = http.getSize();

    const uint16_t kBarLength = 400;
    const uint16_t kBarX = (540 - kBarLength) / 2;

    log_d("jpg size = %d Bytes", size);
    sprintf(buf, "0 / %d Bytes", size);
    _canvas_download->drawRect(kBarX, 10, kBarLength, 20, 15);
    _canvas_download->drawString(buf, 270, 60);
    _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);

    *psize = size;
    WiFiClient *stream = http.getStreamPtr();
    uint8_t *p = (uint8_t *)ps_malloc(size);
    if (p == NULL)
    {
        log_e("Memory overflow.");
        http.end();
        return NULL;
    }

    log_d("downloading...");
    size_t offset = 0;
    while (http.connected())
    {
        size_t len = stream->available();

        if(Serial.available())
        {
            http.end();
            free(p);
            return NULL;
        }

        if (!len)
        {
            delay(1);
            continue;
        }

        stream->readBytes(p + offset, len);
        offset += len;
        log_d("%d / %d", offset, size);
        sprintf(buf, "%d / %d Bytes", offset, size);
        float percent = (float)offset / (float)size;
        uint16_t px = kBarLength * percent;
        _canvas_download->fillCanvas(0);
        _canvas_download->drawRect(kBarX, 10, kBarLength, 20, 15);
        _canvas_download->fillRect(kBarX, 10, px, 20, 15);
        _canvas_download->drawString(buf, 270, 60);
        _canvas_download->pushCanvas(0, 440, UPDATE_MODE_GL16);
        if (offset == size)
        {
            break;
        }
    }

    http.end();
    return p;
}

void save(uint8_t* jpg, uint32_t size)
{
    if(jpg == NULL)
    {
        log_e("null jpg pointer");
        return;
    }
    File file = SPIFFS.open("/cache.jpg", FILE_WRITE);
    if (!file)
    {
        log_e("Failed to open file for writing");
        return;
    }
    int i;
    for(i = 0; i < size; i += 512)
    {
        log_d("saving %d / %d", i, size);
        uint32_t written = file.write(jpg + i, 512);
        if(written != 512)
        {
            log_e("SPIFFS write error at %d", i);
            file.close();
            SPIFFS.remove("/cache.jpg");
            break;
        }
        if(Serial.available())
        {
            file.close();
            return;
        }
    }
    i -= 512;
    uint32_t written = file.write(jpg + i, size - i);
    if(written != size - i)
    {
        log_e("SPIFFS write error");
        file.close();
        SPIFFS.remove("/cache.jpg");
        return;
    }
    
    file.close();
    log_d("image saved.");
}

int run()
{
    bool ret = false;
    _canvas->setTextSize(26);
    _canvas->setTextDatum(CC_DATUM);
    _canvas->drawString("Please upload the url to the picture", 270, 480);
    if (SPIFFS.exists("/cache.jpg"))
    {
        _canvas->fillCanvas(0);
        ret = _canvas->drawJpgFile(SPIFFS, "/cache.jpg");
            _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
        if(ret == 0)
        {
            SPIFFS.remove("/cache.jpg");
        }
    }
    if ((ret == 0) && getImageUrl().length())
    {
        uint32_t size;
        uint8_t *jpg = download(getImageUrl(), &size);
        if (jpg != NULL)
        {
            _canvas->fillCanvas(0);
            ret = _canvas->drawJpg(jpg, size);
            if(ret == 0)
            {
                _canvas->drawString("Error decoding image, please try again", 270, 480);
            }
            _canvas->pushCanvas(0, 0, UPDATE_MODE_GC16);
            save(jpg, size);
            free(jpg);
        }
    }
    return 1;
}


void setup()
{
    M5.begin();
    M5.RTC.begin();
    M5.EPD.Clear(true);
    M5.EPD.SetRotation(90);
    M5.TP.SetRotation(90);
    esp_err_t load_err = LoadFromExt();

    if (!SPIFFS.begin())
    {
        log_e("SPIFFS Mount Failed");
        while (1)
            ;
    }
    else
    {
        log_d("size of SPIFFS = %d bytes", SPIFFS.totalBytes());
        // listDir(SPIFFS, "/", 0);
    }

    M5EPD_Canvas canvas(&M5.EPD);
    canvas.loadFont(binaryttf, sizeof(binaryttf));
    canvas.createRender(26, 128);
    canvas.createRender(36, 128);

    LoadingAnime_32x32_Start(254, 424);
    esp_err_t wifi_err = ESP_FAIL;
    if (load_err == ESP_OK)
    {
        wifi_err = connectWiFi();
    }
    else
    {
        LoadingAnime_32x32_Stop();
        UserMessage(MESSAGE_ERR, 0, "Load Error", "Cannot load data from flash, Please re-burn the firmware", "");
        while (1)
            ;
    }
    _canvas = new M5EPD_Canvas(&M5.EPD);
    _canvas_download = new M5EPD_Canvas(&M5.EPD);
    _canvas->createCanvas(540, 960);
    _canvas_download->createCanvas(540, 80);

    _canvas_download->setTextSize(26);
    _canvas_download->setTextDatum(CC_DATUM);
    _canvas_download->setTextColor(15);
    LoadingAnime_32x32_Stop();
    run();

    M5.RTC.setAlarmIRQ(getWakeTime());
    M5.disableMainPower();
}

void loop()
{
}