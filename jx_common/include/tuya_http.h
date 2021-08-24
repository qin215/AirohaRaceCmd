#ifndef _TUYA__HTTPC_H_
#define _TUYA__HTTPC_H_

#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG 1

#if DEBUG
#define PR_DEBUG(_fmt_, ...) \
        os_printf("[dbg]%s:%d "_fmt_"\n\r", __FILE__,__LINE__,##__VA_ARGS__)
#define PR_DEBUG_RAW(_fmt_, ...) \
        os_printf(_fmt_,##__VA_ARGS__)
#else
#define PR_DEBUG(...)
#define PR_DEBUG_RAW(_fmt_, ...)
#endif

#define PR_NOTICE(_fmt_, ...) \
        os_printf("[notice]%s:%d "_fmt_"\n\r", __FILE__,__LINE__,##__VA_ARGS__)
#define PR_ERR(_fmt_, ...) \
        os_printf("[err]%s:%d "_fmt_"\n\r", __FILE__,__LINE__,##__VA_ARGS__)


typedef int OPERATE_RET; // �����������ֵ

#if 0
#define OPRT_COMMON_START 0 // ͨ�÷���ֵ��ʼ
#define OPRT_OK 0             // ִ�гɹ�
#define OPRT_COM_ERROR 1       // ͨ�ô���
#define OPRT_INVALID_PARM 2    // ��Ч�����
#define OPRT_MALLOC_FAILED 3   // �ڴ����ʧ��
#define OPRT_COMMON_END OPRT_MALLOC_FAILED // ͨ�÷���ֵ����
#endif

/***********************************************************
*************************micro define***********************
***********************************************************/

#if 0
#define TUYA_DEV_GPRS_REGISTER    "tuya.device.gprs.register"
#define TUYA_DEV_GPRS_ACTV        "tuya.device.gprs.active"
#define TUYA_DEV_GPRS_REGION_GET  "tuya.device.gprs.region.get"
#define TUYA_DEV_GPRS_RESET       "tuya.device.gprs.reset"
#define TUYA_DEV_GPRS_EXIST       "tuya.device.exist"
#endif

enum 
{
	TUYA_WIFI_FIRMWARE = 1,
	TUYA_MCU_FIRMWARE = 9
};


//����������
#define TUYA_DEV_GPRS_REGISTER    			"tuya.device.register"
#define TUYA_DEV_GPRS_ACTV        			"tuya.device.active"
#define TUYA_DEV_GPRS_REGION_GET  			"tuya.device.region.get"
#define TUYA_DEV_GPRS_RESET       			"tuya.device.reset"
#define TUYA_DEV_GPRS_EXIST       			"tuya.device.exist"
#define TUYA_DEV_GPRS_GET      				"tuya.device.config.get"

//�����ϱ�
#define TUYA_DEV_GPRS_REPORT    			"tuya.device.dp.report"
#define TUYA_DEV_GPRS_VOICE_CONTROL			"tuya.device.voice.control"

//��������
#define TUYA_DEV_GPRS_UPDATE				"tuya.device.update"
#define TUYA_DEV_GPRS_UPGRADE_GET			"tuya.device.upgrade.get"
#define TUYA_DEV_GPRS_UPGRADE_STATUS_PUT	"tuya.device.upgrade.status.update"

// ���豸����
#define TUYA_DEV_GPRS_SUB_BIND				"tuya.device.sub.bind"
#define TUYA_DEV_GPRS_SUB_UNBIND			"tuya.device.sub.unbind"
#define TUYA_DEV_GPRS_SUB_ONLINE			"tuya.device.sub.online"
#define TUYA_DEV_GPRS_SUB_OFFLINE			"tuya.device.sub.offline"
/***********************************************************
*************************variable define********************
***********************************************************/


/***********************************************************
*************************function define********************
***********************************************************/

#define ICACHE_FLASH_ATTR 

/***********************************************************
*  Function: httpc_aes_init
*  Input:
*  Output:
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET httpc_aes_init(void);

/***********************************************************
*  Function: httpc_aes_set
*  Input:
*  Output:
*  Return: OPERATE_RET
***********************************************************/
OPERATE_RET httpc_aes_set(const unsigned char *key, const unsigned char *iv);



/***********************************************************
*  Function: httpc_gw_dev_active_pk
*  Input: dev_if
*  Output:
*  Return: OPERATE_RET
*  ˵��:ͨ��prodect_key����,���ڵ�Ʒ�豸��+ʵ�����ظ��������豸��
***********************************************************/

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_register(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_active(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_reset(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_region_get(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_exist(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_dev_get(char *http_data);

OPERATE_RET ICACHE_FLASH_ATTR httpc_gprs_upgrade_get(char *http_data);

#ifdef __cplusplus
}
#endif
#endif
