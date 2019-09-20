##############################################
#	Makefile for kidsmath app
#
###############################################

MEMMAN_HOME = $(shell pwd)
BIN_PATH	= $(MEMMAN_HOME)/bin
INC_PATH	= $(MEMMAN_HOME)/inc

CROSS_TOOL =
AS = $(CROSS_TOOL)as
CC = $(CROSS_TOOL)gcc
LD = $(CROSS_TOOL)ld
OD = $(CROSS_TOOL)objdump
OC = $(CROSS_TOOL)objcopy
AR = $(CROSS_TOOL)ar

RM  	     := rm -f

################################################


################################################
app			:= $(BIN_PATH)/kidsmath

sources := 	src/main.c	\
		src/bitcolumnmatrix.c	\
		src/maths.c	\
		src/simplerandom.c	\
		src/simplerandom-discard.c	\

################################################
objects		:= $(subst .c,.o,$(sources))
dependencies 	:= $(subst .c,.d,$(sources))
include_dirs 	:= $(INC_PATH)

CFLAGS += -std=gnu99 -I $(include_dirs) -lm -L/usr/lib/x86_64-linux-gnu/ -lreadline -lncurses -g

################################################
.PHONY:	all $(app)
all: $(app)

$(app): $(objects)
	$(CC)  $^ $(CFLAGS) -o $@


clean:
	$(RM)  $(app) $(objects) $(dependencies)

################################################


%.d: %.c
	$(SHELL) -ec '$(CC) -M $(CFLAGS) $< | sed '\''s,$(notdir $*).o,& $@,g'\'' > $@'

-include $(sources:.c=.d)




