#ifndef __APP_BT_H__
#define __APP_BT_H__

#ifdef __cplusplus
extern "C" {
#endif

enum APP_BT_REQ_T {
    APP_BT_REQ_ACCESS_MODE_SET,
    APP_BT_REQ_CONNECT_PROFILE,

    APP_BT_REQ_NUM
};

typedef void (*APP_BT_REQ_CONNECT_PROFILE_FN_T)(void *, void *);
#define app_bt_accessmode_set_req(accmode) do{app_bt_send_request(APP_BT_REQ_ACCESS_MODE_SET, accmode, 0, 0);}while(0)
#define app_bt_connect_profile_req(fn,param0,param1) do{app_bt_send_request(APP_BT_REQ_CONNECT_PROFILE, (uint32_t)param0, (uint32_t)param1, (uint32_t)fn);}while(0)

void PairingTransferToConnectable(void);

void app_bt_golbal_handle_init(void);

void app_bt_opening_reconnect(void);

void app_bt_accessmode_set(BtAccessibleMode mode);

void app_bt_send_request(uint32_t message_id, uint32_t param0, uint32_t param1, uint32_t ptr);

void app_bt_init(void);

int app_bt_state_checker(void);

void *app_bt_profile_active_store_ptr_get(uint8_t *bdAddr);

void app_bt_profile_connect_manager_open(void);

void app_bt_profile_connect_manager_opening_reconnect(void);

BOOL app_bt_profile_connect_openreconnecting(void *ptr);

#ifdef __cplusplus
}
#endif
#endif /* BESBT_H */
