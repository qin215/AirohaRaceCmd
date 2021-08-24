
#if !defined(AFX_COMMON_H__INCLUDED_)
#define AFX_COMMON_H__INCLUDED_

#include "ServerCmd.h"
#include "data_buff.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define  WM_USER_TEST WM_USER+0x1001
#define WM_EDIT_END WM_USER+0x1002
#define WM_RECVDATA	WM_USER+0x1003
#define WM_CMDWIN_CLOSED	WM_USER+0x1004
#define WM_USER_EDIT_END WM_USER+1005
#define WM_PRINT_RX_MSG WM_USER+1006
#define WM_KILL_CMD_RX_THREAD	WM_USER+1007


#define WM_FW_UP_MSG WM_USER+1008
#define WM_EDIT_BEGIN	WM_USER+1009

#define WM_UPDATE_AMP_INDEX WM_USER+1010

#define MAX_DATA_BUF 8000

#define OP_TIMEOUT				3
#define READ_TIMEOUT			OP_TIMEOUT
#define WRITE_TIMEOUT			OP_TIMEOUT

#define MAX_COMM_BUF			1024
#define MAX_COMM_INPUT_BUF		MAX_COMM_BUF
#define MAX_COMM_OUTPUT_BUF		MAX_COMM_BUF


#define TEST_END 3
#define TEST_OK 2
#define TEST_FALSE 1

#define  __CRYPT_SUPPORT__			1				// 运行时加密支持
#define  __CUSTOMER_JXKJ__				


#define		CFG_FILE_POSTFIX		"Cfg File (*.csv)|*.csv|"

//#define		UA_TEST_MODE			// 自测模式
#define		USB_KEY_SUPPORT			0
#define		WIFI_EFUSE_SUPPORT		1

////For FSK
#define R_REGISTER_CMD	0x00
#define W_REGISTER_CMD	0x20
#define R_RX_PAYLOAD_CMD	0x61
#define W_TX_PAYLOAD_CMD	0xa0
#define FLUSH_TX_CMD		0xe1
#define FLUSH_RX_CMD		0xe2
#define REUSE_TX_PL_CMD		0xe3
#define ACTIVATE_CMD		0x50
#define R_RX_PL_WID_CMD		0x60
#define W_ACK_PAYLOAD_CMD	0xa8
#define W_TX_PAYLOAD_NOACK_CMD	0xb0
#define NOP_CMD			0xff

#define NORDIC_sBitCE   (1<<0)
#define NORDIC_sBitCSN  (1<<1)
#define NORDIC_sBitSCK  (1<<2)
#define NORDIC_sBitMOSI (1<<3)
#define NORDIC_sBitMISO (1<<4)
#define NORDIC_sBitCFG  (1<<5)

//for FPGA
#define FPGA_sBitCE_STC   (1<<1)
#define FPGA_sBitCSN_STC  (1<<5)
#define FPGA_sBitSCK_STC  (1<<2)
#define FPGA_sBitMOSI_STC (1<<6)
#define FPGA_sBitMISO_STC (1<<3)
#define FPGA_sBitCFG_STC  (1<<7)

//for Analog
#define ANALOG_sBitCE_STC   (1<<1)
#define ANALOG_sBitCSN_STC  (1<<2)
#define ANALOG_sBitSCK_STC  (1<<3)
#define ANALOG_sBitMOSI_STC (1<<5)
#define ANALOG_sBitMISO_STC (1<<4)
#define ANALOG_sBitCFG_STC  (1<<0)


#define REG0_CONFIG			0
#define REG1_EN_AA			1
#define REG2_EN_RXADDR		2
#define REG3_SETUP_AW		3
#define REG4_SETUP_RETR		4
#define REG5_RF_CH			5
#define REG6_RF_SETUP		6
#define REG7_STATUS			7
#define REG8_OBSERVE_TX		8
#define REG9_CD				9
#define REG10_RX_ADDR_P0	10
#define REG11_RX_ADDR_P1	11
#define REG12_RX_ADDR_P2	12
#define REG13_RX_ADDR_P3	13
#define REG14_RX_ADDR_P4	14
#define REG15_RX_ADDR_P5	15
#define REG16_TX_ADDR		16
#define REG17_RX_PW_P0		17
#define REG18_RX_PW_P1		18
#define REG19_RX_PW_P2		19
#define REG20_RX_PW_P3		20
#define REG21_RX_PW_P4		21
#define REG22_RX_PW_P5		22
#define REG23_FIFO_STATUS	23
#define REG28_DYNPDc		28
#define REG29_FEATUREc		29


#define TEST_ITEM_NUMBER 71


typedef enum __tag_UART_COMMAND_
{
UART_CMD_HOOK_ON         =0x01,
UART_CMD_DIAL_OUT        ,
UART_CMD_DIAL_OFF    ,
UART_CMD_ANSWER_CALL ,
UART_CMD_SINGLE_KEY   , 
UART_CMD_ADC_CHANNEL_SEL,
UART_CMD_SLOT_NUMBER   ,
UART_CMD_CHARGE_PARAM,
UART_CMD_AFC_PARAM,
};

#define ADC_CHNL_TYPE_ADC0           0
#define ADC_CHNL_TYPE_ADC1           1
#define ADC_CHNL_TYPE_ADC2           2
#define ADC_CHNL_TYPE_VBAT           3
#define ADC_CHNL_TYPE_PARALLEL_SET   4
#define ADC_CHNL_TYPE_RING           5
#define ADC_CHNL_TYPE_CID            6
#define ADC_CHNL_TYPE_MICDET         7
#define ADC_CHNL_TYPE_EARDET         8


#define 	GUISTYLE_XP   0x08001 	
#define		GUISTYLE_2003 0x08002
#define		GUISTYLE_2003MENUBTN 0x08021
#define		WIN_NT		  0x08003 	
#define		WIN_2000	0x08004
#define		WIN_XP		0x08005
#define		WIN_95		0x08006
#define		WIN_98		0x08007
#define		WIN_32		0x08008
#define		WIN_ME		0x08009
#define		WIN_95OSR2	0x0800A
#define		WIN_98_SE	0x0800B


#define MAX_READ_COM_LENGTH 1024

enum
{
   BEKEN_UART_LINK_CHECK=0x00,
    BEKEN_UART_REGISTER_WRITE_CMD=0x01,
    BEKEN_UART_REGISTER_CONINUOUSE_WRITE_CMD=2,
    BEKEN_UART_REGISTER_READ_CMD=3,
    BEKEN_UART_BT_START_CMD=4,
    BEKEN_UART_BT_STOP_CMD=5,
    BEKEN_UART_PATCH_CMD=6,
    BEKEN_UART_SET_PLATFORM_TYPE=7,
    BEKEN_UART_SET_UART_BAUDRATE=8,

    BEKEN_DISABLE_AFC=9,
    BEKEN_CONFIG_PCM=10,
    BEKEN_SET_UART_LOW_LEVEL_WAKEUP_VALUE=11,
    
    BEKEN_ENABLE_32K_SLEEP=12,
    BEKEN_ENABLE_HOST_WAKEUP=13,
    BEKEN_ENABLE_BT_WAKEUP=14,
    BEKEN_ENABLE_UART_LOW_LEVEL_WAKEUP=15,
    
    BEKEN_DISABLE_CPU_HALT_SLEEP=16,

    BEKEN_DISABLE_ANALOG_POWERDOWN=17,

    BEKEN_UART_MODULE_TEST_CMD=0x80,
    BEKEN_UART_MODULE_SUB_TEST_CMD,
    BEKEN_UART_MODULE_GENERAL_CMD

};

#define ST_G ( 0x3F << 10)
#define TCI_BEKEN_HARDWARE_TEST                                 0x00e0 + ST_G

#define HCI_COMMAND_COMPLETE_EVENT                         0x0E

enum
{
    TRA_HCIT_COMMAND = 1,
    TRA_HCIT_ACLDATA = 2,
    TRA_HCIT_SCODATA = 3,
    TRA_HCIT_EVENT   = 4,       
    TRA_HCIT_SREADTRUM  = 0xc0,       

    TRA_HCIT_BEKEN_UART_COMMAND = 0x1B

#ifdef SUPPORT_MTK_SPECAIL_COMMAND//support MTK command
,
    TRA_HCIT_MTK_COMMAND_ff = 0xff
#endif    
};


/*
 * 载入美的动态库, 初始化U盾
 */
Boolean init_midea_device(const char *keyfile);


/*
 * 将输入的license 信息 crypt_data 解密, 放入pdata中, len 至少>= 2K
 */
int midea_decrypt_license(const char *mac_str, const char *crypt_data, kal_uint8 *pdata, int len);

/*
 * 关闭U盾
 */
void midea_close_device();

#pragma pack(push)
#pragma pack(1)
typedef struct psensor_cali_struct
{
	UCHAR side;
	UCHAR cali_flag;
	UINT16 base_value;
	UINT16 gray_value;
} psensor_cali_data_t;

typedef struct race_cmd_relay_header_struct 
{
	BYTE dst_type;
	BYTE dst_id;
	BYTE partner_data[1];
} relay_rsp_t;

typedef struct race_cmd_sw_version_struct 
{
	BYTE status;
	BYTE recv_count;
	BYTE role;
	BYTE str_len;
	CHAR version[1];
} race_rsp_sw_version_t;

#define payload u.rpayload
#define sw_ver_rsp u.sw_rsp

typedef struct race_cmd_struct 
{
	UCHAR frame_start;
	UCHAR frame_type;
	UINT16 frame_len;
	UINT16 frame_cmd;

	union 
	{
		BYTE rpayload[1];
		relay_rsp_t rsp; 
		race_rsp_sw_version_t sw_rsp;
	} u;

} race_cmd_t;

typedef struct onewire_struct 
{
	uint8_t header;
	uint8_t type;
	uint16_t len;
	uint16_t cmd;
	uint8_t event;
	uint8_t side;
	uint8_t param[1];
}  onewire_frame_t;

#pragma pack(pop)

int process_data_buffer(kal_uint8 *pbuff, int len);

#ifdef __cplusplus
}
#endif

#endif

