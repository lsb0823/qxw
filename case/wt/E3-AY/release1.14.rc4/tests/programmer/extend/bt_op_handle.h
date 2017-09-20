#ifndef __BT_OP_HANDLE_H__
#define __BT_OP_HANDLE_H__

#ifdef __cplusplus
extern "C" {
#endif

int btdut_operation_start(void);

int btdut_operation_end(void);

int btdut_nosignalingtest_start(unsigned char *param, unsigned int len);

int btdut_nosignalingtest_resultwait(unsigned char *param, unsigned int *len);

enum ERR_CODE bt_op_handle_cmd(enum BT_OP_TYPE cmd, unsigned char *param, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif

