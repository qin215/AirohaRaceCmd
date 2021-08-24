#ifndef UA800_TEST_MODE_H
#define UA800_TEST_MODE_H
#include <tchar.h>
#include "ServerCmd.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define UA800_SUCCESS					(0)
#define UA800_ERROR_NOT_FOUND_MAC		(-1)
#define UA800_ERROR_ENTER_TST_MODE		(-2)
#define UA800_ERROR_WRITE_MAC			(-3)
#define UA800_ERROR_WRITE_ID			(-4)
#define UA800_ERROR_NOT_LIC_FILE		(-5)
#define UA800_ERROR_MAC_NOT_USED		(-6)
#define UA800_ERROR_READ_MAC			(-7)
#define UA800_ERROR_MAC_NOTEQUAL		(-8)
#define UA800_ERROR_MAC_FORMAT			(-9)
#define UA800_ERROR_MAC_USED			(-10)
#define UA800_ERROR_PRODUCT_MODE		(-11)
#define UA800_ERROR_DATABASE			(-12)

typedef void (*fn_send_msg)(void *dlg, const char *msg);

/*
* ����дMAC��ַָ��
* ������
* pmac: mac ��ַ��6���ֽ�
* ����ֵ��
*		TRUE, �ɹ�
*		FALSE, ʧ��
*/
Boolean ua800_send_write_mac_atcmd(kal_uint8 pmac[6]);

/*
 * дһ��license key
 *  ���̣�
 * 1. ���˳�����ģʽ
 * 2. �������ģʽ��ϵͳ�Զ�����
 * 3. д��MAC
 * 4. д��ID/PASSWORD
 * 5. ���MAC���Ƿ����
 * 6. �˳�����ģʽ
 */
int ua800_write_license_id(const char *lic_path, const char *mac_str, fn_send_msg fn, void *param, const char *server, Boolean db_support);

/*
 * ���ڻص�����
 */
void ua800_do_with_uart_rsp(buf_t *b);

/*******************************************************************
*����
     ���MAC��ַ�Ƿ���Ч
*����
     const kal_uint8 *pmac - MAC��ַ
*����ֵ
     TRUE		- ��Ч
	 FALSE		- ��Ч
*����˵��
	2018/11/26 by qinjiangwei
********************************************************************/
Boolean check_mac_valid(const char *pmac);

void dlg_update_status_ui(int rcv_count, int snd_cnt, int seq, const CString& info);

int t5506_send_get_raw_data();

int t5506_send_get_raw_data_2();

#ifdef __cplusplus
}
#endif
#endif