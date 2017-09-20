#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "string.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "hal_cache.h"
#include "hal_analogif.h"
#include "hal_timer.h"
#include "hal_intersys.h"
#include "nv_factory_section.h"
#include "bt_drv_interface.h"
#include "extend_cmd_handle.h"
#include "bt_op_handle.h"

struct ext_hdl_btnosigtester_cmd_setup
{
    /// test scenario
    uint8_t    test_scenario;
        /// test scenario
    uint16_t    test_period;
    /// hopping mode
    uint8_t    hopping_mode;
    /// whitening mode;
    uint8_t    whitening_mode;  
    /// transmit frequency
    uint8_t    tx_freq;
    /// receive frequency
    uint8_t    rx_freq;
    /// power level
    uint8_t    power_level;
    /// BD Address in nonsig_tester
    struct bd_addr{
        ///6-byte array address value
        uint8_t  addr[6];
    } bd_addr;
    /// LT Address
    uint8_t    lt_addr;
    /// edr_enabled 
    uint8_t    edr_enabled; 
    /// packet type
    uint8_t    packet_type; 
    /// payload pattern
    uint8_t    payload_pattern;
    /// payload size
    uint16_t   payload_length;
};

struct ext_hdl_btnosigtester_cmd_result
{
    uint16_t    pkt_counters;   
    uint16_t    head_errors;
    uint16_t    payload_errors;
    uint16_t    avg_estsw;
    uint16_t    avg_esttpl;
    uint32_t    payload_bit_errors;
};

struct btdut_operation_rx_buf
{
    uint8_t buf[255];
    uint8_t len;
};

#define BTDUT_WRITE_RF_REG(reg,val) hal_analogif_reg_write(reg,val)
#define BTDUT_DIGITAL_REG(a) (*(volatile uint32_t *)(a))
#define BTDUT_DELAY(ms) hal_sys_timer_delay(MS_TO_TICKS(ms))
#define BTDUT_BUFF_TO_U32(ptr,val) do{ \
                                val =( ((uint32_t) *((uint8_t*)ptr+3) << 24)   | \
                                      ((uint32_t) *((uint8_t*)ptr+2) << 16) | \
                                      ((uint32_t) *((uint8_t*)ptr+1) << 8)  | \
                                      ((uint32_t) *((uint8_t*)ptr)) ); \
                         }while(0)

#define BTDUT_SIGNAL_WAIT(to) do{ \
                                uint8_t i=50; \
                                *to = true; \
                                do{ \
                                   if (gbtdud_signal){ \
                                        gbtdud_signal =  false; \
                                        *to = false; \
                                        break; \
                                   }else{ \
                                      BTDUT_DELAY(100); \
                                   } \
                                }while(i--);\
                            }while(0)
                               
#define BTDUT_SIGNAL_MUTUX_SET() do{ \
                                gbtdud_signal = true; \
                           }while(0)

static bool gbtdud_signal = false;
static struct btdut_operation_rx_buf operation_rx_buf;

static int btdut_senddata(const uint8_t *buff,uint8_t len)
{
    hal_intersys_send(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI, buff, len);
    BTDUT_DELAY(1);
    return 0;
}

static int btdut_nosignalingtest_resulthook(const unsigned char *data, unsigned int len)
{   
    unsigned char nonsig_test_report[] = {0x04, 0x0e, 0x12};
    if (0 == memcmp(data, nonsig_test_report, sizeof(nonsig_test_report))){
        BTDUT_SIGNAL_MUTUX_SET();
    }
    return 0;
}

static unsigned int btdut_operation_rx(const unsigned char *data, unsigned int len)
{
    hal_intersys_stop_recv(HAL_INTERSYS_ID_0);

    memcpy(operation_rx_buf.buf, data, len);
    operation_rx_buf.len = len;
    btdut_nosignalingtest_resulthook(data, len);
    hal_intersys_start_recv(HAL_INTERSYS_ID_0);
    return len;
}

int btdut_operation_start(void)
{
    int ret = 0;

    ret = hal_intersys_open(HAL_INTERSYS_ID_0, HAL_INTERSYS_MSG_HCI, btdut_operation_rx, NULL);

    if (ret) {
        return -1;
    }
    gbtdud_signal = false;
    hal_intersys_start_recv(HAL_INTERSYS_ID_0);
    return 0;
}

int btdut_operation_end(void)
{
    btdrv_hci_reset();
    BTDUT_DELAY(200);
    hal_intersys_close(HAL_INTERSYS_ID_0,HAL_INTERSYS_MSG_HCI);
    return 0;
}

int btdut_nosignalingtest_start(unsigned char *param, unsigned int len)
{
    uint8_t hci_cmd_nonsig_buf[24] = {
        0x01, 0x60, 0xfc, 0x14
    };
    memcpy(&hci_cmd_nonsig_buf[4], param, len);
    btdut_senddata(hci_cmd_nonsig_buf, sizeof(hci_cmd_nonsig_buf));
    BTDUT_DELAY(100);
    return 0;
}

int btdut_nosignalingtest_resultwait(unsigned char *param, unsigned int *len)
{
    bool timeout;
    int nRet;
    
    BTDUT_SIGNAL_WAIT(&timeout);
    if (!timeout){
        *len = operation_rx_buf.len-7;
        memcpy(param, operation_rx_buf.buf+7, *len);
        nRet = 0;
    }else{
        nRet = -1;
    }

    return nRet;
}

enum ERR_CODE bt_op_handle_cmd(enum BT_OP_TYPE cmd, unsigned char *param, unsigned int len)
{
    static unsigned short c8_calib_val = 0;
    uint32_t continue_test_param;   
    unsigned char cret[20];
    unsigned int clen;
    int nRet = 0;

    cret[0] = ERR_NONE;

    switch (cmd) {
        case BT_OP_INIT: {
            nv_factory_section_open();
            btdrv_testmode_start();
            btdrv_hciopen();
            btdrv_sleep_config(0);
            btdrv_hcioff();
            hal_analogif_reg_read(0xc8, &c8_calib_val);
            extend_cmd_send_reply(cret,1);
            break;
        }
        case BT_OP_DEINIT: {
            btdrv_stop_bt();
            extend_cmd_send_reply(cret,1);
            break;
        }
        case BT_OP_RESET: {
            nv_factory_section_open();
            btdrv_hciopen();
            btdrv_hci_reset();
            btdrv_sleep_config(0);
            BTDUT_DELAY(800);
            btdrv_hcioff();         
            extend_cmd_send_reply(cret,1);
            break;
        }
        case BT_OP_SIGNALING_TEST: {            
            nv_factory_section_data_t *factory_section_data_p = NULL;
            btdrv_hciopen();            
            nv_factory_section_open();
            factory_section_data_p = nv_factory_section_get_data_ptr();
            if (factory_section_data_p){
                btdrv_write_localinfo((char *)factory_section_data_p->device_name, strlen((char *)factory_section_data_p->device_name), factory_section_data_p->bt_address);
                BTDUT_WRITE_RF_REG(0xC2, (factory_section_data_p->xtal_fcap & 0xff)<<8);
            }           
            btdrv_enable_dut();
            btdrv_hcioff();         
            extend_cmd_send_reply(cret,1);
            break;
        }
        case BT_OP_NOSIGNALING_TEST: {
            btdut_operation_start();
            btdut_nosignalingtest_start(param,len);
            nRet = btdut_nosignalingtest_resultwait(&cret[1],&clen);
            btdut_operation_end();
            if (!nRet)
                extend_cmd_send_reply(cret,clen+1);
            else
                nRet = ERR_INTERNAL;
            break;
        }
        case BT_OP_CONTINUE_TEST: {
            BTDUT_BUFF_TO_U32(param, continue_test_param);            
            BTDUT_DIGITAL_REG(0xd02201bc) = continue_test_param;
            extend_cmd_send_reply(cret,1);
            break;
        }
        case BT_OP_LO_TEST: {
            if (*param & 0x80){            
                BTDUT_DIGITAL_REG(0xd02201bc) = (*param & 0x7f) | 0xa0000;                
                BTDUT_DELAY(10);
                BTDUT_DIGITAL_REG(0xd02201bc) = 0;
                BTDUT_DELAY(10);                
                BTDUT_DIGITAL_REG(0xd0340020) &= (~0x7);
                BTDUT_DIGITAL_REG(0xd0340020) |= 6;
                BTDUT_DELAY(10);
                hal_analogif_reg_write(0xc8, 0xffff);
            }else{        
                BTDUT_DIGITAL_REG(0xd02201bc) = 0;                
                BTDUT_DIGITAL_REG(0xd0340020) &=(~0x7);
                hal_analogif_reg_write(0xc8, c8_calib_val);
                BTDUT_DELAY(10);
            }
            extend_cmd_send_reply(cret,1);
            break;
        }
        default: {
            TRACE("Invalid command: 0x%x", cmd);
            nRet = ERR_INTERNAL;
        }
    }
    return (enum ERR_CODE)nRet;
}

