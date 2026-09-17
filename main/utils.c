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
#include "stdbool.h"
#include "utils.h"
#include <math.h>

//ahhh
int gsx;
int gsy;
int gcx;
int gcy;
//float scalefactor=((float)szx/(float)gsx);
float scalefactor;
int scx;
int scy;

void dli(int x1, int y1, int x2, int y2, int w, lv_color_t c,lv_layer_t* applayer){
    x1=scx+(scalefactor * (float)(x1-gcx));
    x2=scx+(scalefactor * (float)(x2-gcx));
    y1=scy+(scalefactor * (float)(y1-gcx));
    y2=scy+(scalefactor * (float)(y2-gcx));
//    w=(int)((float)w*scalefactor);
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = c;
    dsc.width = w;
    dsc.round_end = 1;
    dsc.round_start = 1;
    dsc.p1.x = x1;
    dsc.p1.y = y1;
    dsc.p2.x = x2;
    dsc.p2.y = y2;
    lv_draw_line(applayer, &dsc);
                                                                                 };



void drai(int x, int y, int w, int h, lv_color_t c,lv_layer_t* applayer){
    x=scx+(scalefactor * (float)(x-gcx));
    y=scy+(scalefactor * (float)(y-gcx));
    w=(float)w *scalefactor;
    h=(float)h * scalefactor;
//    w=(int)((float)w*scalefactor);
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.bg_color = c;
    dsc.border_color = c;
    dsc.border_width = 1;
    dsc.outline_width = 0;
    lv_area_t coords = {x-(w/2), y-(h/2), x+(w/2), y+(h/2)};
    lv_draw_rect(applayer, &dsc, &coords);
                                                                          };

void dri(rect r, lv_color_t c,
lv_layer_t* applayer){
drai(r.x,r.y,r.w,r.h,c,applayer);
}

void dleti(int x, int y, int h,char ahh, lv_color_t c,lv_layer_t* applayer){
    x=scx+(scalefactor * (float)(x-gcx));
    y=scy+(scalefactor * (float)(y-gcy));
    h=(float)h * scalefactor;
    lv_draw_label_dsc_t dsc;
    lv_draw_label_dsc_init(&dsc);
    dsc.color = c;
    dsc.font = &lv_font_montserrat_14;
lv_point_t pos = {
        .x = x,
        .y = y
    };
    lv_draw_character(applayer, &dsc, &pos, ahh);
                                                                          };


bool rpcol(int x1, int y1, int x2, int y2, int x, int y){
if((x>= x1 && x<=x2 && y>=y1 && y<=y2)||
(x<= x1 && x>=x2 && y>=y1 && y<=y2)) return true;
return false;

							}


bool rrcol(rect a, rect b) {
    float dx = fabs(a.x - b.x);
    float dy = fabs(a.y - b.y);
    return (dx < a.w + b.w) && (dy < a.h + b.h);
}

bool pccol(point p, circle c) {
    float dx = p.x - c.x;
    float dy = p.y - c.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSquared = c.r * c.r;
    return distanceSquared <= radiusSquared;
}


