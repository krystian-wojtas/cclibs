# Filename: Makefile
#
# Purpose:  Makefile for libcc
#
# Author:   Stephen Page

override cpu   := $(shell uname -m)
override os    := $(shell uname -s)

# Paths

exec_path       = $(os)/$(cpu)
dep_path        = $(os)/$(cpu)/dep
inc_path        = inc
lib             = $(exec_path)/libcc.a
obj_path        = $(os)/$(cpu)/obj
src_path        = src
doxygen_path    = html

# Libraries

libfg_path      = ../libfg
libfg_inc       = $(libfg_path)/inc
libfg_src       = $(libfg_path)/src

libreg_path     = ../libreg
libreg_inc      = $(libreg_path)/inc
libreg_src      = $(libreg_path)/src

vpath %.c $(src_path):$(libfg_src):$(libreg_src)
vpath %.h $(inc_path):$(libfg_inc):$(libreg_inc)

# Source and objects

source          = $(notdir $(wildcard $(src_path)/*.c))
objects         = $(source:%.c=$(obj_path)/%.o)

# Libraries

includes       += -I$(inc_path) -I$(libfg_inc) -I$(libreg_inc)

# Tools

AR             := $(shell which ar)
CC             := $(shell which gcc)

CFLAGS          = -O3 -g -Wall

# Targets

all: $(lib)

# Clean output files

clean:
	rm -rf $(doxygen_path) $(dep_path)/*.d $(obj_path)/*.o $(lib)

$(lib): $(objects)
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(AR) -rs $@ $?

# Dependencies

include $(wildcard $(dep_path)/*.d)

# C objects

$(obj_path)/%.o: %.c
	@[ -d $(@D) ]       || mkdir -p $(@D)
	@[ -d $(dep_path) ] || mkdir -p $(dep_path)
	$(CC) $(CFLAGS) -MD -MF $(@:$(obj_path)/%.o=$(dep_path)/%.d) $(includes) -c -o $@ $<

# Special targets

doxygen:
	doxygen .doxygen

.PHONY: all clean doxygen

# EOF
