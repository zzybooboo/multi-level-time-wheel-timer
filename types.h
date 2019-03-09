#ifndef TIME_WHEEL_TIMER_TYPES
#define TIME_WHEEL_TIMER_TYPES

#include "list.h"
#include <stdlib.h>
#include <stdbool.h>
#include "tw_timer.h"

#define GRANULARITY			    100000      //0.1 second
#define TIME_WHELL_SIZE         7			//level

#define TIMER_HMS_SOLT_COUNT    10
#define TIMER_SECOND_SOLT_COUNT 60
#define TIMER_MIN_SOLT_COUNT	60
#define TIMER_HOUR_SOLT_COUNT	24
#define TIMER_DAY_SOLT_COUNT	30
#define TIMER_MONTH_SOLT_COUNT	12
#define TIMER_YEAR_SOLT_COUNT	6


#define UNIT_SECOND 10
#define UNIT_MIN    60 * UNIT_SECOND
#define UNIT_HOUR   60 * UNIT_MIN
#define UNIT_DAY    24 * UNIT_HOUR
#define UNIT_MONTH  30 * UNIT_DAY
#define UNIT_YEAR   12 * UNIT_MONTH

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
	struct list_head        node;
	timer_interval_t		solt;
	size_t                  interval;
	void *					param;
};

//时间轮的槽
typedef struct timer_solt_
{
	struct list_head   root;
}*timer_solt_t;

//时间轮
typedef struct time_wheel_
{
	timer_solt_t        solts;
	unsigned char		cur_tick;
	unsigned short		slot_count;
}*time_wheel_t;

//定时器
typedef struct tw_timer_
{
	bool                  run;
	size_t                tick;
	timer_handler_t       handler;
	struct time_wheel_    wheels[TIME_WHELL_SIZE];
};

typedef void(*run_next_level_func_t)(tw_timer_t);
 
#endif