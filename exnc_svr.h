#ifndef __EXNC_SVR_H_
#define __EXNC_SVR_H_

#include "log.h"
#include "timer.h"
#include "lockqueue.h"
#include "file_event.h"
#include "string.h"
#include "tcp.h"

typedef struct stCmd {
	const char *name;
	void (*func)(char *argv[], int argc);
	const char *desc;
}stCmd_t;

typedef struct stCli {
	int fd;
	char mac[24];
	char ip[24];
} stCli_t;

typedef struct stExncSvrEnv {
	struct timer_head *th;
	struct file_event_table *fet;

	struct timer tr;
	stLockQueue_t eq;

	int svr_fd;
	stCli_t clis[128];

	int sel_fd;
}stExncSvrEnv_t;


int exnc_svr_init(void *_th, void *_fet);

void exnc_svr_in(void *arg, int fd);
void exnc_svr_cmd_in(void *arg, int fd);
void exnc_cli_in(void *arg, int fd);

void exnc_svr_run(struct timer *timer);

int exnc_svr_push();
int exnc_svr_step();

int exnc_svr_ctrl_c();
#endif
