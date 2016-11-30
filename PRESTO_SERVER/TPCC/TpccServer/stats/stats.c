#ifdef TXS_STATS

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "stats.h"

#define NUM_TXs		5

static int						priority;

static pthread_spinlock_t*		stats_lock;

static int*						count;
static double*					mean_wait;
static double*					mean_exec;
static int*						num_aborts;
static int*						num_commits;

static int						can_update;

int StatsInit(int prio) {
	int i, j;

	if (prio <= 0)
		return 1;

	priority = prio+1;

	if ((count = (int*) malloc(priority * sizeof(int))) == NULL)
		return 1;

	if ((mean_wait = (double*) malloc(priority * sizeof(double))) == NULL)
		return 1;

	if ((mean_exec = (double*) malloc(priority * sizeof(double))) == NULL)
		return 1;

	if ((num_aborts = (int*) malloc(priority * sizeof(int))) == NULL)
		return 1;

	if ((num_commits = (int*) malloc(priority * sizeof(int))) == NULL)
		return 1;

	if ((stats_lock = (pthread_spinlock_t*) malloc(priority * sizeof(pthread_spinlock_t))) == NULL)
		return 1;

	for (i=0; i<priority-1; i++) {
		count[i] = 0;
		mean_wait[i] = 0.0;
		mean_exec[i] = 0.0;
		num_aborts[i] = 0;
		num_commits[i] = 0;
		pthread_spin_init(&stats_lock[i], PTHREAD_PROCESS_PRIVATE);
	}

	can_update = 0;
	return 0;
}

void StatsFini() {
	
}

void StatsUpdate(task_t* task) {
	int p;
	double exec, wait;

	if (!can_update)
		return;

	p = task->prio;

#ifdef DROP_CONT_TIME
	wait = ((double) (task->before_start.tv_sec - task->enqueue.tv_sec))*1000000000;
	wait += ((double) (task->before_start.tv_nsec - task->enqueue.tv_nsec));
	wait /= 1000.0;		/* Microseconds */
#else
	wait = ((double) (task->start.tv_sec - task->enqueue.tv_sec))*1000000000;
	wait += ((double) (task->start.tv_nsec - task->enqueue.tv_nsec));
	wait /= 1000.0;		/* Microseconds */
#endif
	exec = ((double) (task->end.tv_sec - task->start.tv_sec))*1000000000;
	exec += ((double) (task->end.tv_nsec - task->start.tv_nsec));
	exec /= 1000.0;		/* Microseconds */

	pthread_spin_lock(&stats_lock[p]);

	num_aborts[p] += task->aborts;
	num_commits[p] += task->commits;

	count[p] += 1;
	mean_wait[p] = mean_wait[p] + ((wait - mean_wait[p]) / (double) count[p]);
	mean_exec[p] = mean_exec[p] + ((exec - mean_exec[p]) / (double) count[p]);

	pthread_spin_unlock(&stats_lock[p]);
}

void StatsPrint() {
	int i, j;

	for (i=0; i<priority-1; i++) {
		pthread_spin_lock(&stats_lock[i]);
	}

	FILE* file = fopen("results.tsv","w");
	fprintf(file, "prio\tcount\tmean(wait)\tmean(exec)\tmean(turn)\tnum(aborts)\tnum(commits)\tprob(abort)\n");
	for (i=0; i<priority-1; i++) {
		/*
		 * priority  count  mean(serv)  mean(exec)  mean(turn)  num(aborts)  num(commits)  prob(abort)
		 */
		fprintf(file, "%d\t%d\t%f\t%f\t%f\t%d\t%d\t%f\n", i, count[i], mean_wait[i], mean_exec[i], (mean_wait[i]+mean_exec[i]),
					num_aborts[i], num_commits[i], ((double) num_aborts[i] / ((double) num_aborts[i] + (double) num_commits[i])));
	}
	fflush(file);
	fclose(file);

	for (i=0; i<priority-1; i++) {
		pthread_spin_unlock(&stats_lock[i]);
	}
}

void EnableUpdate() {
	can_update = 1;
}

void DisableUpdate() {
	can_update = 0;
}

int IsEnableUpdate() {
	return can_update;
}

#endif
