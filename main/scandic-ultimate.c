//STOLEN FROM https://github.com/Grrtzm/ESP32_ST7735_DEMO/blob/main/main/main.c

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include <drivers/display/lcd/lv_lcd_generic_mipi.h> // Don't forget to enable the Generic MIPI display driver in menuconfig!

#define TAG "LVGL"

// SPI and display pins for LUATOS AIR101-LCD display based on ST7735 driver on a Luatos ESP32C3-CORE Development Board
#define PIN_NUM_MOSI 9
#define PIN_NUM_CLK 10
#define PIN_NUM_CS 5
#define PIN_NUM_DC 7
#define PIN_NUM_RST 6
#define PIN_NUM_BCKL 21

#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ 40000000 // 40 MHz
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
#define LCD_WIDTH 160   // You will need to tweak this if you choose another display
#define LCD_HEIGHT 128   // You will need to tweak this if you choose another display
#define LCD_X_OFFSET 1  // You will need to tweak this if you choose another display
#define LCD_Y_OFFSET 26 // You will need to tweak this if you choose another display
#define BUFFER_HEIGHT 135

static lv_display_t *disp = NULL;
static esp_lcd_panel_handle_t panel_handle = NULL;
static lv_color_t *buf1 = NULL;

// Periodical tick for lv_tick_inc()
static void lv_tick_cb(void *arg)
{
    lv_tick_inc(1);
}

// Periodical LVGL updates using a timer (rendering, input, animation)
static void lvgl_timer_cb(void *arg)
{
    lv_timer_handler();
}

// LVGL flush callback
static void flush_cb(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *color_map)
{
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, color_map);
    lv_display_flush_ready(disp_drv);
}

void app_main(void)
{
    gpio_config_t bklt_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
    };
    ESP_ERROR_CHECK(gpio_config(&bklt_config));
    gpio_set_level(PIN_NUM_BCKL, 1); // Turn on backlight

    // SPI bus config
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // SPI interface to LCD
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = LCD_PIXEL_CLOCK_HZ,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = NULL,
        .user_ctx = NULL,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    // LCD config
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .color_space = ESP_LCD_COLOR_SPACE_RGB, // actually it's RBG, i couldn't get it working correctly, regardless of this and esp_lcd_panel_invert_color() setting.
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true)); // true = black background, white foreground, false = other way around
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_set_gap(panel_handle, LCD_X_OFFSET, LCD_Y_OFFSET));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // LVGL init
    lv_init();

    // Create Display
    disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp,
                           heap_caps_malloc(LCD_WIDTH * BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA),
                           NULL,
                           LCD_WIDTH * BUFFER_HEIGHT * sizeof(lv_color_t),
                           LV_DISPLAY_RENDER_MODE_FULL);

    buf1 = heap_caps_malloc(LCD_WIDTH * BUFFER_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    assert(buf1 != NULL);

    // LVGL tick timer (1ms)
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lv_tick_cb,
        .name = "lv_tick"};
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000));

    // LVGL rendering timer (5ms)
    const esp_timer_create_args_t render_timer_args = {
        .callback = &lvgl_timer_cb,
        .name = "lvgl_timer"};
    esp_timer_handle_t render_timer;
    ESP_ERROR_CHECK(esp_timer_create(&render_timer_args, &render_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(render_timer, 5000));

    // Example object
    lv_obj_t *label = lv_label_create(lv_screen_active());
    // For colors, remember that this code uses RBG format instead of RGB (which is very unusual, probably a bug somewhere)!

    // lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), 0); // Black background
    // lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x0000FF), 0); // Green background
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x00FF00), 0); // Blue background
    // lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0xFF0000), 0); // Red background
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, 0); // Opa=cover so background color is visible

    // lv_obj_set_style_bg_color(label, lv_color_hex(0x00FF00), 0);   // Label gets a blue background
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); // font 14 is standard in LVGL 9.3
    // lv_obj_set_style_text_color(label, lv_color_hex(0xFF00FF), 0); // Yellow text
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // White text
    // lv_obj_set_style_bg_opa(label, LV_OPA_COVER, 0);               // Make sure label background is opaque
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, 0);             // Opa=cover so text is visible

    lv_label_set_text(label, "Hello LVGL 9.3!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    ESP_LOGI(TAG, "LVGL ready.");
}
