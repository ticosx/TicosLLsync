/*
 * Copyright (C) 2019 THL A29 Limited, a Tencent company. All rights reserved.
 * Licensed under the MIT License (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT
 * Unless required by applicable law or agreed to in writing, software distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "ble_qiot_export.h"
#include "ble_qiot_service.h"
#include "ble_qiot_import.h"

// add std head file here
#include <stdint.h>

// add ble qiot head file here
#include "ble_qiot_import.h"
#include "ble_qiot_config.h"
#include "ble_qiot_log.h"

#include "esp_bt_device.h"
#include "esp_spi_flash.h"
#include "esp_ota_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

// divece info which defined in explorer platform
static TimerHandle_t ota_reboot_timer;

int ble_get_mac(char *mac)
{
    char *address = (char *)esp_bt_dev_get_address();
    memcpy(mac, address, 6);

    return 0;
}

int ble_write_flash(uint32_t flash_addr, const char *write_buf, uint16_t write_len)
{
    int ret = spi_flash_erase_range(flash_addr, BLE_QIOT_RECORD_FLASH_PAGESIZE);
    ret     = spi_flash_write(flash_addr, write_buf, write_len);

    return ret == ESP_OK ? write_len : ret;
}

int ble_read_flash(uint32_t flash_addr, char *read_buf, uint16_t read_len)
{
    int ret = spi_flash_read(flash_addr, read_buf, read_len);

    return ret == ESP_OK ? read_len : ret;
}

typedef struct ble_esp32_timer_id_ {
    uint8_t       type;
    ble_timer_cb  handle;
    TimerHandle_t timer;
} ble_esp32_timer_id;

ble_timer_t ble_timer_create(uint8_t type, ble_timer_cb timeout_handle)
{
    ble_esp32_timer_id *p_timer = malloc(sizeof(ble_esp32_timer_id));
    if (NULL == p_timer) {
        return NULL;
    }

    p_timer->type   = type;
    p_timer->handle = timeout_handle;
    p_timer->timer  = NULL;

    return (ble_timer_t)p_timer;
}

ble_qiot_ret_status_t ble_timer_start(ble_timer_t timer_id, uint32_t period)
{
    ble_esp32_timer_id *p_timer = (ble_esp32_timer_id *)timer_id;

    if (NULL == p_timer->timer) {
        p_timer->timer =
            (ble_timer_t)xTimerCreate("ota_timer", period / portTICK_PERIOD_MS,
                                      p_timer->type == BLE_TIMER_PERIOD_TYPE ? pdTRUE : pdFALSE, NULL, p_timer->handle);
    }
    xTimerReset(p_timer->timer, portMAX_DELAY);

    return BLE_QIOT_RS_OK;
}

ble_qiot_ret_status_t ble_timer_stop(ble_timer_t timer_id)
{
    ble_esp32_timer_id *p_timer = (ble_esp32_timer_id *)timer_id;
    xTimerStop(p_timer->timer, portMAX_DELAY);

    return BLE_QIOT_RS_OK;
}

ble_qiot_ret_status_t ble_timer_delete(ble_timer_t timer_id)
{
    ble_esp32_timer_id *p_timer = (ble_esp32_timer_id *)timer_id;
    xTimerDelete(p_timer->timer, portMAX_DELAY);
    free(p_timer);

    return BLE_QIOT_RS_OK;
}

// return ATT MTU
uint16_t ble_get_user_data_mtu_size(void)
{
    return BLE_QIOT_EVENT_BUF_SIZE;
}

uint8_t ble_ota_is_enable(const char *version)
{
    ble_qiot_log_e("ota version: %s, enable ota", version);
    return BLE_OTA_ENABLE;
}

uint32_t ble_ota_get_download_addr(void)
{
    esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    ble_qiot_log_i("otafile download address: %d", partition->address);
    return partition->address;
}

int ble_ota_write_flash(uint32_t flash_addr, const char *write_buf, uint16_t write_len)
{
    int ret = 0;

    if (flash_addr % BLE_QIOT_RECORD_FLASH_PAGESIZE == 0) {
        ret = spi_flash_erase_range(flash_addr, BLE_QIOT_RECORD_FLASH_PAGESIZE);
    } else {
        if ((flash_addr + write_len - 1) / BLE_QIOT_RECORD_FLASH_PAGESIZE != flash_addr / BLE_QIOT_RECORD_FLASH_PAGESIZE) {
            ret = spi_flash_erase_range(((flash_addr / BLE_QIOT_RECORD_FLASH_PAGESIZE) + 1) * BLE_QIOT_RECORD_FLASH_PAGESIZE, BLE_QIOT_RECORD_FLASH_PAGESIZE);
        }
    }
    // printf("write ota addr 0x%x, write 0x%x, erase flash %d\r\n", flash_addr, write_len, ret);
    ret = spi_flash_write(flash_addr, write_buf, write_len);

    return ret == ESP_OK ? write_len : ret;
}

static void ble_ota_reboot_timer(void *param)
{
    esp_restart();
}
    
void ble_qiot_ota_final_handle(uint8_t result)
{
    ble_qiot_log_i("ble ota_final_handle, result %d\r\n", result);
    if (result == BLE_QIOT_OTA_SUCCESS) {
        esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
        esp_ota_set_boot_partition(partition);
        ble_qiot_log_i("ota success, device restart after 5 seconds\r\n");
        ota_reboot_timer = xTimerCreate("reboot_timer", 5000 / portTICK_PERIOD_MS, pdFALSE, NULL, ble_ota_reboot_timer);
        xTimerStart(ota_reboot_timer, portMAX_DELAY);
    }
    return;
}


#ifdef __cplusplus
}
#endif
