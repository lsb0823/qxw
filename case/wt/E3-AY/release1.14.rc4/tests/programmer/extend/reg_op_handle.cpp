#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "string.h"
#include "nv_factory_section.h"
#include "bt_drv_interface.h"
#include "hal_trace.h"
#include "hal_cache.h"
#include "hal_analogif.h"
#include "extend_cmd_handle.h"
#include "reg_op_handle.h"


#define BUFF_TO_U32(ptr,val) do{ \
                                val =( ((uint32_t) *((uint8_t*)ptr+3) << 24)   | \
                                      ((uint32_t) *((uint8_t*)ptr+2) << 16) | \
                                      ((uint32_t) *((uint8_t*)ptr+1) << 8)  | \
                                      ((uint32_t) *((uint8_t*)ptr)) ); \
                         }while(0)

#define U32_TO_BUFF(ptr,val) do{ \
                            *(ptr+3) = (uint8_t) (val>>24); \
                            *(ptr+2) = (uint8_t) (val>>16); \
                            *(ptr+1) = (uint8_t) (val>>8); \
                            *(ptr+0) = (uint8_t) val; \
                         }while(0)

enum ERR_CODE reg_op_handle_cmd(enum REG_OP_TYPE cmd, unsigned char *param, unsigned int len)
{
    uint32_t reg_reg;   
    uint16_t reg_val_16;
    uint32_t reg_val_32;
    uint8_t cret[5];
    cret[0] = ERR_NONE;

    switch (cmd) {
        case REG_OP_WRITE: {
            BUFF_TO_U32(param, reg_reg);            
            BUFF_TO_U32(param+4, reg_val_32);
            TRACE("REG_OP_WRITE %x=%x",reg_reg,reg_val_32);
            if (reg_reg>0xffff){                
                *(volatile unsigned int *)(reg_reg) = reg_val_32;
            }else{              
                hal_analogif_reg_write(reg_reg, reg_val_32);
            }
            extend_cmd_send_reply(cret,1);
            break;
        }
        case REG_OP_READ: {         
            BUFF_TO_U32(param, reg_reg);
            if (reg_reg>0xffff){                
                reg_val_32 = *(volatile unsigned int *)(reg_reg);

            }else{
                hal_analogif_reg_read(reg_reg, &reg_val_16);
                reg_val_32 = reg_val_16;
            }
            U32_TO_BUFF(&cret[1],reg_val_32);   
            TRACE("REG_OP_READ %x=%x",reg_reg,reg_val_32);
            extend_cmd_send_reply(cret, 5);
            break;
        }
        default: {
            TRACE("Invalid command: 0x%x", cmd);
            return ERR_INTERNAL;
        }
    }   
    return ERR_NONE;
}

