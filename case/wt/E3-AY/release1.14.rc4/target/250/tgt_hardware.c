#include "cmsis_os.h"
#include "tgt_hardware.h"
#include "hal_iomux.h"
#include "hal_gpio.h"
#include "hal_key.h"

//   LED   À¶  ºì
const struct HAL_IOMUX_PIN_FUNCTION_MAP cfg_hw_pinmux_pwl[CFG_HW_PLW_NUM] = {
    {HAL_IOMUX_PIN_P1_6, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},
    {HAL_IOMUX_PIN_P1_7, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE},


};


//adckey define
const uint16_t CFG_HW_ADCKEY_MAP_TABLE[CFG_HW_ADCKEY_NUMBER] = {
    HAL_KEY_CODE_FN9,HAL_KEY_CODE_FN8,HAL_KEY_CODE_FN7,
    HAL_KEY_CODE_FN6,HAL_KEY_CODE_FN5,HAL_KEY_CODE_FN4,
    HAL_KEY_CODE_FN3,HAL_KEY_CODE_FN2,HAL_KEY_CODE_FN1
};

//gpiokey define     °´¼ü   
#define CFG_HW_GPIOKEY_DOWN_LEVEL          (0)
#define CFG_HW_GPIOKEY_UP_LEVEL            (1)
const struct HAL_KEY_GPIOKEY_CFG_T cfg_hw_gpio_key_cfg[CFG_HW_GPIOKEY_NUM] = {
    {HAL_KEY_CODE_FN1,{HAL_IOMUX_PIN_P1_4, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}},
    {HAL_KEY_CODE_FN2,{HAL_IOMUX_PIN_P1_5, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE}},

};

//bt config
const char *BT_LOCAL_NAME = "BT250\0";
uint8_t ble_addr[6] = {0xBE,0x99,0x34,0x45,0x56,0x67};
uint8_t bt_addr[6] = {0x50,0x02,0x34,0x88,0x56,0x67};

//audio config
//freq bands range {[0k:2.5K], [2.5k:5K], [5k:7.5K], [7.5K:10K], [10K:12.5K], [12.5K:15K], [15K:17.5K], [17.5K:20K]}
//gain range -12~+12
const int8_t cfg_aud_eq_sbc_band_settings[CFG_HW_AUD_EQ_NUM_BANDS] = {0, 0, 0, 0, 0, 0, 0, 0};

/* MAX GAIN 16*0.75+03*0+21*1=0db */
/* 16ohm Vpp>=800mV */
const struct  CODEC_DAC_VOL_T codec_dac_vol[CODEC_DAC_VOL_LEVEL_NUM]={
    {16, 3,  7}, /* -14db  VOL_WARNINGTONE */
    { 0, 0,  0}, /* -31db  VOL_LEVEL_0  */
    { 0, 0,  1}, /* -28db  VOL_LEVEL_1  */
    { 1, 3,  2}, /* -26db  VOL_LEVEL_2  */
    { 4, 3,  3}, /* -24db  VOL_LEVEL_3  */
    { 8, 3,  3}, /* -22db  VOL_LEVEL_4  */
    { 8, 3,  2}, /* -20db  VOL_LEVEL_5  */
    {16, 3,  1}, /* -18db  VOL_LEVEL_6  */
    {16, 3,  5}, /* -16db  VOL_LEVEL_7  */   
    {16, 3,  7}, /* -14db  VOL_LEVEL_8  */
    {16, 3,  9}, /* -12db  VOL_LEVEL_9  */
    {16, 3, 11}, /* -10db  VOL_LEVEL_10*/
    {16, 3, 13}, /* -8db    VOL_LEVEL_11*/
    {16, 3, 15}, /* -6db    VOL_LEVEL_12*/
    {16, 3, 17}, /* -4db    VOL_LEVEL_13 */
    {16, 3, 20}, /* -2db    VOL_LEVEL_14 */
    {16, 3, 23}, /*  0db    VOL_LEVEL_15 */
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_detecter_cfg = {
    HAL_IOMUX_PIN_P2_1,HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE
};

const struct HAL_IOMUX_PIN_FUNCTION_MAP app_battery_ext_charger_indicator_cfg = { 
	HAL_IOMUX_PIN_NUM, HAL_IOMUX_FUNC_AS_GPIO, HAL_IOMUX_PIN_VOLTAGE_VIO, HAL_IOMUX_PIN_PULLUP_ENALBE
};
