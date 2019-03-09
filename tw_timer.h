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


//定时器超时处理回调
typedef void(*timer_handler_t)(void *);

typedef struct timer_node_* timer_node_t;
typedef struct tw_timer_ *  tw_timer_t;


EXTERNC timer_node_t       tw_timer_node_create(size_t interval, void * arg);

EXTERNC void		       tw_timer_node_free(timer_node_t node);

EXTERNC tw_timer_t         tw_timer_create(timer_handler_t handler);

EXTERNC void               tw_timer_run(tw_timer_t timer);

EXTERNC void               tw_timer_stop(tw_timer_t timer);

EXTERNC bool	           tw_timer_add(tw_timer_t timer, timer_node_t node);

EXTERNC void               tw_timer_delete(tw_timer_t timer, timer_node_t node);

EXTERNC void               tw_timer_destory(tw_timer_t timer);


//EXTERNC void               tw_timer_destory(tw_timer_t timer);
//print
#endif

