#ifndef EXTEND_CMD_HANDLE_H
#define EXTEND_CMD_HANDLE_H

#include "tool_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

int extend_cmd_init(int (* cb)(const unsigned char *, unsigned int));

int extend_cmd_send_reply(const unsigned char *payload, unsigned int len);

enum ERR_CODE extend_cmd_check_msg_hdr(struct message_t *msg);

enum ERR_CODE extend_cmd_handle_cmd(enum EXTEND_CMD_TYPE cmd, unsigned char *param, unsigned int len);


#ifdef __cplusplus
}
#endif
#endif
