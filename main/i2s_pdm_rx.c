/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "driver/i2s_pdm.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include "i2s_pdm_example.h"
#include "i2s_example_pins.h"

// PDM受信用のGPIO設定
#define EXAMPLE_PDM_RX_CLK_IO   42  // クロック入力
#define EXAMPLE_PDM_RX_DIN_IO   41  // データ入力
#define EXAMPLE_PDM_RX_FREQ_HZ  16000  // サンプリング周波数（Hz）

// PDM受信チャンネルの初期化
static i2s_chan_handle_t i2s_example_init_pdm_rx(void)
{
    i2s_chan_handle_t rx_chan;

    // I2Sチャンネルをマスターとして作成（RXのみ）
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&rx_chan_cfg, NULL, &rx_chan));

    // PDM RXモードの設定
    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg = I2S_PDM_RX_CLK_DEFAULT_CONFIG(EXAMPLE_PDM_RX_FREQ_HZ),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = EXAMPLE_PDM_RX_CLK_IO,
            .din = EXAMPLE_PDM_RX_DIN_IO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    // PDM RXモードで初期化
    ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &pdm_rx_cfg));

    // チャンネル有効化
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
    return rx_chan;
}

// PDM受信タスク（データ読み取り＆表示）
void i2s_example_pdm_rx_task(void *args)
{
    int16_t *r_buf = (int16_t *)calloc(1, EXAMPLE_BUFF_SIZE);
    assert(r_buf != NULL);

    i2s_chan_handle_t rx_chan = i2s_example_init_pdm_rx();
    size_t r_bytes = 0;

    while (1) {
        
        if (i2s_channel_read(rx_chan, r_buf, EXAMPLE_BUFF_SIZE, &r_bytes, 1000) == ESP_OK) {
            int sample_count = r_bytes / sizeof(int16_t);
            int32_t sum = 0;
            for (int i = 0; i < sample_count; i++) {
                sum += r_buf[i];
            }
            int16_t dc = sum / sample_count;
            // データ出力
            for (int i = 0; i < sample_count; i++) {
                printf("%d\n", r_buf[i] - dc);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));  // 間隔調整
    }

    free(r_buf);
    vTaskDelete(NULL);
}
