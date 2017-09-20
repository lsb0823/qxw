#ifndef __AUD_OP_HANDLE_H__
#define __AUD_OP_HANDLE_H__

#ifdef __cplusplus
extern "C" {
#endif

enum ERR_CODE reg_op_handle_cmd(enum REG_OP_TYPE cmd, unsigned char *param, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif

