#include "stdint.h"
#include "stdbool.h"
#include "plat_types.h"
#include "hal_chipid.h"
#include "hal_trace.h"
#include "string.h"
#include "extend_cmd_handle.h"
#include "bt_op_handle.h"
#include "reg_op_handle.h"

static bool extend_cmd_inited = false;
static int (*send_reply_cb)(const unsigned char *, unsigned int);

int extend_cmd_init(int (* cb)(const unsigned char *, unsigned int))
{
    if (!extend_cmd_inited){
        send_reply_cb = cb;
        extend_cmd_inited = true;
        hal_chipid_init();  
        TRACE("\nchip metal id = %x\n", hal_get_chip_metal_id());
    }
    return 0;
}

int extend_cmd_send_reply(const unsigned char *payload, unsigned int len)
{
    return send_reply_cb(payload, len);
}

enum ERR_CODE extend_cmd_check_msg_hdr(struct message_t *msg)
{
    return ERR_NONE;
}

enum ERR_CODE extend_cmd_handle_cmd(enum EXTEND_CMD_TYPE cmd, unsigned char *param, unsigned int len)
{
    enum ERR_CODE nRet = ERR_NONE;

    switch (cmd) {
        case EXTEND_CMD_AUD_OP: {
            break;
        }
        case EXTEND_CMD_BT_OP: {
            nRet = bt_op_handle_cmd((enum BT_OP_TYPE)param[0], &param[1], len - 1);
            break;
        }
        case EXTEND_CMD_IO_OP: {
            break;
        }
        case EXTEND_CMD_REG_OP: {
            nRet = reg_op_handle_cmd((enum REG_OP_TYPE)param[0], &param[1], len - 1);
            break;
        }
        case EXTEND_CMD_DEVICINFO_OP: {
            break;
        }
        default: {
            TRACE("Invalid command: 0x%x", cmd);
            nRet = ERR_INTERNAL;
        }
    }

    return nRet;
}
