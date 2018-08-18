#include "web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "jansson.h"
#include "json_parser.h"

static stWebEnv_t we = {
	.ip		= "192.168.10.6",
	.port = 18080,
	.base	= "./www",
	.svr	= NULL,
	.user = "admin",
	.pass = "admin",
	//.currpage = 3,
	.th = NULL,
	.fet = NULL,

};


static void center_func(httpd *server, httpReq *req);

static stWebNode_t wns[] = {
	{PT_WILD, "/image",		INVALID_ITEM,	INVALID_ITEM,	NULL, INVALID_ITEM,		"image",			INVALID_ITEM},

	{PT_WILD, "/css",			INVALID_ITEM,		INVALID_ITEM,	NULL, INVALID_ITEM,		"css",				INVALID_ITEM},
	{PT_WILD, "/js",			INVALID_ITEM,		INVALID_ITEM,	NULL, INVALID_ITEM,		"js",					INVALID_ITEM},
	{PT_WILD, "/site",		INVALID_ITEM,		INVALID_ITEM,	NULL, INVALID_ITEM,		"site",				INVALID_ITEM},
	{PT_WILD, "/font",		INVALID_ITEM,		INVALID_ITEM,	NULL, INVALID_ITEM,		"font",				INVALID_ITEM},

	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"dl",					INVALID_ITEM},
	/*
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"hex",				INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"bin",				INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"exe",				INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"gz",					INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"xz",					INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"rar",				INVALID_ITEM},
	{PT_WILD, "/dl",			INVALID_ITEM,		INVALID_ITEM, NULL, INVALID_ITEM,		"zip",				INVALID_ITEM},
	*/


	{PT_CFUNC, "/",				"",							HTTP_TRUE,		NULL, center_func,		INVALID_ITEM, INVALID_ITEM},
	{PT_CFUNC, "/",				"index.html",		HTTP_TRUE,		NULL, center_func,		INVALID_ITEM, INVALID_ITEM},
	{PT_CFUNC, "/",				"index.htm",		HTTP_TRUE,		NULL, center_func,		INVALID_ITEM, INVALID_ITEM},

	/*
	{PT_CFUNC, "/",				"include",			0,						NULL, include_func,		INVALID_ITEM, INVALID_ITEM},
	{PT_CFUNC, "/",				"exclude",			0,						NULL, exclude_func,		INVALID_ITEM, INVALID_ITEM},
	{PT_CFUNC, "/",				"list",					0,						NULL, list_func,			INVALID_ITEM, INVALID_ITEM},
	*/

	{PT_FILE,	"/",				"error",				0,						NULL, INVALID_ITEM,		"page/error.html",	INVALID_ITEM},
	{PT_FILE, "/",				"favicon.ico",	0,						NULL, INVALID_ITEM,		"image/favicon.ico",INVALID_ITEM},

	/*{PT_FILE, "/",				"main.sh",			0,						NULL, INVALID_ITEM,		"main/main.sh",			INVALID_ITEM}, */
};

static void web_add_nodes(httpd *svr) {
	int cnt = sizeof(wns)/sizeof(wns[0]);
	int i = 0;

	//httpdAddFileContent(svr, "/", "index", HTTP_TRUE, NULL, "/opt/au/tmp/smartdd/algo/www/index.html");
	//httpdAddCContent(svr, "/", "", HTTP_TRUE, NULL, index_function);
	//httpdAddCContent(svr, "/", "index", HTTP_TRUE, NULL, index_function);
	//httpdAddCContent(svr, "/", "index.html", HTTP_TRUE, NULL, index_function);
	//httpdAddCContent(svr, "/", "index.htm", HTTP_TRUE, NULL, index_function);
	//httpdAddWildcardContent(svr, "/images",NULL, "images");

	for (i = 0; i <  cnt; i++) {
		stWebNode_t *wn = &wns[i];
		switch (wn->type) {
			case PT_CFUNC:
				httpdAddCContent(svr, wn->dir, wn->name, wn->isindex, 
						(WEB_PRELOAD)wn->preload, (WEB_FUNCTION)wn->function);
				break;
			case PT_FILE:
				httpdAddFileContent(svr, wn->dir, wn->name, wn->isindex, wn->preload,
						wn->path);
				break;
			case PT_WILD:
				httpdAddWildcardContent(svr, wn->dir, wn->preload, wn->path);
				break;
			case PT_CWILD:
				httpdAddCWildcardContent(svr, wn->dir, wn->preload, wn->function);
				break;
			case PT_STATIC:
				httpdAddStaticContent(svr, wn->dir, wn->name, wn->isindex, wn->preload, 
					wn->html);
				break;
			default:
				break;
		}
	}
}

void web_init(void *_th, void *_fet, const char *ip, int port, const char *base) {
	we.th = _th;
	we.fet = _fet;

	timer_init(&we.step_timer, web_run);

	strcpy(we.ip, ip);
	we.port = port;
	strcpy(we.base, base);

	we.svr = httpdCreate(we.ip, we.port);
	if (we.svr == NULL) {
		printf("Couldn't create http server : %s %d\n", we.ip, we.port);
		return; 
	}

	httpdSetAccessLog(we.svr,	stdout);
	httpdSetErrorLog(we.svr,	stdout);
	httpdSetFileBase(we.svr,	we.base);

	web_add_nodes(we.svr);

	log_info("http server start at : %s:%d", we.ip, we.port);

	file_event_reg(we.fet, web_getfd(), web_in, NULL, NULL);
}
void web_step() {
	timer_cancel(we.th, &we.step_timer);
	timer_set(we.th, &we.step_timer, 10);
	return;
}
void web_push(void *data, int len) {
}
void web_run(struct timer *timer) {
}
void web_in(void *arg, int fd) {
	while (1) {
		struct timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		int result;
		httpReq *req =  httpdReadRequest(we.svr, &timeout, &result);
		if (req == NULL && result == 0) {
			return;
			continue;
		} 
		if (result < 0) {
			/* error */
			continue;
		}
		httpdProcessRequest(we.svr, req);
		httpdEndRequest(we.svr, req);
	}

}
int  web_getfd() {
	if (we.svr != NULL) {
		return we.svr->serverSock;
	}
	return -1;
}
////////////////////////////////////////////////////////////////
static void render_header(httpd *server, httpReq *req) {
	httpdPrintf(server, req, (char *)
		"<html>\n"\
		"	<head>\n"\
		"		<meta charset=\"UTF-8\">\n"
		"		<title>Pir Test</title>\n"
		"		<script language=\"JavaScript\" src=\"js/jquery-3.2.1.min.js\">\n"\
		"		</script>\n"\
		"		<script language=\"JavaScript\" src=\"js/dr.js\">\n"\
		"		</script>\n"\
		"	</head>\n"\
		"	<body><div id=\"body\">\n"\
	);
}
static void render_footer(httpd *server, httpReq *req) {
	httpdPrintf(server, req, (char *)
		"	</div></body>\n"\
		"</html>\n"
	);
}


static void center_func(httpd *server, httpReq *req) {
	render_header(server, req);
	render_footer(server, req);
}





