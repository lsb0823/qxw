/***
 * besbt.h
 */

#ifndef BESBT_H
#define BESBT_H
#ifdef __cplusplus
extern "C" {
#endif
void BesbtInit(void);
void BesbtThread(void const *argument);
unsigned char *randaddrgen_get_bt_addr(void);
unsigned char *randaddrgen_get_ble_addr(void);
const char *randaddrgen_get_btd_localname(void);
#ifdef __cplusplus
}
#endif
#endif /* BESBT_H */
