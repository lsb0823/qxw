#ifndef __sw_timer_h__
#define __sw_timer_h__

#include "apps.h"
typedef enum
{
    TIMER_TYPE_NORMAL,//���ڶ�ʱ��
    TIMER_TYPE_SINGLE_SHOT,//������ʱ��
} timer_type_e;

typedef enum
{
    TIMER_STATE_NOUSED,//û��ʹ��
    TIMER_STATE_RUNNING,//��������
    TIMER_STATE_STOPED,//ֹͣ
    
} timer_state_e;

/* ��ʱ���������� */
typedef void (*timer_proc)(void);
/*  ��ʱ�����ݽṹ�� */
typedef struct
{
    timer_state_e state;
    timer_type_e type;  
    uint32_t timeout;          //��ʱ�� ��ʱ����
    uint32_t timeout_expires;  //��ʱ�� ��һ�γ�ʱ����ʱ���
    timer_proc func_proc;      //��ʱ��������
} app_timer_t;

/* �����ʱ����Ŀ*/
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
