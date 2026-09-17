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
#include "apps.h"
#include "esp_random.h"


int mrand(){
static int next=125;
next=next*12737+7373;
int fard=(int)(next/9324)%6497;
return fard;
           }


int mcoin(){
int fard=mrand();
fard%=10;
if(fard<0)fard*=-1;
if(fard>=5)return 1;else return -1;
return fard;
           }


int dir;
int trappos;
int fishpos;
int gtimer;


void ifgam(){
gtimer=0;
dir=mcoin();
dir=mcoin();
dir=mcoin();
dir=mcoin();
dir=mcoin();
dir=mcoin();
trappos=20;
fishpos=5;

            }

void ufgam(){
//gtimer++;
trappos=10+fp;
//if(gtimer>=2){
gtimer=0;
fishpos+=dir*3;
if(fishpos>=180) dir=-dir;
if(fishpos<= -20) dir= -dir;
//	    }

lv_layer_t applayer;
lv_canvas_init_layer(appcanvas, &applayer);
rect s = {80,60,160,120};
dri(s,lv_palette_main(LV_PALETTE_BLUE),&applayer);
rect r = {fishpos,100,20,20};
dri(r,lv_palette_main(LV_PALETTE_GREEN),&applayer);
rect t = {65,trappos,25,25};
dri(t,lv_palette_main(LV_PALETTE_RED),&applayer);


lv_canvas_finish_layer(appcanvas,&applayer);

           }

void dfgam(){
            }
