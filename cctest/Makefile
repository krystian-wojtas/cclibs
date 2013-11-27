# Filename: Makefile
#
# Purpose:  Makefile for cctest
#
# Author:   Stephen Page

override cpu   := $(shell uname -m)
override os    := $(shell uname -s)

# Paths

exec_path       = $(os)/$(cpu)
exec            = $(exec_path)/cctest
dep_path        = $(os)/$(cpu)/dep
inc_path        = inc
obj_path        = $(os)/$(cpu)/obj
src_path        = src
dropbox_path    = /c/Dropbox/cctest
sd_path         = /sdcard/projects/webplots

# Libraries

libfg_path      = ../libfg
libfg_inc       = $(libfg_path)/inc
libfg_src       = $(libfg_path)/src

libreg_path     = ../libreg
libreg_inc      = $(libreg_path)/inc
libreg_src      = $(libreg_path)/src

libs            = -lm

# Source and objects

vpath %.c $(src_path):$(libfg_src):$(libreg_src)
vpath %.h $(inc_path):$(libfg_inc):$(libreg_inc)

source          = $(notdir $(wildcard $(src_path)/*.c $(libfg_src)/*.c $(libreg_src)/*.c))
objects         = $(source:%.c=$(obj_path)/%.o)

# header files

includes       += -I$(inc_path) -I$(libfg_inc) -I$(libreg_inc)

# Tools

CC              := $(shell which gcc)

CFLAGS          = -O3 -g -Wall

# Targets

all: $(exec)

# Clean output files

clean:
	rm -f $(exec) $(dep_path)/*.d $(obj_path)/*.o 

$(exec): $(objects) $(libfg) $(libreg)
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

$(inc_path)/flot.h: $(inc_path)/flot/flot_header.h $(inc_path)/flot/flot_footer.h $(inc_path)/flot/flot_part1.htm $(inc_path)/flot/flot_part2.htm $(inc_path)/flot/flot_part3.htm $(inc_path)/flot/flot_part4.htm
	$(inc_path)/flot/htmlquote.sh $^ >$@

# Make flot output files

flot:
	./scripts/tests/run_all.sh

sandbox:
	./scripts/sandbox/run_all.sh

# Make test output the new reference

reference:
	rm -rf results/csv/reference
	cp -rp results/csv/tests results/csv/reference

# Generate test output and compare it to the reference

test:
	./scripts/tests/run_all.sh STANDARD
	status=0 ;\
	cd results/csv/reference && \
	for file in `find -type f` ;\
	do \
	    cmp "$$file" ../tests/"$$file" || status=1 ;\
	done ;\
	exit $$status

# Install to dropbox

dropbox:
	cp $(exec) $(dropbox_path)
	rsync -av scripts/sandbox/RSTtests $(dropbox_path)/scripts --exclude=*.sh
	rsync -av --delete --delete-excluded results/webplots/flot      $(dropbox_path)/results/webplots/ --exclude=.git
	rsync -av --delete --delete-excluded results/webplots/colorbox  $(dropbox_path)/results/webplots/ --exclude=.git
	rsync -av --delete --delete-excluded results/webplots/modernizr $(dropbox_path)/results/webplots/ --exclude=.git

# Refresh flot related files to sdcard projects folder (for debian on Nexus 10)

sdrefresh:
	rsync -av --delete --delete-excluded results/webplots/flot      $(sd_path)/results/webplots/ --exclude=.git
	rsync -av --delete --delete-excluded results/webplots/colorbox  $(sd_path)/results/webplots/ --exclude=.git
	rsync -av --delete --delete-excluded results/webplots/modernizr $(sd_path)/results/webplots/ --exclude=.git

# List targets

.PHONY: all clean reference test flot sandbox dropbox sdrefresh

# EOF
