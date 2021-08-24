
/*插座定时器的数据结构及函数接口*/
#ifndef __SK_TIMER_H_H__
#define __SK_TIMER_H_H__

#include "ntp_client.h"

#define SK_TIMER_NUM 8 		/*最多支持8个定时器节点*/
#define SK_TIMER_INVALID_ID 0XFF	/*无效节点标志*/

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

/*系统中保存的定时器节点*/
#pragma pack(1)
typedef struct sk_timer_s
{		
	U8 timer_id;		/*定时器唯一id，区别定时器，由APP生成，设备会保存, 0xFF表示该定时器无效*/
	char timeZone;		/*时区*/
	char timer_action;	/*定时器的动作, 0x00表示关掉开关，0x01 表示开启开关*/
	char week_flag; 	/*0~6 bit 表示周一到周日是否开启周重复，都不置位表示非周重复,0x00表示执行一次定时*/
	U32 time;			/*最近生效的时刻单位为秒*/
}sk_timer_t;

#pragma pack()

extern sk_timer_t gSzTimer[SK_TIMER_NUM + 1];	/*定时器节点组*/	// 最后一个节点不用，用来表示结束

struct tm current_daytime;

/*处理单个定时器节点的外部函数，由应用模块实现及传入*/
typedef void (*sk_handle_timer_fun)(sk_timer_t *ptimer, void *param, int size, int num);

/*初始化定时器节点*/
extern void init_sk_timer(void);

/*遍历所有的定时器节点，进行fun处理，fun由应用模块实现及传入,返回值为处理的有效节点个数*/
extern int sk_for_each_handle(sk_handle_timer_fun fun, void *param, int size);

/*添加或更新timer节点, 返回值，TRUE - 添加成功，否则，操作失败*/
extern BOOL sk_add_timer(sk_timer_t *ptimer);

/*删除timer节点*/
extern void sk_del_timer(U8 timerId);


void sk_print_timer_list();



#endif

