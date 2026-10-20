#include "board/board.h"

#include "board/board_pins.h"
#include "board_vendor_init.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_lcd_io_i80.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "display";

/* ST7789V 8080 write cycle >= 66 ns (~15 MHz); 10 MHz is in spec and an exact
 * divider of the 160 MHz source (ADR-0006). Step down, never up. */
#define LCD_PIXEL_CLOCK_HZ (10 * 1000 * 1000)
#define LCD_DRAW_LINES 40
#define LCD_DRAW_PIXELS (BOARD_LCD_H_RES * LCD_DRAW_LINES)

static esp_lcd_panel_io_handle_t s_io;
static esp_lcd_panel_handle_t s_panel;

static esp_err_t send_vendor_init(void)
{
    for (size_t i = 0; i < sizeof(k_vendor_init) / sizeof(k_vendor_init[0]); i++) {
        const board_lcd_init_cmd_t *c = &k_vendor_init[i];
        ESP_RETURN_ON_ERROR(
            esp_lcd_panel_io_tx_param(s_io, c->cmd, c->len ? c->data : NULL, c->len), TAG,
            "vendor init 0x%02x", c->cmd);
        if (c->delay_ms) {
            vTaskDelay(pdMS_TO_TICKS(c->delay_ms));
        }
    }
    return ESP_OK;
}

esp_err_t board_display_init(bool rotate_180, lv_display_t **out_disp)
{
    /* esp_lcd never drives RD; a floating RD corrupts transfers (#103). */
    const gpio_config_t rd = {.pin_bit_mask = 1ULL << BOARD_PIN_LCD_RD, .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&rd), TAG, "rd");
    gpio_set_level(BOARD_PIN_LCD_RD, 1);

    esp_lcd_i80_bus_handle_t bus = NULL;
    const esp_lcd_i80_bus_config_t bus_cfg = {
        .dc_gpio_num = BOARD_PIN_LCD_DC,
        .wr_gpio_num = BOARD_PIN_LCD_WR,
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_gpio_nums = {BOARD_PIN_LCD_D0, BOARD_PIN_LCD_D1, BOARD_PIN_LCD_D2, BOARD_PIN_LCD_D3,
                           BOARD_PIN_LCD_D4, BOARD_PIN_LCD_D5, BOARD_PIN_LCD_D6, BOARD_PIN_LCD_D7},
        .bus_width = 8,
        .max_transfer_bytes = LCD_DRAW_PIXELS * sizeof(uint16_t),
        .dma_burst_size = 64,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_i80_bus(&bus_cfg, &bus), TAG, "i80 bus");

    const esp_lcd_panel_io_i80_config_t io_cfg = {
        .cs_gpio_num = BOARD_PIN_LCD_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .dc_levels = {.dc_idle_level = 0,
                      .dc_cmd_level = 0,
                      .dc_dummy_level = 0,
                      .dc_data_level = 1},
        /* LVGL renders little-endian RGB565 (same as the simulator); LCD_CAM swaps
         * the colour bytes for the panel at no CPU cost. Command bytes are untouched. */
        .flags = {.swap_color_bytes = 1},
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_i80(bus, &io_cfg, &s_io), TAG, "panel io");

    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BOARD_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7789(s_io, &panel_cfg, &s_panel), TAG, "st7789");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "reset");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "init");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(s_panel, true), TAG, "invert");
    /* The 170-column panel sits in the middle of the controller's 240 (35+170+35). */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_set_gap(s_panel, 0, 35), TAG, "gap");
    ESP_RETURN_ON_ERROR(send_vendor_init(), TAG, "vendor init");

    lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    /* The LVGL task also runs the reducer and the deep-sleep entry; zen-clock
     * overflowed a 6 KB LVGL stack, so keep headroom over the 7 KB default. */
    port_cfg.task_stack = 10 * 1024;
    ESP_RETURN_ON_ERROR(lvgl_port_init(&port_cfg), TAG, "lvgl port");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = s_io,
        .panel_handle = s_panel,
        .buffer_size = LCD_DRAW_PIXELS,
        .double_buffer = true,
        .hres = BOARD_LCD_H_RES,
        .vres = BOARD_LCD_V_RES,
        /* Landscape with USB on the left; rotation 180 puts USB on the right. */
        .rotation = {.swap_xy = true, .mirror_x = false, .mirror_y = true},
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {.buff_dma = 1, .swap_bytes = 0},
    };
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(disp != NULL, ESP_FAIL, TAG, "lvgl display");

    if (rotate_180) {
        lvgl_port_lock(0);
        lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
        lvgl_port_unlock();
    }
    *out_disp = disp;
    return ESP_OK;
}

void board_display_lock(void)
{
    lvgl_port_lock(0);
}

void board_display_unlock(void)
{
    lvgl_port_unlock();
}

esp_err_t board_display_on(void)
{
    /* tx_param waits for queued colour DMA, so DISPON shows a complete frame. */
    return esp_lcd_panel_disp_on_off(s_panel, true);
}

esp_err_t board_display_sleep(void)
{
    if (s_panel == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return esp_lcd_panel_disp_sleep(s_panel, true);
}
