/*
author: zhangwei
*/


#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <stdbool.h>
#include <stdlib.h>


#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC 
#endif  


//定时器间隔
typedef struct timer_interval_t
{
	unsigned char hms;			// 100ms
	unsigned char year;			// year
	unsigned char month;		// month
	unsigned char day;			// day
	unsigned char hour;			// hour
	unsigned char min;			// min
	unsigned char second;		// second
}timer_interval_t;

//定时器节点
typedef struct timer_node_
{
	timer_interval_t		solt;
	size_t                  interval;
	struct timer_node_   *  next;
	struct timer_node_   *  prev;
	void *			 param;
}*timer_node_t;

//定时器超时处理回调
typedef void(*timer_handler_t)(timer_node_t );
 
typedef struct tw_timer_ * tw_timer_t;


EXTERNC tw_timer_t         tw_timer_create(timer_handler_t handler);

EXTERNC void               tw_timer_run(tw_timer_t timer);

EXTERNC void               tw_timer_stop(tw_timer_t timer);

EXTERNC bool               tw_timer_add(tw_timer_t timer, timer_node_t node);

EXTERNC void               tw_timer_destory(tw_timer_t timer);

//print
#endif

