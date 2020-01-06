TARGET := hw

PF     := 8
ITER   := 

STEP := single_buffer
STEP := split_buffer
STEP := generic_buffer
STEP := sw_overlap

BUILDDIR := ./../build/$(STEP)


ifeq ($(SOLUTION),1)
	SRCDIR := ./../reference_files
else
	SRCDIR := ./../src/$(STEP)
endif



HOST_SRC_CPP := $(SRCDIR)/compute_score_host.cpp
HOST_SRC_CPP += $(SRCDIR)/MurmurHash2.c
HOST_SRC_CPP += $(SRCDIR)/xcl2.cpp
HOST_SRC_CPP += $(SRCDIR)/main.cpp 

ifeq ($(SOLUTION),1)
	HOST_SRC_CPP += $(SRCDIR)/run_$(STEP).cpp
else
	HOST_SRC_CPP += $(SRCDIR)/run_fpga.cpp
endif


include common.mk

setup:
	if ! [ -f temp.txt ]; \
  	then \
		touch temp.txt; \
		sudo -E -- bash -c 'source ~/aws-fpga/sdaccel_setup.sh'; \
	fi;



build: host

run: build setup
	mkdir -p $(BUILDDIR)
	cp runOnfpga_hw.awsxclbin $(BUILDDIR)
	cp xrt.ini $(BUILDDIR)
	sudo -E -- bash -c 'fpga-clear-local-image -S 0'
	sudo -E -- bash -c 'source /opt/xilinx/xrt/setup.sh && cd $(BUILDDIR) && ./host 100000 $(ITER) '
	 

help:
	@echo  " "
	@echo  " "
	@echo  " Makefile Usage:"
	@echo  " "
	@echo  "  Run Part 2 : "  - 
	@echo  "     Step 1 : make run STEP=single_buffer SOLUTION=1"
	@echo  "     Step 2 : make run STEP=split_buffer SOLUTION=1"
	@echo  "     Step 3 : make run STEP=generic_buffer ITER=16 SOLUTION=1"
	@echo  "     Step 4 : make run STEP=sw_overlap ITER=16 SOLUTION=1"
	@echo  " "
	@echo  "  Generate and View Profile Repprt:"
	@echo  "  sdx_analyze  profile  –f html -i ./profile_summary.csv; firefox ./profile_summary;"
	@echo  " "
	@echo  "  Generate and view timeline Trace:"
	@echo  "  sdx_analyze trace –f wdb -I ./timeline_trace.csv; sdx –workspace workspace –report timeline_trace.wdb"
	@echo  ""
