#ifndef _HX_FLASH_OPT_H_
#define _HX_FLASH_OPT_H_

#include "data_buff.h"
#include "uart_cmd.h"
#include "hekr_cmd.h"

Boolean hx_write_conf();
void hx_update_parameter(int *p_esp_char, int * p_esp_time, int * p_atpt_time, int * p_frame_length,
		hx_uart_t *p_uart, int *p_gpio_mode, Boolean *p_data_flag, hx_sock_t *p_sock_info);
void hx_update_conn_parameter(char *ssid, char *bssid, char *key);
void hx_init_conn_parameter(char *ssid, char *bssid, char *key);
Boolean hx_init_parameter(int *p_esp_char, int * p_esp_time, int * p_atpt_time, int * p_frame_length,
		hx_uart_t *p_uart, int *p_gpio_mode,  Boolean *p_data_flag, hx_sock_t *p_sock_info, Boolean *p_ap_mode,  U8 *p_gpio1_status);
void hx_update_echo_flag(Boolean *p_flag);
Boolean hx_init_echo_flag(Boolean *p_flag);
Boolean hm_init_smtlnk_flag(Boolean *p_flag);

extern int profile_size;

#endif
