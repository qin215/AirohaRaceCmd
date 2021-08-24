/*
 * Copyright (c) 2016 Beijing Secsmarts Technology Co., Ltd. All rights reserved.	
 *
 * File Name 		: ProductorDll.h
 * Introduction		: Macro and type define of productor SDK
 *
 * Current Version	: v2.0.2
 * Author			: Done!
 * Create Time		: 2016/03/05
 * Change Log		: 
 *
 * All software and related documentation herein (Secsmarts Software) are intellectual 
 * property of Beijing Secsmarts Technology Co., Ltd. and protected by law, including, 
 * but not limited to, copyright law and international treaties.
 *
 * Any use, modification, reproduction, retransmission, or republication of all
 * or part of Secsmarts Software is expressly prohibited, unless prior written
 * permission has been granted by Secsmarts.
 */

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号

#ifdef __cplusplus
extern "C"
{
#endif

class CProductorDllApp : public CWinApp
{
public:
	CProductorDllApp();

// 重写
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};

#define _RSA_SYSTEM_
//#define _SM2_SYSTEM_

#if 0
#ifdef _RSA_SYSTEM_
    #include "polarssl/entropy.h"
    #include "polarssl/ctr_drbg.h"
    #include "polarssl/bignum.h"
    #include "polarssl/x509.h"
    #include "polarssl/rsa.h"
    #include "polarssl/aes.h"
    #include "polarssl/sha256.h"
#else   //_RSA_SYSTEM_
    #include <polarssl/ecp.h>
    #include <polarssl/hmac_drbg.h>
    #include "sm2.h"
    #include "sm3.h"
    #include "sm4.h"
    #include "sm2_util.h"
#endif  //_RSA_SYSTEM_
#endif

#ifdef _RSA_SYSTEM_

    #define RSA_EXPONENT                65537

    #define CLOUD_PUBLIC_KEY_LEN        256
    #define CLOUD_PRIVATE_KEY_LEN       256
    #define DEVICE_PUBLIC_KEY_LEN       128
    #define DEVICE_PRIVATE_KEY_LEN      128

    #define SECRET_DATA_LEN             2048            //私密数据的缓存大小，与存储私密数据的Flash长度相同
    #define RANDOM_ONE_LEN              16              //随机数R1长度
    #define RANDOM_TWO_LEN              16              //随机数R2长度
    #define CLOUD_ASYM_ENCRYPT_LEN      256             //Hello数据的密文长度
    #define DEVICE_ASYM_ENCRYPT_LEN     128             //服务器响应数据的密文长度
    #define HELLO_DATA_FIFO_MAX         2               //Hello预计算数据存储队列大小
    #define SECRET_RESERVE_LEN          96

    #define HASH_DIGEST_LEN             32              //SHA-256
    #define SYM_KEY_LEN                 32              //AES-256算法的密钥长度
    #define SYM_BLOCK_LEN               16              //AES-256明文块长度

    #define RECOVER_SAP_TIMES           5               //SAP恢复时连接TFP2的次数
    #define LICENSE_SERIAL_LEN          15              //License序列号长度，可能是MAC，也可能是IMEI	//Done! 20160420 
    #define LICENSE_MAC_LEN             6               //MAC长度
    #define LICENSE_RANDOM_LEN          32              //随机数长度
    #define LICENSE_LEN                 256             //License长度
    #define LICENSE_INIT_KEY_LEN        32              //初始化密钥长度
    #define LICENSE_KEY_LEN             32              //License保护密钥长度
    #define LICENSE_VERIFY_LEN          32              //License验证结果长度
    #define LAN_KEY_LEN                 32              //局域网密钥长度
    #define UDP_KEY_ID_LEN              16              //UDP密钥索引AnswerCloud
#else   //_RSA_SYSTEM_

    #define CLOUD_PUBLIC_KEY_LEN        64
    #define CLOUD_PRIVATE_KEY_LEN       32
    #define DEVICE_PUBLIC_KEY_LEN       64
    #define DEVICE_PRIVATE_KEY_LEN      32

    #define SECRET_DATA_LEN             2048            //私密数据的缓存大小，与存储私密数据的Flash长度相同
    #define RANDOM_ONE_LEN              16              //随机数R1长度
    #define RANDOM_TWO_LEN              16              //随机数R2长度
    #define CLOUD_ASYM_ENCRYPT_LEN      128             //Hello数据的密文长度
    #define DEVICE_ASYM_ENCRYPT_LEN     128             //服务器响应数据的密文长度
    #define HELLO_DATA_FIFO_MAX         4               //Hello预计算数据存储队列大小
    #define SECRET_RESERVE_LEN          102

    #define HASH_DIGEST_LEN             32              //SM3
    #define SYM_KEY_LEN                 16              //SM4算法的密钥长度
    #define SYM_BLOCK_LEN               16              //SM4明文块长度

    #define RECOVER_SAP_TIMES           5               //SAP恢复时连接TFP2的次数
    #define LICENSE_SERIAL_LEN          15              //License序列号长度，可能是MAC，也可能是IMEI	//Done! 20160420 
    #define LICENSE_MAC_LEN             6               //MAC长度
    #define LICENSE_RANDOM_LEN          32              //随机数长度
    #define LICENSE_LEN                 64              //License长度
    #define LICENSE_INIT_KEY_LEN        32              //初始化密钥长度
    #define LICENSE_KEY_LEN             32              //License保护密钥长度
    #define LICENSE_VERIFY_LEN          32              //License验证结果长度
    #define LAN_KEY_LEN                 16              //局域网密钥长度
    #define UDP_KEY_ID_LEN              16              //UDP密钥索引AnswerCloud

#endif  //_RSA_SYSTEM_

#define DEVICE_SERIAL_NUMBER_LEN        32

#define CLOUD_PUBLIC_KEY_POS    0x00                    //云端公钥在公钥数组中的位置
#define TFP2_PUBLIC_KEY_POS     0x01                    //TFP2公钥在公钥数据中的位置
#define PUBLIC_KEY_NUM          0x02                    //SST使用到的公钥的数量

#define SST_VERSION_SM2         0x10
#define SST_VERSION_SM2_MAC     0x10					//Done! 20160420 
#define SST_VERSION_SM2_IMEI    0x11					//Done! 20160420 
#define SST_VERSION_RSA         0x20
#define SST_VERSION_RSA_MAC     0x20					//Done! 20160420 
#define SST_VERSION_RSA_IMEI    0x21					//Done! 20160420 
#define SST_VERSION_SM4         0x30
#define SST_VERSION_SM4_MAC     0x30					//Done! 20160420 
#define SST_VERSION_SM4_IMEI    0x31					//Done! 20160420 
#define SST_VERSION_AES         0x40
#define SST_VERSION_AES_MAC     0x40					//Done! 20160420 
#define SST_VERSION_AES_IMEI    0x41					//Done! 20160420 

/**
    预计算数据结构体
*/
typedef struct _HelloCloudType_
{
    char Random[RANDOM_ONE_LEN];                        //随机数R1
    char EncryptData[CLOUD_ASYM_ENCRYPT_LEN];           //预计算Hello数据
}HelloCloudType;


/**
    私密数据结构体
*/
typedef struct LicInfo_ 
{
    char Version;										//数据版本 //Done! 20160420 
    char Serial[LICENSE_SERIAL_LEN] ;                   // 序列号，可能是MAC，也可能是IMEI //Done! 20160420 
    char Random[LICENSE_RANDOM_LEN];                    // 256位随机数
    char License[LICENSE_LEN];                          // license		//Done!
    char PrivateKey[DEVICE_PRIVATE_KEY_LEN*2];          // 家电私钥		//Done!
    char PublicKey[DEVICE_PUBLIC_KEY_LEN];				// 家电公钥		//Done!
    char InitKey[LICENSE_INIT_KEY_LEN];                 // 初始会话密钥
//};    
    HelloCloudType \
    HelloData[PUBLIC_KEY_NUM][HELLO_DATA_FIFO_MAX];     //预计算Hello结果
    char LicenseKey[LICENSE_KEY_LEN];	        		// License密钥
    char VerifyResult[LICENSE_VERIFY_LEN];              // Licnese验证结果
    char LanKey[LAN_KEY_LEN];                           //局域网会话密钥
    char AnswerCloud[UDP_KEY_ID_LEN];					//UDP密钥索引
    char Reserve[SECRET_RESERVE_LEN];					//保留字段
    char HashVal[HASH_DIGEST_LEN];						// 结构体的Hash值
} LicenseInfo;

/**
	工装U盾初始化函数
	功能描述：	在终端上插入工装U盾后，调用该函数完成工装U盾的初始化
	输入：		
			inDeviceNameList 全部工装U盾名称列表，以分号隔离
	输出：		
			返回值			初始化成功返回0，失败返回非0
*/
typedef int (__stdcall *InitDevice_)(char *inDeviceNameList);

/**
	工装U盾打开函数
	功能描述：	在进行加解密操作前，调用该函数打开工装U盾
	输入：		
			inReaderName	工装U盾名称，在下拉列表中选择的
			inPassWord		工装U盾口令
	输出：		
			返回值			打开成功返回0，失败返回非0
*/
typedef int (__stdcall *OpenDevice_)(char *inReaderName, char *inPassWord);

/**
	工装U盾口令修改函数
	功能描述：	输入旧口令，同时输入两遍新口令后，完成口令修改，调用该函数前，需要先调用OpenDevice
	输入：		
			inPassWord		工装U盾口令
			inNewPassWord	工装U盾新口令
	输出：		
			返回值			修改成功返回0，失败返回非0
*/
typedef int (__stdcall *ChangePassWord_)(char *inPassWord, char *inNewPassWord);

/**
	生产数据加密密钥解密函数
	功能描述：	在解密生产数据前，先调用该函数解密密钥
	输入：		
			inEncryptKey	生产数据加密密钥的密文
			inEncryptKeyLen	生产数据加密密钥的密文长度
	输出：		
			返回值			解密成功返回0，失败返回非0
*/
typedef int (__stdcall *ParseKey_)(char* inEncryptKey, int inEncryptKeyLen);

/**
	生产数据解密函数
	功能描述：	在向家电写入数据前，先调用该函数解密生产数据
	输入：		
			inLic			生产数据密文
			inLicLen		生产数据密文长度
			inCloudKey		云端公钥
	输出：	
			outLicInfo		生产数据明文
			outLicInfoLen	生产数据明文长度
			outOtpInfo		OTP数据明文
			outOtpInfoLen	OTP数据明文长度
			返回值			解密成功返回0，失败返回非0
*/
typedef int (__stdcall *DecryptLic_)(char* inLic, int inLicLen, UCHAR inCloudKey[][CLOUD_PUBLIC_KEY_LEN], LicenseInfo* outLicInfo, int *outLicInfoLen, char* outOtpInfo, int *outOtpInfoLen);

/**
	工装U盾关闭函数
	功能描述：	工装对话框退出前，调用该函数关闭工装U盾
	输入：		
			无
	输出：	
			返回值			关闭成功返回0，失败返回非0
*/
typedef int (__stdcall *CloseDevice_)();


#ifdef __cplusplus
}
#endif
