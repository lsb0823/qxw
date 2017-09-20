

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
#include "sw_timer.h"
#ifdef MEDIA_PLAYER_SUPPORT
#include "resources.h"
#include "app_media_player.h"
#endif
#include "app_bt_media_manager.h"
#include "codec_best1000.h"
#include "hal_sleep.h"
#include "btapp.h"
#include "hal_iomuxip.h"




#if SOFTWARE_TIMER_EN
#if MULTI_KEY_ENABLE

#endif
extern struct BT_DEVICE_T  app_bt_device;

app_timer_t g_com_app_timer_vct[COM_APP_TIMER_MAX];

osTimerId timer_hooks_id = NULL;

//软定时器实现例程
void handle_timers(app_timer_t *timers, uint8_t count)
{
    app_timer_t *tm;
    uint32_t cur_time_ms = hal_sys_timer_get();
    uint32_t next_time_ms;
    uint32_t next_timeout;
    uint8_t i;
    bool need_proc;
    for (i = 0; i < count; i++)
    {
        tm = timers + i;
        if (tm->state != TIMER_STATE_RUNNING)
        {
            continue;
        }
        need_proc = FALSE;
        if (cur_time_ms >= tm->timeout_expires) //定时时间已到， 下一次超时的绝对定时点
        {
        	 //单发软定时器一次执行完后 kill 掉
            if (tm->type == TIMER_TYPE_SINGLE_SHOT)
            {
                tm->state = TIMER_STATE_NOUSED;
            }
            need_proc = TRUE;

            next_timeout = tm->timeout;

            next_time_ms = cur_time_ms + next_timeout;
			 //重设软定时器超时时间
            if (next_timeout > (cur_time_ms - tm->timeout_expires))
            {
                tm->timeout_expires += next_timeout;//直接在expires上加定时周期，以确保多次定时时间准确
            }
            else
            {
                tm->timeout_expires = next_time_ms;
            }
        }
        next_timeout = tm->timeout;

        next_time_ms = cur_time_ms + next_timeout;

        if (next_time_ms < tm->timeout_expires)
        {
            tm->timeout_expires = next_time_ms;
        }
		
        if ((need_proc == TRUE) && (tm->func_proc != NULL))
        {
        
		//TRACE("##############execute proc!!");
            tm->func_proc();
        }
        cur_time_ms = hal_sys_timer_get();
    }
	
}
void timer_1ms_hooks(const void *)
{
	handle_timers(g_com_app_timer_vct,COM_APP_TIMER_MAX);
	osTimerStart(timer_hooks_id,30);
}

osTimerDef(APP_SW_TIMER,timer_1ms_hooks);

void create_sw_timer(void)
{
	  init_app_timers(g_com_app_timer_vct,COM_APP_TIMER_MAX);
	  timer_hooks_id = osTimerCreate(osTimer(APP_SW_TIMER),osTimerOnce,NULL);
	  osTimerStart(timer_hooks_id,1);
}

//软定时器初始化 系统初始化的时候只调用一次
int8_t init_app_timers(app_timer_t *timers, uint8_t count)
{
    app_timer_t *tm;
    uint8_t i;
	
    if (count == 0)
    {
        return -1;
    }
	tm = timers;
    for (i = 0; i < count; i++)
    {
        tm = &tm [i];
        tm->state = TIMER_STATE_NOUSED;
    }
    return 0;
}

/*!创建定时器  返回定时器id  id范围0~7 */
int8_t set_app_timer(timer_type_e type, uint32_t timeout, timer_proc func_proc)
{
	app_timer_t *tm;
	int8_t timer_id = -1;
	uint8_t i;
	
	if (timeout == 0 || func_proc == NULL)
	{
		return -1;  
	}
	timeout = (timeout*15993)/1000;//打印显示,系统延时1000ms后计数器增加了15993，这里统一换成一个单位的hal_sys_timer_get()
	
    for (i = 0; i < COM_APP_TIMER_MAX; i++)
    {
        if (g_com_app_timer_vct[i].state == TIMER_STATE_NOUSED)
        {
            timer_id = i;
            tm = &g_com_app_timer_vct[timer_id];
      
            tm->timeout = timeout;
            tm->timeout_expires = hal_sys_timer_get() + timeout;
            tm->func_proc = func_proc;
            tm->state = TIMER_STATE_RUNNING;
            tm->type = type;
            break;
        }
    }
    return timer_id;
}

/*! 修改定时器的定时周期并重新开始计时  */
int8_t modify_app_timer(int8_t timerid,uint32_t timeout)
{
	app_timer_t *tm;
	if (timerid == -1 || timerid >= COM_APP_TIMER_MAX)
	{
		return -1;
	}
	timeout = (timeout*15993)/1000;
	tm = &g_com_app_timer_vct[timerid];
	tm->timeout = timeout;
	tm->timeout_expires = hal_sys_timer_get() + timeout;
	return 0;
}
/*!杀死定时器 kill掉后会把id修改成-1 */
int8_t kill_app_timer(int8_t *timer_id)
{
	app_timer_t *tm;
	if ((*timer_id == -1) || (*timer_id >= COM_APP_TIMER_MAX))//访问越界
	{
		return -1;
	}
	tm = &g_com_app_timer_vct[*timer_id];
	tm->state = TIMER_STATE_NOUSED;
	*timer_id = -1;
	return 0;
}
/*! 重启定时器,清零当前计时，重新开始计时 */
int8_t restart_app_timer(int8_t timer_id)
{
	app_timer_t *tm;
	if ((timer_id == -1) || (timer_id >= COM_APP_TIMER_MAX))//访问越界
	{
		return -1;
	}
	tm = &g_com_app_timer_vct[timer_id];
	tm->timeout_expires = hal_sys_timer_get() + tm->timeout;
    tm->state = TIMER_STATE_RUNNING;
	return 0;
}
/*!停止定时器,需要通过 restart_app_timer 来重启或者kill掉,定时器id还存在 */
int8_t stop_app_timer(int8_t timerid)
{
	app_timer_t *tm;
	if (timerid == -1 || timerid >= COM_APP_TIMER_MAX)
	{
		return -1;
	}
	tm = &g_com_app_timer_vct[timerid];
	tm->state = TIMER_STATE_STOPED;
	return 0;
}
#if MUTE_CTL_EN
const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_mute[1] = { 
    {HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
};
void mute_ctl_init(void)
{
	hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&cfg_mute[0], 1);
	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_mute[0].pin, HAL_GPIO_DIR_OUT, 0);

}
void spk_mute(void)
{
	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_mute[0].pin, HAL_GPIO_DIR_OUT, 0);
}
void spk_unmute(void)
{
	hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)cfg_mute[0].pin, HAL_GPIO_DIR_OUT, 1);
}
#endif





/******************************************************************************/
/*!
 * \par  Description:
 *    led状态轮询设置
 * \param[in]    none
 * \return       none
 * \note:每500ms更新一次LED状态,因为相同的状态app_status_indication_set函数会直接return,所以可以一直调用
 * \     也可以在每次打电话 播放暂停等状态改变的时候调用,但是没必要去找那些改变的代码在哪里,直接建定时器扫即可!!
 * \     要设置一个该函数里面没有的STATUS，可以在这个函数里面添加,或者在需要的地方直接调用,但是必须在该函数之前return掉以防这里修改了状态
 * \ 
 *******************************************************************************/
extern 	struct btdevice_volume *btdevice_volume_p;
extern bool g_bt_lost_flag;
extern void CloseEarphone(void);
extern int app_volume_save(uint8_t volume);
extern int app_volume_get(void);
extern bool poweron_flg;
void app_led_ind_set(void)
{
		u8 tmp;
	static uint16_t poweroff = 0;
	if (g_bt_lost_flag)
	{
		poweroff++;
		if(poweroff == 2*60*5)
		{
			CloseEarphone();
		}
	}else{
		poweroff = 0;
	}
	static uint16_t poweroncnt = 0;
	
	APP_STATUS_INDICATION_T state = APP_STATUS_INDICATION_NUM;
	state = app_status_indication_get();
	if (state == APP_STATUS_INDICATION_POWERON    || \
		state == APP_STATUS_INDICATION_INITIAL    || \
		state == APP_STATUS_INDICATION_POWEROFF   )
	{//优先级高的 直接return掉 不做轮询检测
		return;
	}
	if (poweron_flg)
	{
		poweroncnt++;
		if (poweroncnt >= 8*2 || MEC(activeCons)){
			poweron_flg = false;
		}
		//app_status_indication_set(APP_STATUS_INDICATION_RECNNT);
		//return;
	}
	if (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_IN)
	{
		app_status_indication_set(APP_STATUS_INDICATION_INCOMINGCALL);
		
	}
	else if (app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_OUT || app_bt_device.hfchan_callSetup[BT_DEVICE_ID_1] == HF_CALL_SETUP_ALERT)
	{
		app_status_indication_set(APP_STATUS_INDICATION_OUTGOING);
	}
	else if (app_bt_device.hfchan_call[BT_DEVICE_ID_1] == HF_CALL_ACTIVE)
	{
		app_status_indication_set(APP_STATUS_INDICATION_PHONE);
	}
	else if (MEC(activeCons) == 0)//未连接
	{
	    
		app_status_indication_set(APP_STATUS_INDICATION_BOTHSCAN);//双闪
	}
	else if (MEC(activeCons) != 0)//已连接
	{
		if (app_bt_device.a2dp_stream[BT_DEVICE_ID_1].stream.state == AVDTP_STRM_STATE_OPEN)//音频仅仅是打开 暂停状态
		{
			app_status_indication_set(APP_STATUS_INDICATION_MUS_PAUSE);
		}
		else if (app_bt_device.a2dp_stream[BT_DEVICE_ID_1].stream.state == AVDTP_STRM_STATE_STREAMING)//通道有数据流 说明在播放!!
		{
			app_status_indication_set(APP_STATUS_INDICATION_MUS_PLAY);
		}
	}
}
#if MULTI_KEY_ENABLE

//以下的实现方法这两个id第一次按键按下后就一直存在 即 不等于-1
int8_t g_key_timer_id = -1;
int8_t g_key_ignore_timer_id = -1;

extern BtStatus LinkDisconnectDirectly(void);

void key_ignore_proc(void)
{
	g_key_ignore_flag = false;
	kill_app_timer(&g_key_ignore_timer_id);
}
void key_check(void)
{
	static uint8_t key_cnt = 0;
	bool key1_sta = false;
	bool key2_sta = false;

	
	bool pwrkey_sta = false;
	
	uint32_t pwr_sta;
	
	pwr_sta = (iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX40_REG_OFFSET));
	
	pwr_sta = pwr_sta & IOMUXIP_MUX40_PWRKEY_VAL_MASK;

	if (pwr_sta)
	{
		pwrkey_sta = true;
	}
	
	if (hal_gpio_pin_get_val((HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[0].key_config.pin) == 0)
	{
		key1_sta = true;
	}
	if (hal_gpio_pin_get_val((HAL_GPIO_PIN_T)cfg_hw_gpio_key_cfg[1].key_config.pin) == 0)
	{
		key2_sta = true;
	}

	//创建忽略按键消息定时器 确保同时按下组合按键的时候不会响应其他按键消息
	if (g_key_ignore_timer_id == -1)
	{	
		g_key_ignore_timer_id = set_app_timer(TIMER_TYPE_SINGLE_SHOT,1000,(timer_proc)key_ignore_proc);
	}

    //  2个按键同时按下
	if (key1_sta && key2_sta)
	{
		g_key_ignore_flag = true;
		modify_app_timer(g_key_ignore_timer_id,1000);//只要同时按下 重新配置定时器定时时间并重启定时器
		key_cnt++;
	}
	else
	{
		if (!key1_sta && !key2_sta)
		{//2个按键都同时松开，退出检测
			kill_app_timer(&g_key_timer_id);
		}
		key_cnt = 0;
	}
	if (key_cnt >= 20*3 )//按键按下3秒，执行组合按键服务例程
	{
		key_cnt = 0;
		kill_app_timer(&g_key_timer_id);
	///	TRACE("execute multi key process.\n");
		if (MEC(activeCons) != 0)
		{
			LinkDisconnectDirectly();//  断开
		}
		else //回连
		{
			app_bt_profile_connect_manager_opening_reconnect();
		}
	}
}
void app_multikey(APP_KEY_STATUS *status, void *param)
{
	if (g_key_timer_id == -1)
	{
		g_key_timer_id = set_app_timer(TIMER_TYPE_NORMAL,50,key_check);
	}
}
#endif

#endif




















