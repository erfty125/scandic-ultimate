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
#include <drivers/display/lcd/lv_lcd_generic_mipi.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "system.h"
#include "apps.h"

#define TAG "Scandic"

#define PIN_NUM_MOSI 7
#define PIN_NUM_CLK 6
#define PIN_NUM_CS 21
#define PIN_NUM_DC 9
#define PIN_NUM_RST 10
#define PIN_NUM_BCKL 5
#define ADCCHAN1 ADC_CHANNEL_1
#define ADCCHAN2 ADC_CHANNEL_2
#define potscale2 28.0f
#define potscale1 15.0f

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void adc_calibration_deinit(adc_cali_handle_t handle);

adc_oneshot_unit_handle_t adc1_handle;
adc_cali_handle_t adc1_cali_chan0_handle;
adc_cali_handle_t adc1_cali_chan1_handle;
bool do_calibration1_chan0;
bool do_calibration1_chan1;

int fp;
int sp;
//cálibration & muiltisampling shit
int pvals[2];
int adc_raw[2];


#define LCD_HOST SPI2_HOST
#define LCD_PIXEL_CLOCK_HZ 40000000 // 40 MHz
#define LCD_CMD_BITS 8
#define LCD_PARAM_BITS 8
#define LCD_WIDTH 160   // You will need to tweak this if you choose another display
#define LCD_HEIGHT 128   // You will need to tweak this if you choose another display
#define LCD_X_OFFSET 0  // You will need to tweak this if you choose another display
#define LCD_Y_OFFSET 0 // You will need to tweak this if you choose another display
#define BUFFER_HEIGHT 135

esp_lcd_panel_handle_t panel_handle=NULL;
lv_display_t *disp=NULL;



static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

static void flush_cb(lv_display_t *disp_drv, const lv_area_t *area, uint8_t *color_map)
{
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, color_map);
    lv_display_flush_ready(disp_drv);
}


void displvglinit(){
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
        .color_space = ESP_LCD_COLOR_SPACE_RGB, // actually it's RBG, i couldn't get it working correctly, regardless of this and esp_lcd_panel_invert_>
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false)); // true = black background, white foreground, false = other way around
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
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
                   }


void initpot(){
adc1_cali_chan0_handle = NULL;
adc1_cali_chan1_handle = NULL;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADCCHAN1, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADCCHAN2, &config));

    do_calibration1_chan0 = adc_calibration_init(ADC_UNIT_1, ADCCHAN1, ADC_ATTEN_DB_12, &adc1_cali_chan0_handle);
    do_calibration1_chan1 = adc_calibration_init(ADC_UNIT_1, ADCCHAN2, ADC_ATTEN_DB_12, &adc1_cali_chan1_handle);
               }

void updatepotval(){
fp=0;
sp=0;
ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADCCHAN1, &adc_raw[0]));
ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADCCHAN1, adc_raw[0]);
 if (do_calibration1_chan0){
 ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan0_handle, adc_raw[0], &pvals[0]));
 ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADCCHAN1, pvals[0]);
                           }
 else{pvals[0]=adc_raw[0];}
ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADCCHAN2, &adc_raw[1]));
ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, ADCCHAN2, adc_raw[1]);
 if (do_calibration1_chan1){
 ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan1_handle, adc_raw[1], &pvals[1]));
 ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, ADCCHAN2, pvals[1]);
                           }
else{pvals[1]=adc_raw[1];}
fp=(int)(float)(pvals[0]/potscale1);
sp=(int)(float)(pvals[1]/potscale2);

                   }

void updatelvgl(){
lv_timer_handler();
                 }

void systeminit(){
displvglinit();
initpot();
initbars();
initapps();
                 }

void appsupdate(){
static int psp=100;
static int pfp=100;
static int curapp=0;

if(psp<=98 && sp==100 && pfp==100 && fp==100){
dapp[curapp]();
curapp++;
if(curapp>=appscount)curapp=0;
iapp[curapp]();
                                               }

if(pfp<=98 && fp==100 && psp==100 && sp==100){
dapp[curapp]();
curapp--;
if(curapp<0)curapp=appscount-1;
iapp[curapp]();
                                               }

uapp[curapp]();
psp=sp;
pfp=fp;
                 }

void systemupdate(){
updatepotval();
updatelvgl();
barsupdate();
appsupdate();
                   }
