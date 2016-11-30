#include <errno.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../lib/tm.h"
#include "../lib/random.h"
#include "manager.h"
#include "stm_threadpool.h"

#ifdef PRINT_PROGRESS
#include "arch/atomic.h"
#endif

#include "arch/ult.h"
#include "datatypes/state.h"
#include "datatypes/task.h"
#include "datatypes/nblist.h"
#include "datatypes/priority.h"

#ifdef TXS_STATS
#include "stats/stats.h"
#endif

manager_t* managerPtr;
random_t* randomPtr;

struct prio_task_array* pta;

extern int tick_count;

#ifdef SCHED_MISS_COUNT
extern int exploration_miss;
#endif

#ifdef PRINT_PROGRESS
double tm;
struct timespec* aux;
struct timespec* t1;
struct timespec* t2;
int txs_print_percentage;
int txs_five_percent;
#endif

int txs_total;
int txs_completed;

#ifdef TXS_STATS
int update_enabled;
int update_printed;
#endif

void worker_runMethod(void* arg) {
	char* delims = ";";
	long prio, id_op, W_ID, D_ID, C_ID, TR, CAR;
	float H_AM;
	char* saveptr = NULL;
	char* token;
	char* c_last = NULL;
	task_t* task;

	while (1) {
		token = NULL;
		task = running_task;

		token = strtok_r(task->args, delims, &saveptr);
		if (token != NULL) {
			prio = strtol(token, NULL, 10);
			token=strtok_r(NULL, delims, &saveptr);
			if (token != NULL) {
				id_op = strtol(token, NULL, 10);
				switch (id_op) {
					case 1:
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						W_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						D_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						C_ID=strtol(token,NULL,10);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->start);
#endif
						//printf("new order transaction: parameters W_ID:%ld,D_ID:%ld,C_ID:%ld ",W_ID,D_ID,C_ID);
						TMMANAGER_NEWORDER(managerPtr, W_ID,  D_ID,  C_ID);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->end);
#endif
						break;
					case 2:
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						W_ID=strtol(token,NULL,10);

						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						D_ID=strtol(token,NULL,10);

						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						C_ID=strtol(token,NULL,10);

						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						c_last =token;

						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						H_AM=strtof(token,NULL);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->start);
#endif
						//printf("payment transaction: parameters W_ID:%ld,D_ID:%ld,C_ID:%ld",W_ID,D_ID,C_ID);
						TMMANAGER_PAYMENT(managerPtr, W_ID,  D_ID,  C_ID, H_AM, c_last);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->end);
#endif
						break;
					case 3:
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						W_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						D_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						C_ID=strtol(token,NULL,10);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->start);
#endif
						//printf("orderstatus transaction: parameters W_ID:%ld,D_ID:%ld,C_ID:%ld",W_ID,D_ID,C_ID);
						TMMANAGER_ORDSTATUS(managerPtr, W_ID,  D_ID,  C_ID);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->end);
#endif
						break;
					case 4:
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						W_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						D_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						TR=strtol(token,NULL,10);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->start);
#endif
						//printf("STOCKLEVEL transaction parameters W_ID:%ld,D_ID:%ld,TR:%ld",W_ID,D_ID,TR);
						TMMANAGER_STOCKLEVEL(managerPtr, W_ID,  D_ID,  TR);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->end);
#endif
						break;
					case 5:
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						W_ID=strtol(token,NULL,10);
						token=strtok_r(NULL, delims, &saveptr);
						if(token==NULL){
							break;
						}
						CAR=strtol(token,NULL,10);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->start);
#endif
						//printf("DELIVERY transaction parameters W_ID:%ld,CAR:%ld",W_ID,CAR);
						TMMANAGER_DELIVERY(managerPtr, W_ID,CAR);
#ifdef TXS_STATS
						clock_gettime(CLOCK_MONOTONIC_RAW, &task->end);
#endif
						break;
					default:
						break;
				}
			}
		}
#ifdef PRINT_PROGRESS
		atomic_inc(&txs_completed);
#endif

#ifdef SINGLE_POOL
		context_switch(&task->state.context, &platform_context);
#else
		context_switch(&task->state->context, &platform_context);
#endif
	}
}

void server_runMethod(struct prio_task_array* p, server_thread_t* thread) {
	char recvbuf[256];
	int iResult = 0;

	long prio, txid;
	char* delims = ";";
	char* saveptr = NULL;
	char* token = NULL;

	task_t* task = NULL;

	if (p == NULL || thread->conn < 0) {
		puts("Invalid arguments\n");
		return;
	}

	iResult = recv(thread->conn, recvbuf, 256, MSG_WAITALL);

	if (iResult < 0) {
		printf("ERROR: \"recv\" function failed with error message \"%s\".\n", strerror(errno));
		return;
	}

#ifdef SINGLE_POOL
	while ((task = GetTask()) == NULL)
		gc_clean(thread);
#else
	while ((task = GetTask()) == NULL) ;
#endif

#ifdef SINGLE_POOL
	gc_insert(thread, task);
#endif

	strncpy(task->args, recvbuf, sizeof(task->args));

	token = strtok_r(recvbuf, delims, &saveptr);
	if (token == NULL) {
		printf("ERROR: \"recv\" function did not receive Priority argument.\n");
#ifdef SINGLE_POOL
		task->free_gc = 1;
#else
		FreeTask(task);
#endif
		return;
	}
	prio = strtol(token, NULL, 10);
	prio = (prio < 0) ? 0 : ((prio >= p->num_prio) ? p->num_prio-1 : prio);
	task->prio = (int) prio;

	token = strtok_r(NULL, delims, &saveptr);
	if (token == NULL) {
		printf("ERROR: \"recv\" function did not receive TX-ID argument.\n");
#ifdef SINGLE_POOL
		task->free_gc = 1;
#else
		FreeTask(task);
#endif
		return;
	}
	txid = strtol(token, NULL, 10);
	txid = (txid < 1) ? 1 : ((txid > 5) ? 5 : txid);
	task->txid = (int) txid;

#ifdef TXS_STATS
#ifndef DROP_CONT_TIME
	clock_gettime(CLOCK_MONOTONIC_RAW, &task->enqueue);
#endif
#endif

	if (InsertNewTask(p, task)) {
		puts("Unable to insert task\n");
#ifdef SINGLE_POOL
		task->free_gc = 1;
#else
		FreeTask(task);
#endif
	}
}

int startupPoolMemory(int num_workers, int num_servers, int num_prio,
		worker_threadpool_t** worker_threadpool, server_threadpool_t** server_threadpool, int pool_size) {

	TM_STARTUP(num_workers);

#ifdef TXS_STATS
	if (StatsInit(num_prio))
		goto error0;
#endif

#ifndef SINGLE_POOL
	if (StatePoolInit(num_workers*num_prio) == -1)
		goto error1;
#endif

	if (TaskPoolInit(pool_size) == -1)
		goto error2;

	if ((pta = GetPrioTaskArray(num_prio)) == NULL)
		goto error3;

	if (((*worker_threadpool) = worker_threadpool_create(pta, num_workers)) == NULL)
		goto error4;

	if (((*server_threadpool) = server_threadpool_create(pta, num_servers)) == NULL)
		goto error5;

	return 0;

error5:
	worker_threadpool_destroy(*worker_threadpool);
error4:
	FreePrioTaskArray(&pta);
error3:
	TaskPoolDestroy();
error2:
#ifndef SINGLE_POOL
	StatePoolDestroy();
#endif
error1:
#ifdef TXS_STATS
	StatsFini();
#endif
error0:
	TM_SHUTDOWN();
	return -1;
}

void shutdownPoolMemory(worker_threadpool_t* worker_threadpool, server_threadpool_t* server_threadpool) {

	server_threadpool_destroy(server_threadpool);
	printf("server_threadpool destroyed!\n");

	worker_threadpool_destroy(worker_threadpool);
	printf("worker_threadpool destroyed!\n");

	FreePrioTaskArray(&pta);
	printf("pta freed!\n");

	TaskPoolDestroy();
	printf("taskpool freed!\n");

#ifndef SINGLE_POOL
	StatePoolDestroy();
#endif

#ifdef TXS_STATS
	StatsFini();
	printf("stats freed!\n");
#endif

	TM_SHUTDOWN();
}

int main(int argc, char* argv[]) {
	int n;
	int cont;
	int err;
	long res;

	int sockfd;
	int connaddr;
	int sa_len;
	int option = 1;

	int serv_port;
	int pool_size;
	int num_servers;
	int num_workers;
	int txs_x_thread;
	int num_prio;

	struct sockaddr_in addrin;
	struct sockaddr *Cli;

	const char* delims = ";";
	char* token = NULL;

	char sendbuf[20];
	char recvbuf[256];

	server_threadpool_t* server_threadpool;
	worker_threadpool_t* worker_threadpool;

	if (argc < 4) {
		fprintf(stderr,"[Error] Usage: %s server_port pool_size num_workers\n", argv[0]);
		exit(0);
	}

	serv_port = atoi(argv[1]);
	pool_size = atoi(argv[2]);
	num_workers = atoi(argv[3]);

	printf("Manager initialization...\n");
	managerPtr = manager_allocation();
	printf("complete!\n");

	bzero((char*) &Cli, sizeof(Cli));
	bzero((char*) &addrin, sizeof(addrin));
	bzero((char*) &recvbuf, sizeof(recvbuf));

	addrin.sin_family = AF_INET;
	addrin.sin_addr.s_addr = htonl(INADDR_ANY);
	addrin.sin_port = htons(serv_port);

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr,"[Error] Socket call failed.\n");
		exit(0);
	}
	if (setsockopt(sockfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), &option, sizeof(option)) < 0) {
		fprintf(stderr,"[Error] Setsockopt failed.\n");
		close(sockfd);
		exit(0);
	}
	if (bind(sockfd, (struct sockaddr*) &addrin, sizeof(addrin)) == -1) {
		fprintf(stderr,"[Error] Bind to port number %d failed.\n", serv_port);
		close(sockfd);
		exit(0);
	}
	if (listen(sockfd, 1000) == -1) {
		fprintf(stderr,"[Error] Listen failed.\n");
		close(sockfd);
		exit(0);
	}

	do {

		sa_len = sizeof(Cli);

		if ((connaddr = accept(sockfd, (struct sockaddr*) &Cli, &sa_len)) == -1) {
			fprintf(stderr,"[Error] Accept failed.\n");
			close(sockfd);
			exit(0);
		}

		memset(recvbuf, 0, sizeof(recvbuf));

		if ((n = read(connaddr,recvbuf, sizeof(recvbuf))) < 0) {
			fprintf(stderr,"[Error] Unable to read.\n");
			close(connaddr);
			close(sockfd);
			exit(0);
		}
		printf("Handshake: %s\n", recvbuf);

		token = strtok(recvbuf, delims);
		num_servers = strtol(token, NULL, 10);

		if (num_servers <= 0) {
			fprintf(stderr,"[Error] The number of server (numbero of priorities) must be greater than zero.\n");
			close(connaddr);
			close(sockfd);
			exit(0);
		}

		num_prio = num_servers;

		token = strtok(NULL, delims);
		txs_x_thread = strtol(token, NULL, 10);

		if (txs_x_thread <= 0) {
			fprintf(stderr,"[Error] The number transactions send by the clients must be greater than zero.\n");
			close(connaddr);
			close(sockfd);
			exit(0);
		}

		txs_total = 0;
		txs_completed = 0;

#ifdef PRINT_PROGRESS
		if ((t1 = (struct timespec*) malloc(sizeof(struct timespec))) == NULL) {
			fprintf(stderr,"[Error] Unable to allocate memory.\n");
			close(connaddr);
			close(sockfd);
			exit(0);
		}
		if ((t2 = (struct timespec*) malloc(sizeof(struct timespec))) == NULL) {
			fprintf(stderr,"[Error] Unable to allocate memory.\n");
			free(t1);
			close(connaddr);
			close(sockfd);
			exit(0);
		}
		txs_total = num_servers * txs_x_thread;
		txs_print_percentage = 0;
		txs_five_percent = (int) (((double ) txs_total / 100.0) * 5.0);
		txs_five_percent = (txs_five_percent > 5000) ? 5000 : txs_five_percent;
#endif

#ifdef SCHED_MISS_COUNT
		exploration_miss = 0;
#endif

		printf("TM START...");
		if (startupPoolMemory(num_workers, num_servers, num_prio, &worker_threadpool, &server_threadpool, pool_size) == -1) {
			fprintf(stderr,"[Error] Unable to initialize pool memory.\n");
			close(connaddr);
			close(sockfd);
			exit(0);
		}
		printf("complete!\n");

		send(connaddr, "OK", sizeof("OK"), 0);
		close(connaddr);

		printf("Start main cycle...\n");
		for (cont=0; cont<num_servers; cont++) {
			connaddr = accept(sockfd, &Cli, &sa_len);

			err = server_threadpool_add(server_threadpool, connaddr, txs_x_thread);

			if (err < 0) {
				if (err == threadpool_invalid)
					puts("Invalid argument to threadpool");
				else if (err == threadpool_shutdown)
					puts("Error threadpool in shutdown");
				else if (err == threadpool_thread_failure)
					puts("Error generic failure");
				else
					puts("Error unknown");

				bzero((char*) sendbuf, sizeof(sendbuf));
				sprintf(sendbuf, "%ld", -17);
				send(connaddr, sendbuf, sizeof(sendbuf), 0);
				close(connaddr);
			}
		}

		server_threadpool->can_start = 1;

#ifdef SCHED_MISS_COUNT
		worker_threadpool->can_increment = 1;
#endif

#ifdef TXS_STATS
		update_enabled = 0;
		update_printed = 0;
#endif

#ifdef PRINT_PROGRESS
		clock_gettime(CLOCK_MONOTONIC_RAW, t1);
		txs_print_percentage += txs_five_percent;
#endif
		while (txs_completed < txs_total) {
#ifdef PRINT_PROGRESS
			if (txs_completed >= txs_print_percentage) {
				clock_gettime(CLOCK_MONOTONIC_RAW, t2);
				tm = ((double) (t2->tv_sec - t1->tv_sec))*1000000000;
				tm += ((double) (t2->tv_nsec - t1->tv_nsec));
				tm /= 1000.0;
				aux = t2;
				t2 = t1;
				t1 = aux;
				printf("TXS Completed/Total: %8d/%8d - Time: %12.2f - Free-Pool: %d\n", txs_completed, txs_total, tm, GetNumFreeTasks());
				txs_print_percentage += txs_five_percent;
			}
#endif
#ifdef TXS_STATS
			if (!update_enabled) {
				if (txs_completed >= 100000) {
					EnableUpdate();
					update_enabled = 1;
				}
			} else if (!update_printed) {
				if (!IsEnableUpdate()) {
					StatsPrint();
					update_printed = 1;
				}
			}
#endif
		}
#ifdef PRINT_PROGRESS
		printf("Transactions Completed/Total: %d/%d\n", txs_total, txs_total);
#endif

#ifdef SCHED_MISS_COUNT
		worker_threadpool->can_increment = 0;
#endif

#ifdef TXS_STATS
//		StatsPrint();
#endif

		printf("TM SHUTDOWN...\n");
		shutdownPoolMemory(worker_threadpool, server_threadpool);
		printf("complete!\n");

		printf("\n");
		printf("Total number of tick arrived:\t\t%d\n", tick_count);
#ifdef SCHED_MISS_COUNT
		printf("Number of times No Higher-Priority TXs are found by the Scheduled:\t%d\n", exploration_miss);
#endif
		printf("\n");

	} while (0);

	close(sockfd);
	printf("Halt!!!\n");

	return 0;
}
