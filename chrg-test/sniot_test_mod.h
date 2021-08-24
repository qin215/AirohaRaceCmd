#ifndef SNIOT_TEST_MODE_H
#define SNIOT_TEST_MODE_H
#include <tchar.h>
#include "ServerCmd.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#define SNIOT_LINE_SIZE			(256)		// License 行长
#define SNIOT_MAC_ADDR_LEN		(12)		// MAC 地址长度

enum SNIOT_CMD_TYPE
{
	SNIOT_MANUFATURE_ID = 0,		// 制造厂id， 0
	SNIOT_MATERIAL_ID,				// 物料	id， 1
	SNIOT_SW_VER,					// 软件版本， 2
	SNIOT_HW_VER,					// 硬件版本， 3
	SNIOT_PRODUCT_ID,				// 产品ID， 4
	SNIOT_PASSWORD,					// 产品密码， 5
	SNIOT_MAC_ADDR,					// MAC地址，6
	SNIOT_NUM
};


typedef struct sniot_lic_line_struct
{
	char line_buf[SNIOT_LINE_SIZE];				// license行记录
	Boolean used;								// license 是否使用
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
* 发送写MAC地址指令
*/
Boolean sniot_send_write_mac_atcmd(sniot_lic_line_t *line);

/*
* 发送写所有信息指令
*/
Boolean sniot_send_write_info_atcmd(sniot_lic_line_t *line);


/*
 * 发送进入测试模式指令
 */
Boolean sniot_send_enter_test_atcmd();

/*
* 发送离开测试模式指令
*/
Boolean sniot_send_leave_test_atcmd();


/*
 * 标记MAC/ID已使用
 */
Boolean UpdateMachFlag(sniot_lic_line_t *plines, sniot_lic_line_t *pline);


/*
 * 从License File 中获取空闲的Mac/Id
 */
sniot_lic_line_t * FindFreeMachAddr(sniot_lic_line_t *pline);

/*
 * 保存License文件
 */
Boolean SaveLicenseFile(const TCHAR *lpszPath, sniot_lic_line_t *pline);

/*
 * 读取License文件, 返回lint_t数组,
 * 数组的最后一个元素的line_buff长度为0
 */
sniot_lic_line_t * ReadLicenseFile(const TCHAR *lpszPath);

/*
 * 从mac string找到对应的license行
 * 返回值：
 *		NULL: 表示未找到
 */
sniot_lic_line_t * sniot_find_lic_line(sniot_lic_line_t *plines, const char *mac_str);


/*
 * 写一个license key
 */
int sniot_write_license_id(const TCHAR *lpszPath, const char *mac_str, fn_send_msg fn, void *param);

void sniot_test_lic_file();

/*
* 获取license的使用情况
*/
Boolean sniot_get_license_used_info(sniot_lic_line_t *plines, int *total, int *used);


#ifdef __cplusplus
}
#endif
#endif