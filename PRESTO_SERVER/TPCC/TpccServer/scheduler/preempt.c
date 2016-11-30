/**
*			Copyright (C) 2008-2015 HPDCS Group
*			http://www.dis.uniroma1.it/~hpdcs
*
* @file preempt.c
* @brief LP preemption management
* @author Alessandro Pellegrini
* @author Francesco Quaglia
* @date March, 2015
*/
#include <timestretch.h>
#include "scheduler.h"

extern void preempt_callback(void);

int tick_count;

int preempt_init() {
	int ret;

	ret = ts_open();
	if(ret == TS_OPEN_ERROR) {
		return 1;
	}

	tick_count = 0;

	return 0;
}

int enable_preemption() {
	int ret;

	if((ret = register_ts_thread()) != TS_REGISTER_OK) {
		return 1;
	}

	if((ret = register_buffer((void*) &standing_tick)) != TS_REGISTER_BUFFER_OK) {
		return 1;
	}
/*
	if(register_callback(preempt_callback) != TS_REGISTER_CALLBACK_OK) {
		return 1;
	}
*/
	return 0;
}


int disable_preemption() {

	if(deregister_ts_thread() != TS_DEREGISTER_OK) {
		return 1;
	}

	return 0;
}
