
/*������ʱ�������ݽṹ�������ӿ�*/
#ifndef __SK_TIMER_H_H__
#define __SK_TIMER_H_H__

#include "ntp_client.h"

#define SK_TIMER_NUM 8 		/*���֧��8����ʱ���ڵ�*/
#define SK_TIMER_INVALID_ID 0XFF	/*��Ч�ڵ��־*/

#define SK_ON	0x1
#define SK_OFF	0x0

#ifndef U8
#define U8 unsigned char
#endif

#ifndef U32
#define U32 unsigned int
#endif

#ifndef BOOL
#define BOOL U8
#endif

/*ϵͳ�б���Ķ�ʱ���ڵ�*/
#pragma pack(1)
typedef struct sk_timer_s
{		
	U8 timer_id;		/*��ʱ��Ψһid������ʱ������APP���ɣ��豸�ᱣ��, 0xFF��ʾ�ö�ʱ����Ч*/
	char timeZone;		/*ʱ��*/
	char timer_action;	/*��ʱ���Ķ���, 0x00��ʾ�ص����أ�0x01 ��ʾ��������*/
	char week_flag; 	/*0~6 bit ��ʾ��һ�������Ƿ������ظ���������λ��ʾ�����ظ�,0x00��ʾִ��һ�ζ�ʱ*/
	U32 time;			/*�����Ч��ʱ�̵�λΪ��*/
}sk_timer_t;

#pragma pack()

extern sk_timer_t gSzTimer[SK_TIMER_NUM + 1];	/*��ʱ���ڵ���*/	// ���һ���ڵ㲻�ã�������ʾ����

struct tm current_daytime;

/*��������ʱ���ڵ���ⲿ��������Ӧ��ģ��ʵ�ּ�����*/
typedef void (*sk_handle_timer_fun)(sk_timer_t *ptimer, void *param, int size, int num);

/*��ʼ����ʱ���ڵ�*/
extern void init_sk_timer(void);

/*�������еĶ�ʱ���ڵ㣬����fun����fun��Ӧ��ģ��ʵ�ּ�����,����ֵΪ�������Ч�ڵ����*/
extern int sk_for_each_handle(sk_handle_timer_fun fun, void *param, int size);

/*��ӻ����timer�ڵ�, ����ֵ��TRUE - ��ӳɹ������򣬲���ʧ��*/
extern BOOL sk_add_timer(sk_timer_t *ptimer);

/*ɾ��timer�ڵ�*/
extern void sk_del_timer(U8 timerId);


void sk_print_timer_list();



#endif

