/**
 * @file bsp_i2s.c
 * @brief 
 * @version 0.1
 * @date 2021-07-23
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "bsp_board.h"
#include "bsp_i2s.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

/* Required for I2S driver workaround */
#include "esp_rom_gpio.h"
#include "hal/gpio_hal.h"
#include "hal/i2s_ll.h"

#define I2S_CONFIG_DEFAULT() { \
    .mode                   = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX, \
    .sample_rate            = sample_rate, \
    .bits_per_sample        = I2S_BITS_PER_SAMPLE_16BIT, \
    .channel_format         = I2S_CHANNEL_FMT_MULTIPLE, \
    .communication_format   = I2S_COMM_FORMAT_STAND_PCM_SHORT, \
    .intr_alloc_flags       = ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_IRAM, \
    .dma_buf_count          = 4, \
    .dma_buf_len            = 256, \
    .use_apll               = false, \
    .tx_desc_auto_clear     = true, \
    .fixed_mclk             = 0, \
    .mclk_multiple          = I2S_MCLK_MULTIPLE_DEFAULT, \
    .bits_per_chan          = I2S_BITS_PER_CHAN_16BIT, \
    .chan_mask              = I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1 | I2S_TDM_ACTIVE_CH2, \
    .total_chan             = 3, \
    .left_align             = false, \
    .big_edin               = false, \
    .bit_order_msb          = false, \
    .skip_msk               = false, \
}

esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;

    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

    i2s_pin_config_t pin_config = {
        .bck_io_num = GPIO_I2S_SCLK,
        .ws_io_num = GPIO_I2S_LRCK,
        .data_out_num = GPIO_I2S_DOUT,
        .data_in_num = GPIO_I2S_SDIN,
        .mck_io_num = GPIO_I2S_MCLK,
    };

    ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(i2s_num, &pin_config);

    ret_val |= i2s_stop(I2S_NUM_0);

    /* Config I2S channel format of TX and RX */
    i2s_ll_tx_set_active_chan_mask(&I2S0, I2S_TDM_ACTIVE_CH0);
    i2s_ll_rx_set_active_chan_mask(&I2S0, I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1 | I2S_TDM_ACTIVE_CH2);

    /* Inverse DSP mode WS signal polarity. See IDF-4140 for more */
    gpio_hal_iomux_func_sel(GPIO_PIN_MUX_REG[GPIO_I2S_LRCK], PIN_FUNC_GPIO);
    gpio_set_direction(GPIO_I2S_LRCK, GPIO_MODE_OUTPUT);
    esp_rom_gpio_connect_out_signal(GPIO_I2S_LRCK, i2s_periph_signal[I2S_NUM_0].m_tx_ws_sig, true, false);

    /* Clear I2S DMA buffer and start I2S */
    ret_val |= i2s_zero_dma_buffer(I2S_NUM_0);
    ret_val |= i2s_start(I2S_NUM_0);

    return ret_val;
}

esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= i2s_stop(I2S_NUM_0);
    ret_val |= i2s_driver_uninstall(i2s_num);

    return ret_val;
}
