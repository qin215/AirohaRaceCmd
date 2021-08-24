#ifndef SNIOT_TEST_MODE_H
#define SNIOT_TEST_MODE_H
#include <tchar.h>
#include "ServerCmd.h"
#include "homi_common.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define SNIOT_LINE_SIZE			(256)		// License �г�
#define SNIOT_MAC_ADDR_LEN		(12)		// MAC ��ַ����

enum SNIOT_CMD_TYPE
{
	SNIOT_MANUFATURE_ID = 0,		// ���쳧id�� 0
	SNIOT_MATERIAL_ID,				// ����	id�� 1
	SNIOT_SW_VER,					// ����汾�� 2
	SNIOT_HW_VER,					// Ӳ���汾�� 3
	SNIOT_PRODUCT_ID,				// ��ƷID�� 4
	SNIOT_PASSWORD,					// ��Ʒ���룬 5
	SNIOT_MAC_ADDR,					// MAC��ַ��6
	SNIOT_NUM
};


typedef struct sniot_lic_line_struct
{
	char line_buf[SNIOT_LINE_SIZE];				// license�м�¼
	Boolean used;								// license �Ƿ�ʹ��
} sniot_lic_line_t;



#define SNIOT_SUCCESS					(0)
#define SNIOT_ERROR_NOT_FOUND_MAC		(-1)
#define SNIOT_ERROR_ENTER_TST_MODE		(-2)
#define SNIOT_ERROR_WRITE_MAC			(-3)
#define SNIOT_ERROR_WRITE_ID			(-4)
#define SNIOT_ERROR_NOT_LIC_FILE		(-5)
#define SNIOT_ERROR_MAC_USED			(-6)
#define SNIOT_ERROR_READ_MAC			(-7)
#define SNIOT_ERROR_MAC_NOTEQUAL		(-8)

typedef void (*fn_send_msg)(void *dlg, const char *msg);

/*
* ����дMAC��ַָ��
*/
Boolean sniot_send_write_mac_atcmd(sniot_lic_line_t *line);

/*
* ����д������Ϣָ��
*/
Boolean sniot_send_write_info_atcmd(sniot_lic_line_t *line);


/*
 * ���ͽ������ģʽָ��
 */
Boolean sniot_send_enter_test_atcmd();

/*
* �����뿪����ģʽָ��
*/
Boolean sniot_send_leave_test_atcmd();


/*
 * ���MAC/ID��ʹ��
 */
Boolean SniotUpdateMachFlag(sniot_lic_line_t *plines, sniot_lic_line_t *pline);


/*
 * ��License File �л�ȡ���е�Mac/Id
 */
sniot_lic_line_t * SniotFindFreeMachAddr(sniot_lic_line_t *pline);

/*
 * ����License�ļ�
 */
Boolean SniotSaveLicenseFile(const TCHAR *lpszPath, sniot_lic_line_t *pline);

/*
 * ��ȡLicense�ļ�, ����lint_t����,
 * ��������һ��Ԫ�ص�line_buff����Ϊ0
 */
sniot_lic_line_t * SniotReadLicenseFile(const TCHAR *lpszPath);

/*
 * ��mac string�ҵ���Ӧ��license��
 * ����ֵ��
 *		NULL: ��ʾδ�ҵ�
 */
sniot_lic_line_t * sniot_find_lic_line(sniot_lic_line_t *plines, const char *mac_str);


/*
 * дһ��license key
 */
int sniot_write_license_id(const TCHAR *lpszPath, const char *mac_str, fn_send_msg fn, void *param);

void sniot_test_lic_file();

/*
* ��ȡlicense��ʹ�����
*/
Boolean sniot_get_license_used_info(sniot_lic_line_t *plines, int *total, int *used);

/*
 * ��ȡLicense�ļ�, ����lint_t����,
 * ��������һ��Ԫ�ص�line_buff����Ϊ0
 */
user_config_t * PwReadLicenseFile(const TCHAR *lpszPath);

/*
 * ��ȡlicense�ļ���iot2��ʽ��
 */
user_config_t * PwReadLicenseFileIot2(const TCHAR *lpszPath);

/*
 * ����License�ļ�
 */
Boolean PwSaveLicenseFile(const TCHAR *lpszPath, user_config_t *pline);

/*
 * ��ȡ��һ��δʹ�õ�MAC��ַ
 */
const char *PwGetFirstUnusedMac(const user_config_t *pline);

/*
 * ��ȡ���һ��δʹ�õ�MAC��ַ
 */
const char *PwGetLastUnusedMac(const user_config_t *pline);

/*
* ����δʹ�õ�mac��ַ����
*/
int PwGetUnusedNumber(const user_config_t *pline);

/*
* �������е�mac��ַ����
*/
int PwGetTotalNumber(const user_config_t *pline);

/*
 * ��ȡ���һ��MAC��ַ
 */
const char *PwGetLastMac(const user_config_t *pline);

/*
 * ��ȡ��һ��MAC��ַ
 */
const char *PwGetFirstMac(const user_config_t *pline);

/*
 * ���license�ļ����Ƿ������ͬ��MAC��ַ
 */
Boolean PwCheckSameMac(const user_config_t *pline);

/*
 * ��ȡlicense�ļ�����ͬ��MAC��ַ
 */
const char * PwGetSameMac(const user_config_t *pline);


struct __macaddr_mgmt_struct;

/*
 * ��License File �л�ȡ���е�Mac/Id
 * ���������
 *	pline: license ����
 *  paddr: �洢���ص�����
 *  n : ��Ҫ���صĸ���
 * ����ֵ��
 *	��Ч�����ݸ���
 */
int PwFindFreeMachAddr(user_config_t *pline, user_config_mgmt_t *paddr, int n);

/*
 * ��HEX��ʽ��MAC��ַתΪ������
 */
Boolean PwHexMac2Binary(const char *pmac, kal_uint8 bin_mac[6]);

/*
 * ���MAC/ID��ʹ��
 */
BOOL PwUpdateMachFlag(user_config_t *pline, user_config_mgmt_t *paddr);

/*
 * ���밢��OS��Ȩ���ñ�
 */
user_config_t * ReadAliosLicenseFile(const TCHAR *lpszPath);

/*
 * ��ʼ��ַΪpmac����n����ַ���µ�user_config_t��
 */
Boolean AliosFillMacAddress(user_config_t *pline, kal_uint8 pmac[6], int n);

/*
 * �ҵ���Ӧ��mac/name/secret��¼
 */
const user_config_t * AliosFindLineByMac(const user_config_t *plines, const char *mac_str);

#ifdef __cplusplus
}
#endif
#endif