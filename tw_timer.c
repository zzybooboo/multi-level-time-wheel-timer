#include "types.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

const unsigned char solt_counts[TIME_WHELL_SIZE] = { 
	TIMER_HMS_SOLT_COUNT,
	TIMER_SECOND_SOLT_COUNT ,
	TIMER_MIN_SOLT_COUNT,
	TIMER_HOUR_SOLT_COUNT,
	TIMER_DAY_SOLT_COUNT,
	TIMER_MONTH_SOLT_COUNT,
	TIMER_YEAR_SOLT_COUNT	
};

const size_t sc[TIME_WHELL_SIZE] = {
	1,
	UNIT_SECOND,
	UNIT_MIN,
	UNIT_HOUR,
	UNIT_DAY,
	UNIT_MONTH,
	UNIT_YEAR
};

//清理时间轮 槽里面的链表
static inline void _timer_solt_clean(timer_solt_t  solt)
{
	if (!solt->nodes)
		return;

	timer_node_t node = solt->nodes;
	while (node)
	{
		free(node);
		node = node->next;
	}
}

//清理时间轮
static inline void _timer_wheel_clean(tw_timer_t timer)
{
	int i = 0;
	for (; i < TIME_WHELL_SIZE; i++)
	{
		if (!timer->wheels[i].solts)
			continue;

		int j = 0;
		for (; j < timer->wheels[i].slot_count; j++)
		{
			_timer_solt_clean(&timer->wheels[i].solts[j]);

		}
		//释放槽
		free(timer->wheels[i].solts);
		timer->wheels[i].slot_count = 0;
	}
}

//初始化槽
static inline bool _timer_wheel_init(tw_timer_t timer)
{
	int level = 0;
	for (; level < TIME_WHELL_SIZE; level++)
	{
		timer->wheels[level].solts = (timer_solt_t)calloc(solt_counts[level], sizeof(struct timer_solt_));
		if (!timer->wheels[level].solts)
			return false;

		timer->wheels[level].cur_solt = 0;
		timer->wheels[level].slot_count = solt_counts[level];
	}
	return true;
}

//创建timer
tw_timer_t tw_timer_create(timer_handler_t handler)
{
	if (!handler)
		return NULL;
	
	//申请内存
	tw_timer_t timer = (tw_timer_t)calloc(1, sizeof(struct tw_timer_));
	if (!timer)
		return NULL;
	
	//设置handler
	timer->handler = handler;
	
	//初始化时间轮j
	if (!_timer_wheel_init(timer))
	{
		tw_timer_destory(timer);
		return NULL;
	}
	return timer;
}

//插入定时器
static inline void _timer_node_insert(tw_timer_t timer, time_wheel_t dst_wheel,timer_node_t node, const int level)
{
	const char indexs[] = {
		node->solt.hms,
		node->solt.second,
		node->solt.min,
		node->solt.hour,
		node->solt.day,
		node->solt.month,
		node->solt.year
	};

	
	node->prev = NULL;
	int index = indexs[level];
	node->next = dst_wheel->solts[index].nodes;
	if (dst_wheel->solts[index].nodes)
		dst_wheel->solts[index].nodes->prev = node;

	dst_wheel->solts[index].nodes = node;
}

//逐级检测
static inline void _timer_check(tw_timer_t timer)
{
	int level = TIME_WHELL_SIZE - 1;
	for (; level >= 1; level--)
	{
		if (!(timer->tick >= sc[level]))
			continue;

		time_wheel_t wheel = &timer->wheels[level];
		time_wheel_t prev_wheel = &timer->wheels[level - 1]; 
 
		timer_node_t  node = wheel->solts[wheel->cur_solt].nodes;
		//printf("check level:%d, cur_solt:%d\n", level, wheel->cur_solt);
		wheel->solts[wheel->cur_solt].nodes = NULL;
		
		while (node)
		{
			timer_node_t  cur = node;
			//删除
			node = node->next;
			if (node)
				node->prev = NULL;

			//插入
			_timer_node_insert(timer, prev_wheel, cur, level - 1);
		}
	}
}

//推动轮
static inline void _timer_run_wheel(tw_timer_t timer, const int level, 
	run_next_level_func_t next_level_func)
{
	if (level <= 0)
		return;

	time_wheel_t wheel = &timer->wheels[level];
	wheel->cur_solt++;
	if (wheel->cur_solt >= solt_counts[level])
	{
		//推动下一级
		wheel->cur_solt = 0;
		if (next_level_func)
			(*next_level_func)(timer);
	}
}

//推动年轮
static inline void _timer_run_year_wheel(tw_timer_t timer)
{
	_timer_run_wheel(timer, 6, NULL);
}

//推动月轮
static inline void _timer_run_month_wheel(tw_timer_t timer)
{
	_timer_run_wheel(timer, 5, _timer_run_year_wheel);
}

//推动天轮
static inline void _timer_run_day_wheel(tw_timer_t timer)
{
	
	_timer_run_wheel(timer, 4, _timer_run_month_wheel);
}

//推动时轮
static inline void _timer_run_hour_wheel(tw_timer_t timer)
{
	_timer_run_wheel(timer, 3, _timer_run_day_wheel);
}

//推动分轮
static inline void _timer_run_min_wheel(tw_timer_t timer)
{
	_timer_run_wheel(timer, 2, _timer_run_hour_wheel);
}

//推动秒轮
static inline void _timer_run_second_wheel(tw_timer_t timer)
{
	_timer_run_wheel(timer, 1, _timer_run_min_wheel);
}

//计算timer 的间隔
static inline size_t _timer_calculate_interval(timer_interval_t * tv)
{ 
	const size_t t[] = { 
		tv->hms,                         //0-9
		tv->second * UNIT_SECOND,        //0-60
		tv->min * UNIT_MIN,				 //0-60
		tv->hour * UNIT_HOUR,			 //0-24
		tv->day * UNIT_DAY,			     //0-30	
		tv->month * UNIT_MONTH,          //0-12
		tv->year * UNIT_YEAR             //0-5
	};
	
	int i = 0; 
	size_t solt = 0;
	for (; i < (sizeof(t)/sizeof(t[0])); i++)
	{
		solt += t[i];
	}
	return solt;
}

//将hms当前槽的定时器 重新计算下个周期的槽，并插入到目标槽
static inline size_t _timer_move(tw_timer_t timer,time_wheel_t hms_wheel)
{
	//printf("_timer_move,cur_solt:%d\n", hms_wheel->cur_solt);
	//获取当前hms的solt
	timer_solt_t solt = &hms_wheel->solts[hms_wheel->cur_solt -1];
	timer_node_t node = solt->nodes;
	solt->nodes = NULL;

	while (node)
	{
		timer_node_t cur_node = node;
		node = node->next;

		tw_timer_add(timer, cur_node);
	}
}

//推动0.1秒的轮
static inline void _timer_run_hms_wheel(tw_timer_t timer)
{
	int i = 0;
	_timer_check(timer);
	for (; i < TIMER_HMS_SOLT_COUNT && timer->run; i++)
	{
		timer->wheels[0].cur_solt++;
		if (timer->wheels[0].solts[i].nodes)
		{
			//调用回调
			//printf("cur tick:%d\n", timer->tick);
			(*timer->handler)(timer->wheels[0].solts[i].nodes);
			//计算下一个执行周期的槽
			_timer_move(timer, &timer->wheels[0]);
		}
		usleep(GRANULARITY);
		timer->tick++;
	}
	timer->wheels[0].cur_solt = 0;
	//推动秒轮
	_timer_run_second_wheel(timer);
}

//运行timer
void  tw_timer_run(tw_timer_t timer)
{
	if (!timer || timer->run)
		return;

	timer->run = true;
	while (timer->run)
	{
		_timer_run_hms_wheel(timer);
	}
}

//停止timer
void  tw_timer_stop(tw_timer_t timer)
{
	if (!timer || !timer->run)
		return;

	timer->run = false;
}

//将间隔数字转换成 interval 结构
static inline void _timer_trans_interval(size_t interval,timer_interval_t * tv)
{
	memset(tv, 0, sizeof(timer_interval_t));
	unsigned char *  t[] ={
		&tv->hms,
		&tv->second,
		&tv->min,
		&tv->hour,
		&tv->day,
		&tv->month,
		&tv->year
	};

	//size_t left = interval;
	int level = (sizeof(t) / sizeof(t[0])) - 1;
	for (; level >= 0; level--)
	{
		*t[level] = (unsigned char)(interval / sc[level]);
		interval = interval % sc[level];
	}
}

//添加timer
bool  tw_timer_add(tw_timer_t timer, timer_node_t node)
{
	if (!timer)
		return false;
	
	size_t dst_solt = timer->tick + node->interval;
	_timer_trans_interval(dst_solt, &node->solt);
	
	const unsigned char t[] = {
		node->solt.hms,
		node->solt.second,
		node->solt.min,
		node->solt.hour,
		node->solt.day,
		node->solt.month,
		node->solt.year
	};

	int level = (sizeof(t) / sizeof(t[0])) - 1;
	for (; level >= 0; level--)
	{
		if (t[level] == 0)
			continue;

		_timer_node_insert(timer, &timer->wheels[level], node, level);
		break;
	}

	return true;
}

//销毁timer
void   tw_timer_destory(tw_timer_t timer)
{
	if (!timer)
		return;

	tw_timer_stop(timer);
	_timer_wheel_clean(timer);
	free(timer);
}