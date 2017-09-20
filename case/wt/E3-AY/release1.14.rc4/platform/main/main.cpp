#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_cmu.h"
#include "hal_timer.h"
#include "hal_dma.h"
#include "hal_iomux.h"
#include "hal_iomuxip.h"
#include "hal_wdt.h"
#include "hal_sleep.h"
#include "hal_bootmode.h"
#include "hal_trace.h"
#include "hal_chipid.h"
#include "cmsis.h"
#include "hwtimer_list.h"
#include "pmu.h"
#include "analog.h"
#include "apps.h"
#include "app_factory.h"
#include "hal_gpio.h"

#ifdef FPGA
#include "audioflinger.h"
#include "iabt.h"
#include "list.h"

#include "hal_iomux.h"
#include "App_audio.h"
#include "app_thread.h"

#endif

#ifdef __SBC_FUNC_IN_ROM__
#include "sbc.h"
#endif

#define  system_shutdown_wdt_config(seconds) \
                        do{ \
                            hal_wdt_stop(HAL_WDT_ID_0); \
                            hal_wdt_set_irq_callback(HAL_WDT_ID_0, NULL); \
                            hal_wdt_set_timeout(HAL_WDT_ID_0, seconds); \
                            hal_wdt_start(HAL_WDT_ID_0); \
                            hal_sleep_set_lowpower_hook(HAL_SLEEP_HOOK_USER_0, NULL); \
                        }while(0)

#define REG_SET_FIELD(reg, mask, shift, v)\
                        do{ \
                            *(volatile unsigned int *)(reg) &= ~(mask<<shift); \
                            *(volatile unsigned int *)(reg) |= (v<<shift); \
                        }while(0)

static osThreadId main_thread_tid = NULL;

int system_shutdown(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x4);
    return 0;
}

int system_reset(void)
{
    osThreadSetPriority(main_thread_tid, osPriorityRealtime);
    osSignalSet(main_thread_tid, 0x8);
    return 0;
}

int tgt_hardware_setup(void)
{
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)cfg_hw_pinmux_pwl, sizeof(cfg_hw_pinmux_pwl)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));

	    if (app_battery_ext_charger_indicator_cfg.pin != HAL_IOMUX_PIN_NUM){
            hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)&app_battery_ext_charger_indicator_cfg, 1);
            hal_gpio_pin_set_dir((enum HAL_GPIO_PIN_T)app_battery_ext_charger_indicator_cfg.pin, HAL_GPIO_DIR_OUT, 0);
        }
    return 0;
}


#ifdef __SBC_FUNC_IN_ROM__

SBC_ROM_STRUCT   SBC_ROM_FUNC = 
{
    .SBC_FrameLen = (__SBC_FrameLen )0x000085f5,
    .SBC_InitDecoder = (__SBC_InitDecoder )0x000086ad,
    .SbcSynthesisFilter4 = (__SbcSynthesisFilter4 )0x000086b9,
    .SbcSynthesisFilter8 = (__SbcSynthesisFilter8 )0x00008f0d,
    .SBC_DecodeFrames = (__SBC_DecodeFrames )0x0000a165,
    .SBC_DecodeFrames_Out_SBSamples = (__SBC_DecodeFrames_Out_SBSamples )0x0000a4e1
    
};
#endif


#ifdef FPGA
uint32_t a2dp_audio_more_data(uint8_t *buf, uint32_t len);
uint32_t a2dp_audio_init(void);

extern "C" void app_audio_manager_open(void);


//void IabtInit(void);

volatile uint32_t ddddd = 0;
//#define BT_AUDIO_PLAYBACK_BUFF_SIZE    1024*4

//extern uint8_t bt_audio_playback_buff[BT_AUDIO_PLAYBACK_BUFF_SIZE];
//extern void app_audio_open(void);
extern "C" void app_bt_init(void);
extern "C" uint32_t hal_iomux_init(struct HAL_IOMUX_PIN_FUNCTION_MAP *map, uint32_t count);
void app_overlay_open(void);

int main(void)
{
    struct AF_STREAM_CONFIG_T stream_cfg;
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP POSSIBLY_UNUSED pinmux_uart0[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP POSSIBLY_UNUSED pinmux_iic[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };

    hal_sys_timer_open();
    hal_audma_open();
    hal_gpdma_open();
    hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic, sizeof(pinmux_iic)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
    
    TRACE("\n[best of best of best...]\n");
    TRACE("\n[ps: w4 0x%x,2]", &ddddd);

    ddddd = 1;
    while (ddddd == 1);
    TRACE("bt start");

    list_init();

    a2dp_audio_init();
    IabtInit();
    app_os_init();

    af_open();
    app_audio_open();
    app_audio_manager_open();
    app_overlay_open();

	app_bt_init();
    
    while (1);

    return 0;
}


#else
int main(int argc, char *argv[])
{
    uint8_t sys_case = 0;
    int ret = 0;
#ifdef __FACTORY_MODE_SUPPORT__
    uint32_t bootmode = hal_sw_bootmode_get();
#endif

    static const struct HAL_IOMUX_PIN_FUNCTION_MAP POSSIBLY_UNUSED pinmux_iic[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_I2C_SCL, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_I2C_SDA, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP POSSIBLY_UNUSED pinmux_uart1[] = {
        {HAL_IOMUX_PIN_P3_2, HAL_IOMUX_FUNC_UART1_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_3, HAL_IOMUX_FUNC_UART1_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };
    static const struct HAL_IOMUX_PIN_FUNCTION_MAP POSSIBLY_UNUSED pinmux_uart0[] = {
        {HAL_IOMUX_PIN_P3_0, HAL_IOMUX_FUNC_UART0_RX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
        {HAL_IOMUX_PIN_P3_1, HAL_IOMUX_FUNC_UART0_TX, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    };

    tgt_hardware_setup();

    main_thread_tid = osThreadGetId();

    hwtimer_init();

    hal_dma_set_delay_func((HAL_DMA_DELAY_FUNC)osDelay);
    hal_audma_open();
    hal_gpdma_open();

#ifdef DEBUG
#if (DEBUG_PORT == 1)
#ifdef __FACTORY_MODE_SUPPORT__
    if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
    {
        hal_trace_open(HAL_TRACE_TRANSPORT_UART0);
    }
    hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart0, sizeof(pinmux_uart0)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
#endif

#if (DEBUG_PORT == 2)
#ifdef __FACTORY_MODE_SUPPORT__
        if (!(bootmode & HAL_SW_BOOTMODE_FACTORY))
#endif
        {
            hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_iic, sizeof(pinmux_iic)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
        }
        hal_iomux_init((struct HAL_IOMUX_PIN_FUNCTION_MAP *)pinmux_uart1, sizeof(pinmux_uart1)/sizeof(struct HAL_IOMUX_PIN_FUNCTION_MAP));
        hal_trace_open(HAL_TRACE_TRANSPORT_UART1);
#endif
#endif

    // disable bt spi access ana interface
    uint32_t v = 0;
    v = iomuxip_read32(IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);
    v &= ~(1<<2);
    iomuxip_write32(v, IOMUXIP_REG_BASE, IOMUXIP_MUX44_REG_OFFSET);

    TRACE("\nchip metal id = %x\n", hal_get_chip_metal_id());

    pmu_open();
    //fix me set digital gain
    REG_SET_FIELD(0x4000a050, 0x07, 24, 4); // [26:24] ch0 adc hbf3_sel 0 for *8 1 for *10 2 for *11 3 for *12 4 for *13 5 for *16
    REG_SET_FIELD(0x4000a050, 0x07, 27, 4); // [29:27] ch1 adc hbf3_sel
    REG_SET_FIELD(0x4000a050, 0x0f,  7, 6); // [10:7]  ch1 adc volume mic gain -12db(0001)~16db(1111) mute(0000)

    REG_SET_FIELD(0x4000a048, 0x0f, 25, 0x06); // // [28:25]  ch0 adc volume mic gain -12db(0001)~16db(1111) mute(0000)

    anlog_open();

#ifdef __FACTORY_MODE_SUPPORT__
    if (bootmode & HAL_SW_BOOTMODE_FACTORY){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_FACTORY);
        ret = app_factorymode_init(bootmode);

    }else if(bootmode & HAL_SW_BOOTMODE_CALIB){
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_CALIB);
        app_factorymode_calib_only();
    }else
#endif
    {
        ret = app_init();
    }

    if (!ret){
        while(1)
        {
            osEvent evt;
#ifndef __POWERKEY_CTRL_ONOFF_ONLY__
            osSignalClear (main_thread_tid, 0x0f);
#endif
            //wait any signal
            evt = osSignalWait(0x0, osWaitForever);

            //get role from signal value
            if(evt.status == osEventSignal)
            {
                if(evt.value.signals & 0x04)
                {
                    sys_case = 1;
                    break;
                }
                else if(evt.value.signals & 0x08)
                {
                    sys_case = 2;
                    break;
                }
            }else{
                sys_case = 1;
                break;
            }
         }
    }
#ifdef __WATCHER_DOG_RESET__
    system_shutdown_wdt_config(10);
#endif
    app_deinit(ret);
    TRACE("byebye~~~ %d\n", sys_case);
    if ((sys_case == 1)||(sys_case == 0)){
        TRACE("shutdown\n");
        hal_sw_bootmode_clear(HAL_SW_BOOTMODE_REBOOT);
        pmu_shutdown();
    }else if (sys_case == 2){
        TRACE("reset\n");
        hal_cmu_sys_reboot();
    }
    return 0;
}
#endif
