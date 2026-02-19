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
#include <drivers/display/lcd/lv_lcd_generic_mipi.h>
#include "system.h"
#include "apps.h"

#define TAG "Scandic"

lv_obj_t *l;

static void augh(void *augh)
{


l=NULL;
l=lv_label_create(lv_screen_active());
lv_obj_set_pos(l, 60, 50);
char buff[20];
const TickType_t xDelay = pdMS_TO_TICKS(2);
int timer=0;

while(1){
timer++;
TickType_t start_time = xTaskGetTickCount();
    lv_tick_inc(2);

if(timer>=1000)timer=0;

if(timer%8==0){
systemupdate();
}

TickType_t elapsed = xTaskGetTickCount() - start_time;
if (elapsed < xDelay) vTaskDelay(xDelay - elapsed);

for(int i=0;i<20;i++)
buff[i]='\0';
sprintf(buff, "%d", (int)elapsed);
lv_label_set_text(l,buff);
       }




}


void app_main(void){
systeminit();
xTaskCreate(augh, "system", 4096 * 2, NULL, 1, NULL);
                   }
