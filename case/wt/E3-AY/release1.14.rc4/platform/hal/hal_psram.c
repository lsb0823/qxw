#include "plat_types.h"
#include "hal_psram.h"
#include "hal_psramip.h"
#include "hal_trace.h"
#include "hal_uart.h"
#include "hal_timer.h"
#include "hal_cmu.h"

#define HAL_PSRAM_YES 1
#define HAL_PSRAM_NO 0

#define HAL_PSRAM_CMD_REG_READ 0x40
#define HAL_PSRAM_CMD_REG_WRITE 0xc0
#define HAL_PSRAM_CMD_RAM_READ 0x00
#define HAL_PSRAM_CMD_RAM_WRITE 0x80

struct HAL_Psram_Context psram_ctx[HAL_PSRAM_ID_NUM];

static const char * const invalid_drv = "psram drv invalid";

static uint32_t _psram_get_reg_base(enum HAL_PSRAM_ID_T id)
{
    switch(id) {
        case HAL_PSRAM_ID_0:
        default:
            return 0x40150000;
            break;
    }
}

static void _psram_exitsleep_onprocess_wait(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);
    while (psramip_r_exit_sleep_onprocess(reg_base));

    while (!psramip_r_sleep_wakeup_state(reg_base));
}
static void _psram_busy_wait(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);
    while (psramip_r_busy(reg_base));
}
static void _psram_div(enum HAL_PSRAM_ID_T id)
{
    /* TODO */
}
/* hal api */
uint8_t hal_psram_open(enum HAL_PSRAM_ID_T id, struct HAL_PSRAM_CONFIG_T *cfg)
{
    uint32_t div = 0, reg_base = 0;
    //uint32_t psram_id = 0;
    reg_base = _psram_get_reg_base(id);

    hal_cmu_clock_enable(HAL_CMU_MOD_O_PSRAM);
    hal_cmu_clock_enable(HAL_CMU_MOD_H_PSRAM);
    hal_cmu_reset_clear(HAL_CMU_MOD_O_PSRAM);
    hal_cmu_reset_clear(HAL_CMU_MOD_H_PSRAM);

    /* over write config */
    if (cfg->override_config) {
        /* div */
        _psram_div(cfg->div);
    }
    else {
        div = cfg->source_clk/cfg->speed;
        _psram_div(div);
    }

    /* 0. dqs config */
    psramip_w_dqs_rd_sel(reg_base, cfg->dqs_rd_sel);
    psramip_w_dqs_wr_sel(reg_base, cfg->dqs_wr_sel);

    /* 1. high speed mode */
    if (cfg->speed >= HAL_PSRAM_SPEED_50M)
        psramip_w_high_speed_enable(reg_base, HAL_PSRAM_YES);
    else
        psramip_w_high_speed_enable(reg_base, HAL_PSRAM_NO);

    _psram_busy_wait(id);

    /* 2. wait calib done or FIXME timeout */
    psramip_w_enable_and_trigger_calib(reg_base);
    while (!psramip_r_calibst(reg_base));

    psramip_w_wrap_mode_enable(reg_base, HAL_PSRAM_YES);
    //psramip_w_wrap_mode_enable(reg_base, HAL_PSRAM_NO);

    //psramip_w_32bytewrap_mode(reg_base);
    psramip_w_1kwrap_mode(reg_base);
#if 0
    /* psram device register read 1 or 2 or 3 */
    psramip_w_acc_size(reg_base, 1);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_READ, 2);
    _psram_busy_wait(id);
    psram_id = psramip_r_rx_fifo(reg_base);

    uart_printf("psram id 0x%x\n", psram_id);
#endif

    return 0;
}

void hal_psram_suspend(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);

    psramip_w_acc_size(reg_base, 1);
    psramip_w_tx_fifo(reg_base, 0xf0);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_WRITE, 6);
    _psram_busy_wait(id);
}

void hal_psram_resume(enum HAL_PSRAM_ID_T id)
{
    uint32_t reg_base = 0;
    reg_base = _psram_get_reg_base(id);

    psramip_w_exit_sleep(reg_base);

    _psram_exitsleep_onprocess_wait(id);
}

void hal_psram_reg_dump(enum HAL_PSRAM_ID_T id)
{
#if 0
    uint32_t reg_base = 0;
    uint32_t psram_id = 0;
    reg_base = _psram_get_reg_base(id);

    /* psram device register read 1 or 2 or 3 */
    psramip_w_acc_size(reg_base, 1);
    psramip_w_cmd_addr(reg_base, HAL_PSRAM_CMD_REG_READ, 2);
    _psram_busy_wait(id);
    psram_id = psramip_r_rx_fifo(reg_base);

    uart_printf("psram id 0x%x\n", psram_id);
#endif
}

uint8_t hal_psram_close(enum HAL_PSRAM_ID_T id)
{
    hal_cmu_reset_set(HAL_CMU_MOD_H_PSRAM);
    hal_cmu_reset_set(HAL_CMU_MOD_O_PSRAM);
    hal_cmu_clock_disable(HAL_CMU_MOD_H_PSRAM);
    hal_cmu_clock_disable(HAL_CMU_MOD_O_PSRAM);

    return 0;
}
