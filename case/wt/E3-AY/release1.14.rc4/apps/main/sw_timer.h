#ifndef __sw_timer_h__
#define __sw_timer_h__

#include "apps.h"
typedef enum
{
    TIMER_TYPE_NORMAL,//周期定时器
    TIMER_TYPE_SINGLE_SHOT,//单发定时器
} timer_type_e;

typedef enum
{
    TIMER_STATE_NOUSED,//没有使用
    TIMER_STATE_RUNNING,//正在运行
    TIMER_STATE_STOPED,//停止
    
} timer_state_e;

/* 软定时器服务例程 */
typedef void (*timer_proc)(void);
/*  软定时器数据结构体 */
typedef struct
{
    timer_state_e state;
    timer_type_e type;  
    uint32_t timeout;          //软定时器 定时周期
    uint32_t timeout_expires;  //软定时器 下一次超时绝对时间点
    timer_proc func_proc;      //超时服务例程
} app_timer_t;

/* 最大软定时器数目*/
#define COM_APP_TIMER_MAX    4



extern void app_led_ind_set(void);

extern int8_t stop_app_timer(int8_t timerid);

extern int8_t restart_app_timer(int8_t timer_id);

extern int8_t kill_app_timer(int8_t *timer_id);

extern int8_t init_app_timers(app_timer_t *timers, uint8_t count);

extern int8_t modify_app_timer(int8_t timerid,uint32_t timeout);

extern int8_t set_app_timer(timer_type_e type, uint32_t timeout, timer_proc func_proc);

extern void create_sw_timer(void);

extern void spk_mute(void);

extern void spk_unmute(void);

extern void mute_ctl_init(void);



#endif
