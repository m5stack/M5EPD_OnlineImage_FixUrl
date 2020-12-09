#pragma once

#include <M5EPD.h>
#include "esp_err.h"

#define EXTINFO_DEFAULT_FLASH_ADDR 0x3FF000

esp_err_t ExtInfoInitAddr(uint32_t addr);

esp_err_t ExtInfoGetString(char *key, char **string);

esp_err_t ExtInfoGetInt(char *key, int* value);

esp_err_t ExtInfoGetDouble(char *key, double* value);

