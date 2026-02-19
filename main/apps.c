#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "apps.h"
#include "system.h"



lv_obj_t *label;
//lv_obj_t *label2;
//lv_obj_t *label3;
lv_obj_t *bar1;
lv_obj_t *bar2;

lv_style_t style_bg;
lv_style_t style_indic;

void initapps(){
iapp[0]();
               }

void initbars(){

//label2=lv_label_create(lv_screen_active());
//label3=lv_label_create(lv_screen_active());

bar1=lv_bar_create(lv_screen_active());
bar2=lv_bar_create(lv_screen_active());

    lv_style_init(&style_bg);
    lv_style_set_border_width(&style_bg, 0);

    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&style_indic, 3);

    lv_obj_remove_style_all(bar1);  /*To have a clean start*/
    lv_obj_add_style(bar1, &style_bg, 0);
    lv_obj_add_style(bar1, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(bar1, 160, 4);
    lv_obj_center(bar1);
    lv_bar_set_value(bar1, 100, LV_ANIM_ON);
 lv_obj_remove_style_all(bar2);  /*To have a clean start*/
    lv_obj_add_style(bar2, &style_bg, 0);
    lv_obj_add_style(bar2, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(bar2, 160, 4);
    lv_obj_center(bar2);
    lv_bar_set_value(bar2, 100, LV_ANIM_ON);

lv_bar_set_range(bar1,0,100);
lv_bar_set_range(bar1,0,100);
//lv_obj_set_width(bar1,160);
//lv_obj_set_width(bar2,160);
lv_obj_align(bar1,LV_ALIGN_BOTTOM_MID,0,0);
lv_obj_align(bar2,LV_ALIGN_BOTTOM_MID,0,-4);

//lv_obj_set_pos(label, 20, 20);
//lv_obj_set_pos(label2, 20, 8);
//lv_label_set_text(label, "Audio");
lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x00FF00), 0);

               }

void barsupdate(){
/*char buf[10];
sprintf(buf, "%d", fp);
lv_label_set_text(label2,buf);
*/
lv_bar_set_value(bar1,fp,LV_ANIM_OFF);
lv_bar_set_value(bar2,sp,LV_ANIM_OFF);
/*
char sbuf[10];
sprintf(sbuf, "%d", sp);
lv_label_set_text(label3,sbuf);
*/
                 }


void ia1(){
label=NULL;
label=lv_label_create(lv_screen_active());
lv_obj_set_pos(label, 20, 20);
lv_label_set_text(label, "app1");
          }

void ua1(){
          }

void da1(){
lv_obj_del(label);
          }



void ia2(){
label=NULL;
label=lv_label_create(lv_screen_active());
lv_obj_set_pos(label, 30, 30);
lv_label_set_text(label, "app2");
          }

void ua2(){
          }

void da2(){
lv_obj_del(label);

          }

void ia3(){
label=NULL;
label=lv_label_create(lv_screen_active());
lv_obj_set_pos(label, 50, 40);
lv_label_set_text(label, "app3");
          }

void ua3(){
          }

void da3(){
lv_obj_del(label);

          }

void (*uapp[appscount])()={ua1,ua2,ua3};
void (*iapp[appscount])()={ia1,ia2,ia3};
void (*dapp[appscount])()={da1,da2,da3};
