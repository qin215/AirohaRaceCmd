#ifndef UART_OBD_TEST_H
#define UART_OBD_TEST_H

#ifdef __cplusplus
extern "C"
{
#endif

void obd_send_random_pack();
void obd_log_recv_buff(buf_t *b);
void obd_send_heart_pack();
void obd_log_uart_rx_data(buf_t *b);

#define OBD_RANDOM_MAX_PACK_SIZE	(128)

#if OBD_RANDOM_MAX_PACK_SIZE > BUF_SIZE
#error "OBD_RANDOM_MAX_PACK_SIZE is too large"
#endif

#define OBD_TEST_SERVER "192.168.0.10"
#define OBD_TEST_PORT 35000

#ifdef __cplusplus
}
#endif

#endif

