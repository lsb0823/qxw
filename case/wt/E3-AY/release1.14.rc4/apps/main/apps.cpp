#include "stdio.h"
#include "cmsis_os.h"
#include "list.h"
#include "string.h"

#include "hal_timer.h"
#include "hal_trace.h"
#include "hal_bootmode.h"

#include "audioflinger.h"
#include "apps.h"
#include "app_thread.h"
#include "app_key.h"
#include "app_pwl.h"
#include "app_audio.h"
#include "app_overlay.h"
#include "app_battery.h"
#include "app_utils.h"
#include "app_status_ind.h"
#ifdef __FACTORY_MODE_SUPPORT__
#include "app_factory.h"
#include "app_factory_bt.h"
#endif
#include "bt_drv_interface.h"
#include "besbt.h"
#include "nvrecord.h"
#include "nvrecord_dev.h"
#include "nvrecord_env.h"

extern "C" {
#include "eventmgr.h"
#include "me.h"
#include "sec.h"
#include "a2dp.h"
#include "avdtp.h"
#include "avctp.h"
#include "avrcp.h"
#include "hf.h"
}
#include "btalloc.h"
#include "btapp.h"
#include "app_bt.h"

#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#include "app_bt_media_manager.h"
#include "codec_best1000.h"
#include "hal_sleep.h"
#include "btapp.h"
#include "hal_iomuxip.h"
#include "sw_timer.h"


#define APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD (1)


uint8_t  app_poweroff_flag = 0;

static uint8_t app_status_indication_init(void)
{
    struct APP_PWL_CFG_T cfg;
    memset(&cfg, 0, sizeof(struct APP_PWL_CFG_T));
    app_pwl_open();
    app_pwl_setup(APP_PWL_ID_0, &cfg);
    app_pwl_setup(APP_PWL_ID_1, &cfg);
    return 0;
}


#if defined(__EARPHONE__) && defined(__AUTOPOWEROFF__)

void PairingTransferToConnectable(void);

typedef void (*APP_10_SECOND_TIMER_CB_T)(void);

void app_pair_timerout(void);
void app_poweroff_timerout(void);
void CloseEarphone(void);

typedef struct
{
    uint8_t timer_id;
    uint8_t timer_en;
    uint8_t timer_count;
    uint8_t timer_period;
    APP_10_SECOND_TIMER_CB_T cb;
}APP_10_SECOND_TIMER_STRUCT;



APP_10_SECOND_TIMER_STRUCT app_pair_timer =
{
    .timer_id =APP_PAIR_TIMER_ID,
    .timer_en = 0,
    .timer_count = 0,
    .timer_period = 30,
    .cb =    CloseEarphone//PairingTransferToConnectable
};

APP_10_SECOND_TIMER_STRUCT app_poweroff_timer=
{
    .timer_id =APP_POWEROFF_TIMER_ID,
    .timer_en = 0,
    .timer_count = 0,
    .timer_period = 6*5,
    .cb =    CloseEarphone
};

APP_10_SECOND_TIMER_STRUCT  *app_10_second_array[2] =
{
    &app_pair_timer,
    &app_poweroff_timer,
};



void app_stop_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];
    timer->timer_en = 0;
    timer->timer_count = 0;
}

void app_start_10_second_timer(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];

    timer->timer_en = 1;
    timer->timer_count = 0;
}

void app_10_second_timerout_handle(uint8_t timer_id)
{
    APP_10_SECOND_TIMER_STRUCT *timer = app_10_second_array[timer_id];

    timer->timer_en = 0;
    timer->cb();
}




void app_10_second_timer_check(void)
{
    uint8_t i;
    for(i=0;i<sizeof(app_10_second_array)/sizeof(app_10_second_array[0]);i++)
    {
        if(app_10_second_array[i]->timer_en)
        {
            app_10_second_array[i]->timer_count++;
            if(app_10_second_array[i]->timer_count >= app_10_second_array[i]->timer_period)
            {
            	TRACE("###handle@@");
                app_10_second_timerout_handle(i);
            }
        }

    }
}
#endif

#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT
extern int app_hfp_battery_report(uint8_t battery_level);
#endif
int app_status_battery_report(uint8_t level)
{
#if defined(__EARPHONE__)
    app_bt_state_checker();
    app_10_second_timer_check();
#endif
    if (level<=APP_BATTERY_LEVEL_LOWPOWERTHRESHOLD){
        //add something
    }
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_BATTERY_REPORT
    app_hfp_battery_report(level);
#else
    TRACE("[%s] Can not enable HF_CUSTOM_FEATURE_BATTERY_REPORT",__func__);
#endif
    return 0;
}

#ifdef MEDIA_PLAYER_SUPPORT

void app_status_set_num(const char* p)
{
    media_Set_IncomingNumber(p);
}


extern bool g_bt_lost_flag;
extern int8_t g_led_timer_id;
extern "C" int app_voice_report(APP_STATUS_INDICATION_T status,uint8_t device_id)
{
    TRACE("%s %d",__func__, status);
    AUD_ID_ENUM id = MAX_RECORD_NUM;
    switch (status) {
        case APP_STATUS_INDICATION_POWERON:
            id = AUD_ID_POWER_ON;
            break;
        case APP_STATUS_INDICATION_POWEROFF:
            id = AUD_ID_POWER_OFF;
            break;    
        case APP_STATUS_INDICATION_CONNECTED:
			if (g_led_timer_id == -1)
			{
				g_led_timer_id = set_app_timer(TIMER_TYPE_NORMAL,500,app_led_ind_set);
			}

            id = AUD_ID_BT_CONNECTED;
            break;
		case APP_STATUS_INDICATION_MAXVOL:
			id = AUD_ID_BT_MAXVOL;
			break;
        case APP_STATUS_INDICATION_DISCONNECTED:
           // id = AUD_ID_BT_DIS_CONNECT;        
         
		// id = AUD_ID_BT_PAIR_ENABLE;
            break;    
        case APP_STATUS_INDICATION_CALLNUMBER:
            id = AUD_ID_BT_CALL_INCOMING_NUMBER;         
            break;  
        case APP_STATUS_INDICATION_CHARGENEED:
            id = AUD_ID_BT_CHARGE_PLEASE;   
            break;
        case APP_STATUS_INDICATION_FULLCHARGE:
            id = AUD_ID_BT_CHARGE_FINISH;
            break;  
        case APP_STATUS_INDICATION_PAIRSUCCEED:
         //   id = AUD_ID_BT_PAIRING_SUC;
            break;  
        case APP_STATUS_INDICATION_PAIRFAIL:
            id = AUD_ID_BT_PAIRING_FAIL;
            break;      
            
        case APP_STATUS_INDICATION_HANGUPCALL:
            id = AUD_ID_BT_CALL_HUNG_UP;
            break;  
            
        case APP_STATUS_INDICATION_REFUSECALL:
            id = AUD_ID_BT_CALL_REFUSE; 
            break;  
            
        case APP_STATUS_INDICATION_ANSWERCALL:
            id = AUD_ID_BT_CALL_ANSWER; 
            break;  
            
        case APP_STATUS_INDICATION_CLEARSUCCEED:
            id = AUD_ID_BT_CLEAR_SUCCESS;
            break;      
            
        case APP_STATUS_INDICATION_CLEARFAIL:
            id = AUD_ID_BT_CLEAR_FAIL;  
            break;
        case APP_STATUS_INDICATION_INCOMINGCALL:
            id = AUD_ID_BT_CALL_INCOMING_CALL;  
            break;
        case APP_STATUS_INDICATION_BOTHSCAN:
			
            id = AUD_ID_BT_PAIR_ENABLE;
            break;
        case APP_STATUS_INDICATION_WARNING:
            id = AUD_ID_BT_WARNING;
        default:
            break;
    }

    if ((app_poweroff_flag == 1)&&
        (status == APP_STATUS_INDICATION_BOTHSCAN||
        status == APP_STATUS_INDICATION_DISCONNECTED||
        status == APP_STATUS_INDICATION_CHARGENEED)){
        //block ring tong
    }else{
        media_PlayAudio(id, device_id);  
    }
    return 0;
}
#endif


#define POWERON_PRESSMAXTIME_THRESHOLD_MS  (5000)

enum APP_POWERON_CASE_T {
    APP_POWERON_CASE_NORMAL = 0,
    APP_POWERON_CASE_DITHERING,
    APP_POWERON_CASE_REBOOT,
    APP_POWERON_CASE_ALARM,
    APP_POWERON_CASE_CALIB,
    APP_POWERON_CASE_BOTHSCAN,
    APP_POWERON_CASE_CHARGING,
    APP_POWERON_CASE_FACTORY,
    APP_POWERON_CASE_TEST,
    APP_POWERON_CASE_INVALID,

    APP_POWERON_CASE_NUM
};


static osThreadId apps_init_tid = NULL;

static enum APP_POWERON_CASE_T pwron_case = APP_POWERON_CASE_INVALID;

static void app_poweron_normal(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_NORMAL;

    osSignalSet(apps_init_tid, 0x2);
}

static void app_poweron_scan(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    pwron_case = APP_POWERON_CASE_BOTHSCAN;

    osSignalSet(apps_init_tid, 0x2);
}

#ifdef __ENGINEER_MODE_SUPPORT__
static void app_poweron_factorymode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);    
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_factorymode_enter();
}
#endif

static void app_poweron_finished(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
    osSignalSet(apps_init_tid, 0x2);
}

void app_poweron_wait_finished(void)
{
    osSignalWait(0x2, osWaitForever);
}

#if  defined(__POWERKEY_CTRL_ONOFF_ONLY__)
void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param);
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},           "power on: shutdown"     , app_bt_key_shutdown, NULL},
};
#elif defined(__ENGINEER_MODE_SUPPORT__)
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGLONGPRESS},"power on: factory mode", app_poweron_factorymode  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#else
const  APP_KEY_HANDLE  pwron_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITUP},           "power on: normal"     , app_poweron_normal, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITLONGPRESS},    "power on: both scan"  , app_poweron_scan  , NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_INITFINISHED},     "power on: finished"   , app_poweron_finished  , NULL},
};
#endif
static void app_poweron_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);

    for (i=0; i<(sizeof(pwron_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&pwron_key_handle_cfg[i]);
    }
}

static uint8_t app_poweron_wait_case(void)
{
    uint32_t stime, etime;

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    pwron_case = APP_POWERON_CASE_NORMAL;
#else
    pwron_case = APP_POWERON_CASE_INVALID;

    stime = hal_sys_timer_get();
    osSignalWait(0x2, POWERON_PRESSMAXTIME_THRESHOLD_MS);
    etime = hal_sys_timer_get();
    TRACE("powon raw case:%d time:%d", pwron_case, TICKS_TO_MS(etime - stime));
#endif
    return pwron_case;

}

int system_shutdown(void);
int app_shutdown(void)
{
    system_shutdown();
    return 0;
}

int system_reset(void);
int app_reset(void)
{
    system_reset();
    return 0;
}

void app_bt_key_shutdown(APP_KEY_STATUS *status, void *param)
{
	{
		//TRACE("ignore function key\n");
		//return;
	}

    TRACE("%s %d,%d",__func__, status->code, status->event);
#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
    hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
    app_reset();
#else
    app_shutdown();
#endif
}

void app_bt_key_enter_testmode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s\n",__FUNCTION__);

    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__
        app_factorymode_bt_signalingtest(status, param);
#endif
    }
}

void app_bt_key_enter_nosignal_mode(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s\n",__FUNCTION__);
    if(app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN){
#ifdef __FACTORY_MODE_SUPPORT__        
        app_factorymode_bt_nosignalingtest(status, param);
#endif
    }
}


void app_bt_singleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_bt_doubleclick(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s %d,%d",__func__, status->code, status->event);
}

void app_power_off(APP_KEY_STATUS *status, void *param)
{
    TRACE("app_power_off\n");
}

extern "C" void OS_NotifyEvm(void);

#define PRESS_KEY_TO_ENTER_OTA_INTERVEL    (15000)          // press key 15s enter to ota
#define PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT    ((PRESS_KEY_TO_ENTER_OTA_INTERVEL - 2000) / 500) 
void app_otaMode_enter(APP_KEY_STATUS *status, void *param)
{
    TRACE("%s",__func__);
    hal_sw_bootmode_set(HAL_SW_BOOTMODE_ENTER_HIDE_BOOT);
#ifdef __KMATE106__
    app_status_indication_set(APP_STATUS_INDICATION_OTA);
    app_voice_report(APP_STATUS_INDICATION_WARNING, 0);
    osDelay(1200);
#endif
    hal_cmu_reset_set(HAL_CMU_MOD_P_GLOBAL);
}

void app_ota_key_handler(APP_KEY_STATUS *status, void *param)
{
    static uint32_t time = hal_sys_timer_get();
    static uint16_t cnt = 0;

    TRACE("%s %d,%d",__func__, status->code, status->event);

    if (TICKS_TO_MS(hal_sys_timer_get() - time) > 600) // 600 = (repeat key intervel)500 + (margin)100
        cnt = 0;
    else
        cnt++;

    if (cnt == PRESS_KEY_TO_ENTER_OTA_REPEAT_CNT) {
        app_otaMode_enter(NULL, NULL);
    }

    time = hal_sys_timer_get();
}
void app_bt_key(APP_KEY_STATUS *status, void *param)
{
    
	//if (g_key_ignore_flag)
	{
		//TRACE("ignore function key\n");
	//	return;
	}
	
	TRACE("%s %d,%d",__func__, status->code, status->event);
	
    switch(status->event)
    {
        case APP_KEY_EVENT_CLICK:
            TRACE("first blood!");
            break;
        case APP_KEY_EVENT_DOUBLECLICK:
            TRACE("double kill");
            break;      
        case APP_KEY_EVENT_TRIPLECLICK:
            TRACE("triple kill");
            break;                 
        case APP_KEY_EVENT_ULTRACLICK:
            TRACE("ultra kill");
            break;    
        case APP_KEY_EVENT_RAMPAGECLICK:
            TRACE("rampage kill!you are crazy!");
            break;            
    }
#ifdef __FACTORY_MODE_SUPPORT__
    if ((app_status_indication_get() == APP_STATUS_INDICATION_BOTHSCAN) && (status->event == APP_KEY_EVENT_DOUBLECLICK)&& (status->code == APP_KEY_CODE_PWR)){
     //   app_factorymode_languageswitch_proc();
    }else
#endif
    {
        bt_key_send(status->code, status->event);
    }
}
extern void app_multikey(APP_KEY_STATUS *status, void *param);

#ifdef __POWERKEY_CTRL_ONOFF_ONLY__
#if defined(__APP_KEY_FN_STYLE_A__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#else //#elif defined(__APP_KEY_FN_STYLE_B__)
const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_UP},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN3,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif

};
#endif
#else
#if defined(__APP_KEY_FN_STYLE_A__)

const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
#if RAMPAGECLICK_TEST_MODE    
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},
#endif    
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_LONGPRESS},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_LONGPRESS},"bt play forward key",app_bt_key, NULL},
#if MULTI_KEY_ENABLE
		{{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOWN},"bt volume up key",app_multikey, NULL},
		{{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOWN},"bt volume up key",app_multikey, NULL},
#endif

#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
};

#else //#elif defined(__APP_KEY_FN_STYLE_B__)

const APP_KEY_HANDLE  app_key_handle_cfg[] = {
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGLONGPRESS},"bt function key",app_bt_key_shutdown, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_LONGPRESS},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_CLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_DOUBLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_TRIPLECLICK},"bt function key",app_bt_key, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_ULTRACLICK},"bt function key",app_bt_key_enter_nosignal_mode, NULL},
    {{APP_KEY_CODE_PWR,APP_KEY_EVENT_RAMPAGECLICK},"bt function key",app_bt_key_enter_testmode, NULL},    
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_REPEAT},"bt volume up key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN1,APP_KEY_EVENT_UP},"bt play backward key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_REPEAT},"bt volume down key",app_bt_key, NULL},
    {{APP_KEY_CODE_FN2,APP_KEY_EVENT_UP},"bt play forward key",app_bt_key, NULL},
#if MULTI_KEY_ENABLE
	{{APP_KEY_CODE_FN1,APP_KEY_EVENT_DOWN},"bt volume up key",app_multikey, NULL},
	{{APP_KEY_CODE_FN2,APP_KEY_EVENT_DOWN},"bt volume up key",app_multikey, NULL},
#endif
	
#if HF_CUSTOM_FEATURE_SUPPORT & HF_CUSTOM_FEATURE_SIRI_REPORT
    {{APP_KEY_CODE_NONE ,APP_KEY_EVENT_NONE},"none function key",app_bt_key, NULL},
#endif
};
#endif
#endif

void app_key_init(void)
{
    uint8_t i = 0;
    TRACE("%s",__func__);
    for (i=0; i<(sizeof(app_key_handle_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&app_key_handle_cfg[i]);
    }
}
void app_key_init_on_charging(void)
{
    uint8_t i = 0;
	const APP_KEY_HANDLE  key_cfg[] = {
        {{APP_KEY_CODE_PWR,APP_KEY_EVENT_REPEAT},"ota function key",app_ota_key_handler, NULL},
	};

    TRACE("%s",__func__);
    for (i=0; i<(sizeof(key_cfg)/sizeof(APP_KEY_HANDLE)); i++){
        app_key_handle_registration(&key_cfg[i]);
    }
}

#ifdef __AUTOPOWEROFF__

void CloseEarphone(void)
{
    int activeCons;
    OS_LockStack();
    activeCons = MEC(activeCons);
    OS_UnlockStack();

    if(activeCons == 0)
    {
      TRACE("!!!CloseEarphone\n");
      app_shutdown();
    }
}
#endif /* __AUTOPOWEROFF__ */

//bool app_ledfresh = false;
BtStatus LinkDisconnectDirectly(void);
void a2dp_suspend_music_force(void);

int app_deinit(int deinit_case)
{
    int nRet = 0;
    TRACE("%s case:%d",__func__, deinit_case);

    if (!deinit_case){
#if FPGA==0
    nv_record_flash_flush();
#endif
    app_poweroff_flag = 1;

    app_status_indication_filter_set(APP_STATUS_INDICATION_BOTHSCAN);
    app_bt_stream_volumeset(TGT_VOLUME_LEVEL_0);
    a2dp_suspend_music_force();
    osDelay(200);
    LinkDisconnectDirectly();
    osDelay(500);
    app_status_indication_set(APP_STATUS_INDICATION_POWEROFF);
#ifdef MEDIA_PLAYER_SUPPORT
    app_voice_report(APP_STATUS_INDICATION_POWEROFF, 0);
#endif
    osDelay(2000);
//app_ledfresh = true;
#ifdef __CODEC_ASYNC_CLOSE__
    codec_best1000_real_close();
#else
    codec_best1000_close();
#endif
    }

    return nRet;
}

#ifdef APP_TEST_MODE
extern void app_test_init(void);
int app_init(void)
{
    int nRet = 0;
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE("%s",__func__);
    app_poweroff_flag = 0;

    apps_init_tid = osThreadGetId();
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);    
    list_init();
    af_open();
    app_os_init();
    app_pwl_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();
    if (app_key_open(true))
    {
        nRet = -1;
        goto exit;
    }

    app_test_init();
exit:
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    return nRet;
}
#else
#define NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS (0xffffaa55)
int app_bt_connect2tester_init(void)
{
    BtDeviceRecord rec;
    BT_BD_ADDR tester_addr;
    uint8_t i;
    bool find_tester = false;
    struct nvrecord_env_t *nvrecord_env;    
    btdevice_profile *btdevice_plf_p;
    nv_record_env_get(&nvrecord_env);

    if (nvrecord_env->factory_tester_status.status != NVRAM_ENV_FACTORY_TESTER_STATUS_DEFAULT)
        return 0;

    if (!nvrec_dev_get_dongleaddr(&tester_addr)){
        nv_record_open(section_usrdata_ddbrecord);
        for (i = 0; nv_record_enum_dev_records(i, &rec) == BT_STATUS_SUCCESS; i++) {
            if (!memcmp(rec.bdAddr.addr, tester_addr.addr, BD_ADDR_SIZE)){
                find_tester = true;
            }
        }
        if(i==0 && !find_tester){
            memset(&rec, 0, sizeof(BtDeviceRecord));
            memcpy(rec.bdAddr.addr, tester_addr.addr, BD_ADDR_SIZE);
            nv_record_add(section_usrdata_ddbrecord, &rec);
            btdevice_plf_p = (btdevice_profile *)app_bt_profile_active_store_ptr_get(rec.bdAddr.addr);
            btdevice_plf_p->a2dp_act = true;
            btdevice_plf_p->a2dp_act = true;
        }
        if (find_tester && i>2){
            nv_record_ddbrec_delete(&tester_addr);
            nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
            nv_record_env_set(nvrecord_env);
        }
    }

    return 0;
}

int app_nvrecord_rebuild(void)
{
    struct nvrecord_env_t *nvrecord_env;
    nv_record_env_get(&nvrecord_env);

    nv_record_sector_clear();
    nv_record_open(section_usrdata_ddbrecord);
    nvrecord_env->factory_tester_status.status = NVRAM_ENV_FACTORY_TESTER_STATUS_TEST_PASS;
    nv_record_env_set(nvrecord_env);
    nv_record_flash_flush();

    return 0;
}
extern int8_t app_volume_get(void);
extern int8_t stream_local_volume;
#if defined(__Qxw_BtAddr_Encrypt__)
extern U8 BT_Addr[6];
#endif
extern int8_t g_led_timer_id;
extern int8_t stream_local_volume;
extern 	struct btdevice_volume *btdevice_volume_p;

int app_init(void)
{
    int nRet = 0;
    struct nvrecord_env_t *nvrecord_env;
    bool need_check_key = true;
    uint8_t pwron_case = APP_POWERON_CASE_INVALID;
    TRACE("app_init\n");
#ifdef __WATCHER_DOG_RESET__
    app_wdt_open(15);
#endif

    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_52M);
    apps_init_tid = osThreadGetId();
    list_init();
    app_os_init();

   create_sw_timer();
   
#if MUTE_CTL_EN
	mute_ctl_init();
	spk_mute();
#endif
    app_status_indication_init();
    if (app_battery_open()){
        pwron_case = APP_POWERON_CASE_CHARGING;
        app_status_indication_set(APP_STATUS_INDICATION_CHARGING);
        app_battery_start();
        btdrv_start_bt();
        btdrv_hciopen();
        btdrv_sleep_config(1);
        btdrv_hcioff();
		app_key_open(false);
        app_key_init_on_charging();
        goto exit;
    }
    
    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_REBOOT){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_MASK);
        pwron_case = APP_POWERON_CASE_REBOOT;
        need_check_key = false;
        TRACE("REBOOT!!!");
    }

    if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_MODE){        
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MODE);
        pwron_case = APP_POWERON_CASE_TEST;
        need_check_key = false;
        TRACE("TESTER!!!");
    }
    
    if (app_key_open(need_check_key)){
        nRet = -1;
        goto exit;
    }

    hal_sw_bootmode_set(HAL_SW_BOOTMODE_REBOOT);

    app_bt_init();
    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();

    nv_record_env_init();
    nvrec_dev_data_open();
	
#if defined(__Qxw_BtAddr_Encrypt__)
    if ((BT_Addr[0] != 0x11) ||(BT_Addr[1] != 0x11) ||(BT_Addr[2] >= 0x42) ||(BT_Addr[2] < 0x11) ||(BT_Addr[3] != 0x11) || (BT_Addr[4] != 0x11) || (BT_Addr[5] != 0x11))
   {
        TRACE("NO No No No No No no!!!");
        nRet = -1;
        goto exit;
    }
#endif

    nv_record_env_get(&nvrecord_env);

#ifdef MEDIA_PLAYER_SUPPORT
    app_play_audio_set_lang(nvrecord_env->media_language.language);
#endif

    app_bt_stream_volume_ptr_update(NULL);
	btdevice_volume_p->a2dp_vol = app_volume_get();
	TRACE("#### A4 VOL:%d\n",btdevice_volume_p->a2dp_vol);
	app_bt_stream_volumeset(btdevice_volume_p->a2dp_vol);  

    if (pwron_case == APP_POWERON_CASE_REBOOT){
        btdrv_start_bt();
        BesbtInit();
        osDelay(600);
        TRACE("\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);        
        app_bt_accessmode_set(BAM_NOT_ACCESSIBLE);
        app_bt_profile_connect_manager_opening_reconnect();
        app_key_init();
        app_battery_start();
#if defined(__EARPHONE__) && defined(__AUTOPOWEROFF__)
        app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif
    }
#ifdef __ENGINEER_MODE_SUPPORT__
    else if(pwron_case == APP_POWERON_CASE_TEST){        
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);        
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
#ifdef __WATCHER_DOG_RESET__
        app_wdt_close();
#endif
        TRACE("!!!!!ENGINEER_MODE!!!!!\n");
        nRet = 0;
        btdrv_start_bt();
        BesbtInit();
        osDelay(600);
        app_nvrecord_rebuild();
        if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_SIGNALINGMODE){
            hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
            app_factorymode_bt_signalingtest(NULL, NULL);
            app_status_indication_set(APP_STATUS_INDICATION_CONNECTING);
        }
   //     if (hal_sw_bootmode_get() & HAL_SW_BOOTMODE_TEST_NOSIGNALINGMODE){            
    //        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_TEST_MASK);
   //         app_factorymode_bt_nosignalingtest(NULL, NULL);
   //     }
        app_factorymode_key_init();
    }
#endif
    else{
        app_status_indication_set(APP_STATUS_INDICATION_POWERON);
#ifdef MEDIA_PLAYER_SUPPORT
        app_voice_report(APP_STATUS_INDICATION_POWERON, 0);
#endif
        app_poweron_key_init();
        pwron_case = app_poweron_wait_case();
        {
            if (pwron_case != APP_POWERON_CASE_INVALID && pwron_case != APP_POWERON_CASE_DITHERING){
                TRACE("hello world i'm bes1000 hahaha case:%d\n", pwron_case);
                nRet = 0;
#if 1//ndef __POWERKEY_CTRL_ONOFF_ONLY__
                app_status_indication_set(APP_STATUS_INDICATION_INITIAL);
				
                osDelay(600);
#endif
                btdrv_start_bt();
                BesbtInit();
                osDelay(600);
                TRACE("\n\n\nbt_stack_init_done:%d\n\n\n", pwron_case);
                switch (pwron_case) {
                    case APP_POWERON_CASE_CALIB:
                        break;
                    case APP_POWERON_CASE_BOTHSCAN:

                        app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);
#ifdef MEDIA_PLAYER_SUPPORT
                        app_voice_report(APP_STATUS_INDICATION_BOTHSCAN,0);
#endif
#ifdef __EARPHONE__
                        app_bt_accessmode_set(BT_DEFAULT_ACCESS_MODE_PAIR);
#if defined(__AUTOPOWEROFF__)
                        app_start_10_second_timer(APP_PAIR_TIMER_ID);
#endif
#endif
						if (g_led_timer_id == -1)
						{
							g_led_timer_id = set_app_timer(TIMER_TYPE_NORMAL,500,app_led_ind_set);
						}

                        break;
                    case APP_POWERON_CASE_NORMAL:
#if defined( __EARPHONE__ ) && !defined(__EARPHONE_STAY_BOTH_SCAN__)
                        app_bt_accessmode_set(BAM_NOT_ACCESSIBLE);
#endif
                    case APP_POWERON_CASE_REBOOT:
                    case APP_POWERON_CASE_ALARM:
                    default:
						
                        app_status_indication_set(APP_STATUS_INDICATION_PAGESCAN);
#if defined( __EARPHONE__) && defined(__BT_RECONNECT__)
                        app_bt_profile_connect_manager_opening_reconnect();
#endif
						
                        break;
                }
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
                app_poweron_wait_finished();
#endif
                app_key_init();
                app_battery_start();
#if defined(__EARPHONE__) && defined(__AUTOPOWEROFF__)
                app_start_10_second_timer(APP_POWEROFF_TIMER_ID);
#endif


            }
			else{
                af_close();
                app_key_close();
                nRet = -1;
            }
        }
    }
exit:
    app_sysfreq_req(APP_SYSFREQ_USER_APP_0, APP_SYSFREQ_32K);
    return nRet;
}
#endif
