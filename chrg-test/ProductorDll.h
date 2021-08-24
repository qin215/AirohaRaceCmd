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
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������

#ifdef __cplusplus
extern "C"
{
#endif

class CProductorDllApp : public CWinApp
{
public:
	CProductorDllApp();

// ��д
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

    #define SECRET_DATA_LEN             2048            //˽�����ݵĻ����С����洢˽�����ݵ�Flash������ͬ
    #define RANDOM_ONE_LEN              16              //�����R1����
    #define RANDOM_TWO_LEN              16              //�����R2����
    #define CLOUD_ASYM_ENCRYPT_LEN      256             //Hello���ݵ����ĳ���
    #define DEVICE_ASYM_ENCRYPT_LEN     128             //��������Ӧ���ݵ����ĳ���
    #define HELLO_DATA_FIFO_MAX         2               //HelloԤ�������ݴ洢���д�С
    #define SECRET_RESERVE_LEN          96

    #define HASH_DIGEST_LEN             32              //SHA-256
    #define SYM_KEY_LEN                 32              //AES-256�㷨����Կ����
    #define SYM_BLOCK_LEN               16              //AES-256���Ŀ鳤��

    #define RECOVER_SAP_TIMES           5               //SAP�ָ�ʱ����TFP2�Ĵ���
    #define LICENSE_SERIAL_LEN          15              //License���кų��ȣ�������MAC��Ҳ������IMEI	//Done! 20160420 
    #define LICENSE_MAC_LEN             6               //MAC����
    #define LICENSE_RANDOM_LEN          32              //���������
    #define LICENSE_LEN                 256             //License����
    #define LICENSE_INIT_KEY_LEN        32              //��ʼ����Կ����
    #define LICENSE_KEY_LEN             32              //License������Կ����
    #define LICENSE_VERIFY_LEN          32              //License��֤�������
    #define LAN_KEY_LEN                 32              //��������Կ����
    #define UDP_KEY_ID_LEN              16              //UDP��Կ����AnswerCloud
#else   //_RSA_SYSTEM_

    #define CLOUD_PUBLIC_KEY_LEN        64
    #define CLOUD_PRIVATE_KEY_LEN       32
    #define DEVICE_PUBLIC_KEY_LEN       64
    #define DEVICE_PRIVATE_KEY_LEN      32

    #define SECRET_DATA_LEN             2048            //˽�����ݵĻ����С����洢˽�����ݵ�Flash������ͬ
    #define RANDOM_ONE_LEN              16              //�����R1����
    #define RANDOM_TWO_LEN              16              //�����R2����
    #define CLOUD_ASYM_ENCRYPT_LEN      128             //Hello���ݵ����ĳ���
    #define DEVICE_ASYM_ENCRYPT_LEN     128             //��������Ӧ���ݵ����ĳ���
    #define HELLO_DATA_FIFO_MAX         4               //HelloԤ�������ݴ洢���д�С
    #define SECRET_RESERVE_LEN          102

    #define HASH_DIGEST_LEN             32              //SM3
    #define SYM_KEY_LEN                 16              //SM4�㷨����Կ����
    #define SYM_BLOCK_LEN               16              //SM4���Ŀ鳤��

    #define RECOVER_SAP_TIMES           5               //SAP�ָ�ʱ����TFP2�Ĵ���
    #define LICENSE_SERIAL_LEN          15              //License���кų��ȣ�������MAC��Ҳ������IMEI	//Done! 20160420 
    #define LICENSE_MAC_LEN             6               //MAC����
    #define LICENSE_RANDOM_LEN          32              //���������
    #define LICENSE_LEN                 64              //License����
    #define LICENSE_INIT_KEY_LEN        32              //��ʼ����Կ����
    #define LICENSE_KEY_LEN             32              //License������Կ����
    #define LICENSE_VERIFY_LEN          32              //License��֤�������
    #define LAN_KEY_LEN                 16              //��������Կ����
    #define UDP_KEY_ID_LEN              16              //UDP��Կ����AnswerCloud

#endif  //_RSA_SYSTEM_

#define DEVICE_SERIAL_NUMBER_LEN        32

#define CLOUD_PUBLIC_KEY_POS    0x00                    //�ƶ˹�Կ�ڹ�Կ�����е�λ��
#define TFP2_PUBLIC_KEY_POS     0x01                    //TFP2��Կ�ڹ�Կ�����е�λ��
#define PUBLIC_KEY_NUM          0x02                    //SSTʹ�õ��Ĺ�Կ������

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
    Ԥ�������ݽṹ��
*/
typedef struct _HelloCloudType_
{
    char Random[RANDOM_ONE_LEN];                        //�����R1
    char EncryptData[CLOUD_ASYM_ENCRYPT_LEN];           //Ԥ����Hello����
}HelloCloudType;


/**
    ˽�����ݽṹ��
*/
typedef struct LicInfo_ 
{
    char Version;										//���ݰ汾 //Done! 20160420 
    char Serial[LICENSE_SERIAL_LEN] ;                   // ���кţ�������MAC��Ҳ������IMEI //Done! 20160420 
    char Random[LICENSE_RANDOM_LEN];                    // 256λ�����
    char License[LICENSE_LEN];                          // license		//Done!
    char PrivateKey[DEVICE_PRIVATE_KEY_LEN*2];          // �ҵ�˽Կ		//Done!
    char PublicKey[DEVICE_PUBLIC_KEY_LEN];				// �ҵ繫Կ		//Done!
    char InitKey[LICENSE_INIT_KEY_LEN];                 // ��ʼ�Ự��Կ
//};    
    HelloCloudType \
    HelloData[PUBLIC_KEY_NUM][HELLO_DATA_FIFO_MAX];     //Ԥ����Hello���
    char LicenseKey[LICENSE_KEY_LEN];	        		// License��Կ
    char VerifyResult[LICENSE_VERIFY_LEN];              // Licnese��֤���
    char LanKey[LAN_KEY_LEN];                           //�������Ự��Կ
    char AnswerCloud[UDP_KEY_ID_LEN];					//UDP��Կ����
    char Reserve[SECRET_RESERVE_LEN];					//�����ֶ�
    char HashVal[HASH_DIGEST_LEN];						// �ṹ���Hashֵ
} LicenseInfo;

/**
	��װU�ܳ�ʼ������
	����������	���ն��ϲ��빤װU�ܺ󣬵��øú�����ɹ�װU�ܵĳ�ʼ��
	���룺		
			inDeviceNameList ȫ����װU�������б��ԷֺŸ���
	�����		
			����ֵ			��ʼ���ɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *InitDevice_)(char *inDeviceNameList);

/**
	��װU�ܴ򿪺���
	����������	�ڽ��мӽ��ܲ���ǰ�����øú����򿪹�װU��
	���룺		
			inReaderName	��װU�����ƣ��������б���ѡ���
			inPassWord		��װU�ܿ���
	�����		
			����ֵ			�򿪳ɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *OpenDevice_)(char *inReaderName, char *inPassWord);

/**
	��װU�ܿ����޸ĺ���
	����������	����ɿ��ͬʱ���������¿������ɿ����޸ģ����øú���ǰ����Ҫ�ȵ���OpenDevice
	���룺		
			inPassWord		��װU�ܿ���
			inNewPassWord	��װU���¿���
	�����		
			����ֵ			�޸ĳɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *ChangePassWord_)(char *inPassWord, char *inNewPassWord);

/**
	�������ݼ�����Կ���ܺ���
	����������	�ڽ�����������ǰ���ȵ��øú���������Կ
	���룺		
			inEncryptKey	�������ݼ�����Կ������
			inEncryptKeyLen	�������ݼ�����Կ�����ĳ���
	�����		
			����ֵ			���ܳɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *ParseKey_)(char* inEncryptKey, int inEncryptKeyLen);

/**
	�������ݽ��ܺ���
	����������	����ҵ�д������ǰ���ȵ��øú���������������
	���룺		
			inLic			������������
			inLicLen		�����������ĳ���
			inCloudKey		�ƶ˹�Կ
	�����	
			outLicInfo		������������
			outLicInfoLen	�����������ĳ���
			outOtpInfo		OTP��������
			outOtpInfoLen	OTP�������ĳ���
			����ֵ			���ܳɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *DecryptLic_)(char* inLic, int inLicLen, UCHAR inCloudKey[][CLOUD_PUBLIC_KEY_LEN], LicenseInfo* outLicInfo, int *outLicInfoLen, char* outOtpInfo, int *outOtpInfoLen);

/**
	��װU�ܹرպ���
	����������	��װ�Ի����˳�ǰ�����øú����رչ�װU��
	���룺		
			��
	�����	
			����ֵ			�رճɹ�����0��ʧ�ܷ��ط�0
*/
typedef int (__stdcall *CloseDevice_)();


#ifdef __cplusplus
}
#endif
