# Filename: Makefile
#
# Purpose:  Makefile for ccrt
#
# Author:   Stephen Page

override cpu   := $(shell uname -m)
override os    := $(shell uname -s)

# Paths

exec_path       = $(os)/$(cpu)
exec            = $(exec_path)/ccrt
dep_path        = $(os)/$(cpu)/dep
inc_path        = inc
obj_path        = $(os)/$(cpu)/obj
src_path        = src
sd_path         = /sdcard/projects/webplots

# Libraries

libfg_path      = ../libfg
libfg_inc       = $(libfg_path)/inc
libfg_src       = $(libfg_path)/src

libreg_path     = ../libreg
libreg_inc      = $(libreg_path)/inc
libreg_src      = $(libreg_path)/src

libcc_path      = ../libcc
libcc_inc       = $(libcc_path)/inc
libcc_src       = $(libcc_path)/src

libterm_path    = ../libterm
libterm_inc     = $(libterm_path)/inc
libterm_src     = $(libterm_path)/src

libs            = -lm -lrt

# Source and objects

vpath %.c $(src_path):$(libfg_src):$(libreg_src):$(libcc_src):$(libterm_src)
vpath %.h $(inc_path):$(libfg_inc):$(libreg_inc):$(libcc_inc):$(libterm_inc)

#source          = $(notdir $(wildcard $(src_path)/*.c $(libfg_src)/*.c $(libreg_src)/*.c $(libcc_src)/*.c $(libterm_src)/*.c))
source          = $(notdir $(wildcard $(src_path)/*.c $(libterm_src)/*.c))
objects         = $(source:%.c=$(obj_path)/%.o)

# header files

includes       += -I$(inc_path) -I$(libfg_inc) -I$(libreg_inc) -I$(libcc_inc) -I$(libterm_inc)

# Tools

CC              := $(shell which gcc)

CFLAGS          = -O3 -g -Wall

# Targets

all: $(exec)

# Clean output files

clean:
	rm -f $(exec) $(dep_path)/*.d $(obj_path)/*.o $(inc_path)/flot.h results/webplots/converters/*

$(exec): $(objects) $(libfg) $(libreg) $(libcc)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^ $(libs)

# Dependencies

include $(wildcard $(dep_path)/*.d)

# C objects

$(obj_path)/%.o: %.c $(inc_path)/flot.h
	@[ -d $(@D) ]       || mkdir -p $(@D)
	@[ -d $(dep_path) ] || mkdir -p $(dep_path)
	$(CC) $(CFLAGS) -MD -MF $(@:$(obj_path)/%.o=$(dep_path)/%.d) $(includes) -c -o $@ $<

# Special targets

# flot header file

$(inc_path)/flot.h: $(inc_path)/flot/flot_header.h $(inc_path)/flot/flot_footer.h $(inc_path)/flot/flot_part0.htm $(inc_path)/flot/flot_part1.htm $(inc_path)/flot/flot_part2.htm $(inc_path)/flot/flot_part3.htm $(inc_path)/flot/flot_part4.htm
	$(inc_path)/flot/htmlquote.sh $^ >$@

# List targets

.PHONY: all clean

# EOF
