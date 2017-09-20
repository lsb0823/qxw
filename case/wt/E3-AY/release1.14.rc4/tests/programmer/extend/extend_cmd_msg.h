#ifndef __EXTEND_CMD_MSG_H__
#define __EXTEND_CMD_MSG_H__

#ifdef __cplusplus
extern "C" {
#endif

enum EXTEND_CMD_TYPE {
    EXTEND_CMD_AUD_OP = 0xA1,
    EXTEND_CMD_BT_OP  = 0xA2,
    EXTEND_CMD_IO_OP  = 0xA3,
    EXTEND_CMD_REG_OP = 0xA4,
    EXTEND_CMD_DEVICINFO_OP = 0xA5,
};

enum BT_OP_TYPE {
    BT_OP_INIT   = 0x01,        
    BT_OP_DEINIT = 0x02,
    BT_OP_RESET  = 0x03,
    BT_OP_SIGNALING_TEST  = 0x04,
    BT_OP_NOSIGNALING_TEST = 0x05,
    BT_OP_CONTINUE_TEST  = 0x06,
    BT_OP_LO_TEST  = 0x07,
};

enum REG_OP_TYPE {
    REG_OP_WRITE = 0x01,        
    REG_OP_READ = 0x02,
};

#ifdef __cplusplus
}
#endif

#endif

