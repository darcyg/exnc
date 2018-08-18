ROOTDIR=$(shell pwd)
WORKDIR=$(ROOTDIR)/build

targets			+= test

.PHONY: targets

all : $(targets)


srcs				+= $(ROOTDIR)/libs/osi/src/cond.c
srcs				+= $(ROOTDIR)/libs/osi/src/mutex.c
srcs				+= $(ROOTDIR)/libs/osi/src/list.c
srcs				+= $(ROOTDIR)/libs/osi/src/lockqueue.c
srcs				+= $(ROOTDIR)/libs/osi/src/serial.c
srcs				+= $(ROOTDIR)/libs/osi/src/tcp.c

srcs				+= $(ROOTDIR)/libs/osi/src/nameval.c
srcs				+= $(ROOTDIR)/libs/osi/src/log.c
srcs				+= $(ROOTDIR)/libs/osi/src/crc.c
srcs				+= $(ROOTDIR)/libs/osi/src/time_utils.c
srcs				+= $(ROOTDIR)/libs/osi/src/timer.c
srcs				+= $(ROOTDIR)/libs/osi/src/hex.c
srcs				+= $(ROOTDIR)/libs/osi/src/parse_argv.c
srcs				+= $(ROOTDIR)/libs/osi/src/file_io.c
srcs				+= $(ROOTDIR)/libs/osi/src/file_event.c
srcs				+= $(ROOTDIR)/libs/osi/src/filesystem_monitor.c
srcs				+= $(ROOTDIR)/libs/osi/src/hashmap.c
srcs				+= $(ROOTDIR)/libs/osi/src/base64.c
srcs				+= $(ROOTDIR)/libs/osi/src/md5.c
srcs				+= $(ROOTDIR)/libs/osi/src/des.cpp
srcs				+= $(ROOTDIR)/libs/osi/src/util.c


srcs				+= $(ROOTDIR)/libs/third/json/src/dump.c
srcs				+= $(ROOTDIR)/libs/third/json/src/error.c
srcs				+= $(ROOTDIR)/libs/third/json/src/hashtable.c
srcs				+= $(ROOTDIR)/libs/third/json/src/hashtable_seed.c
srcs				+= $(ROOTDIR)/libs/third/json/src/json_parser.c
srcs				+= $(ROOTDIR)/libs/third/json/src/load.c
srcs				+= $(ROOTDIR)/libs/third/json/src/memory.c
srcs				+= $(ROOTDIR)/libs/third/json/src/pack_unpack.c
srcs				+= $(ROOTDIR)/libs/third/json/src/strbuffer.c
srcs				+= $(ROOTDIR)/libs/third/json/src/strconv.c
srcs				+= $(ROOTDIR)/libs/third/json/src/utf.c
srcs				+= $(ROOTDIR)/libs/third/json/src/value.c


srcs				+= $(ROOTDIR)/libs/third/libhttpd/src/api.c
srcs				+= $(ROOTDIR)/libs/third/libhttpd/src/ip_acl.c
srcs				+= $(ROOTDIR)/libs/third/libhttpd/src/version.c
srcs				+= $(ROOTDIR)/libs/third/libhttpd/src/ember.c
srcs				+= $(ROOTDIR)/libs/third/libhttpd/src/protocol.c
srcs				+= $(ROOTDIR)/exnc_svr.c
srcs				+= $(ROOTDIR)/web.c

srcs	      := $(subst .cpp,.c,$(srcs))


testsrcs		+= $(ROOTDIR)/main.c
testsrcs		+= $(srcs)
testsrcs    := $(subst .cpp,.c,$(testsrcs))

objs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(srcs)))
testobjs = $(subst $(ROOTDIR),$(WORKDIR), $(subst .c,.o,$(testsrcs)))



-include $(ROOTDIR)/make/arch.mk
-include $(ROOTDIR)/make/rules.mk


#$(eval $(call LinkLio,libdbsync.so$(VERSION),$(dbsyncobjs)))
$(eval $(call LinkApp,test,$(testobjs)))

run :
	sudo ./build/test
