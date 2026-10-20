#include "board/board.h"

#include "board/board_pins.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "power";

#define VSYS_ADC_CHANNEL ADC_CHANNEL_3 /* GPIO4 */
#define VSYS_SAMPLES 16
#define USB_PIN_MV 1000 /* GPIO15 LOW: ~0 mV on battery, ~2300 mV on USB */
#define SETTLE_MS 5     /* 10 nF behind 50 kOhm: tau 0.5 ms */

static adc_oneshot_unit_handle_t s_adc;
static adc_cali_handle_t s_cali;

static int pin_mv(void)
{
    int sum = 0;
    for (int i = 0; i < VSYS_SAMPLES; i++) {
        int raw = 0;
        int mv = 0;
        if (adc_oneshot_read(s_adc, VSYS_ADC_CHANNEL, &raw) != ESP_OK) {
            return 0;
        }
        if (s_cali == NULL || adc_cali_raw_to_voltage(s_cali, raw, &mv) != ESP_OK) {
            mv = raw * 3100 / 4095; /* uncalibrated fallback, 12 dB range */
        }
        sum += mv;
    }
    return sum / VSYS_SAMPLES;
}

esp_err_t board_power_init(bool *usb_present)
{
    /* Output LOW with no pulls: even the weak internal pull-up would switch V3V on. */
    gpio_set_level(BOARD_PIN_POWER_EN, 0);
    const gpio_config_t en = {
        .pin_bit_mask = 1ULL << BOARD_PIN_POWER_EN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&en), TAG, "power_en");
    gpio_set_level(BOARD_PIN_POWER_EN, 0);

    const adc_oneshot_unit_init_cfg_t unit = {.unit_id = ADC_UNIT_1};
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit, &s_adc), TAG, "adc unit");
    const adc_oneshot_chan_cfg_t chan = {.atten = ADC_ATTEN_DB_12,
                                         .bitwidth = ADC_BITWIDTH_DEFAULT};
    ESP_RETURN_ON_ERROR(adc_oneshot_config_channel(s_adc, VSYS_ADC_CHANNEL, &chan), TAG,
                        "adc chan");
    const adc_cali_curve_fitting_config_t cali = {
        .unit_id = ADC_UNIT_1,
        .chan = VSYS_ADC_CHANNEL,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali, &s_cali) != ESP_OK) {
        ESP_LOGW(TAG, "no ADC calibration, using the raw curve");
        s_cali = NULL;
    }

    /* With GPIO15 LOW only VBUS powers the divider: a clean USB test. */
    vTaskDelay(pdMS_TO_TICKS(SETTLE_MS));
    *usb_present = pin_mv() > USB_PIN_MV;

    /* V3V on: backlight supply and the divider on battery. */
    gpio_set_level(BOARD_PIN_POWER_EN, 1);
    vTaskDelay(pdMS_TO_TICKS(SETTLE_MS));
    return ESP_OK;
}

uint16_t board_power_vsys_mv(void)
{
    return (uint16_t)(pin_mv() * 2);
}
