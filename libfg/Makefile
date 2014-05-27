# Filename: Makefile
#
# Purpose:  Makefile for libfg
#
# Author:   Stephen Page

override cpu   := $(shell uname -m)
override os    := $(shell uname -s)

# Paths

exec_path       = $(os)/$(cpu)
dep_path        = $(os)/$(cpu)/dep
inc_path        = inc
lib             = $(exec_path)/libfg.a
obj_path        = $(os)/$(cpu)/obj
src_path        = src
doxygen_path    = html

vpath %.c $(src_path)
vpath %.h $(inc_path)

# Source and objects

source          = $(notdir $(wildcard $(src_path)/*.c))
objects         = $(source:%.c=$(obj_path)/%.o)

# Libraries

includes       += -I$(inc_path)

# Tools

AR             := $(shell which ar)
CC             := $(shell which gcc)
SHELL          := $(shell which sh)

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
