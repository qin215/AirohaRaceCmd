/*
 * Copyright (c) 2016 Beijing Secsmarts Technology Co., Ltd. All rights reserved.	
 *
 * File Name 		: ProductorErrorCode.h
 * Introduction		: Error code of productor SDK
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

#ifndef _PRODUCTOR_ERROR_CODE_
#define _PRODUCTOR_ERROR_CODE_

#define SST_SUCCESS				0x00000000
#define SST_LIBRARY_ERROR		SST_SUCCESS - 1
#define SST_MEMORY_EMPTY		SST_SUCCESS - 2
#define SST_COM_INIT_ERROR		SST_SUCCESS - 3
#define SST_COM_UNINIT_ERROR	SST_SUCCESS - 4
#define SST_ENUM_KEY_ERROR		SST_SUCCESS - 5
#define SST_KEY_NAME_ERROR		SST_SUCCESS - 6
#define SST_OPEN_SESSION_ERROR	SST_SUCCESS - 7
#define SST_CLOSE_SESSION_ERROR	SST_SUCCESS - 8
#define SST_LOGIN_ERROR			SST_SUCCESS - 9
#define SST_CHANGE_PASS_ERROR	SST_SUCCESS - 10
#define SST_KEY_PAIR_ERROR		SST_SUCCESS - 11
#define SST_ASYM_INIT_ERROR		SST_SUCCESS - 12
#define SST_ASYM_DECRYPT_ERROR	SST_SUCCESS - 13
#define SST_ASYM_ENCRYPT_ERROR	SST_SUCCESS - 14
#define SST_INPUT_EMPTY_ERROR	SST_SUCCESS - 15
#define SST_RANDOM_INIT_ERROR	SST_SUCCESS - 16
#define SST_ASYM_KEY_ERROR		SST_SUCCESS - 17

#endif //_PRODUCTOR_ERROR_CODE_