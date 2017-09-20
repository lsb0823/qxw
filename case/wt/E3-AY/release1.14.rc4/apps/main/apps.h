#ifndef __APPS_H__
#define __APPS_H__

#include "app_status_ind.h"

#ifdef __cplusplus
extern "C" {
#endif

int app_init(void);

int app_deinit(int deinit_case);

int app_shutdown(void);

int app_reset(void);

int app_status_battery_report(uint8_t level);

int app_voice_report( APP_STATUS_INDICATION_T status,uint8_t device_id);

/*FixME*/
void app_status_set_num(const char* p);

////////////10 second tiemr///////////////
#define APP_PAIR_TIMER_ID    0
#define APP_POWEROFF_TIMER_ID 1
void app_stop_10_second_timer(uint8_t timer_id);
void app_start_10_second_timer(uint8_t timer_id);

//软定时器  打开对系统无影响

#define SOFTWARE_TIMER_EN     1


// 组合按键，打开后同时按下V+ V-3秒会断开蓝牙，具体见key_check()函数 

#define MULTI_KEY_ENABLE      0 


//功放 耳放控制使能  

#define MUTE_CTL_EN           0

//打开功放 耳放的延时参数， 单位ms 即 有声音时延时 PA_DLY_TIME ms 后打开功放
#define PA_DLY_TIME   200


#define CHIP_ID_C     1
#define CHIP_ID_D     2


////////////////////


#ifdef __cplusplus
}
#endif
#endif//__FMDEC_H__
