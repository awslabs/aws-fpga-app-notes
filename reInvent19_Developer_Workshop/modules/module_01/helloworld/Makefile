.PHONY: help

help::
	@echo  ""
	@echo  " Makefile Usage:"
	@echo  ""
	@echo  " Make target to run  TARGET : sw_emu or hw_emu or hw"
	@echo  ""
	@echo  " make run TARGET=sw_emu/hw_emu/hw "
	@echo  ""


## platform selection
#PLATFORM := xilinx_u200_xdma_201830_2
#PLATFORM := $(AWS_PLATFORM)
PLATFORM := xilinx_aws-vu9p-f1-04261818_dynamic_5_0


## TARGET can be set as:
TARGET := sw_emu

## Below are the names for host executable and xclbin.
## Please keep it unchanged.
HOST_EXE := helloworld
XO_NAME := vector_addition_$(TARGET)
XCLBIN := vector_addition_$(TARGET).awsxclbin


## BUILD_DIR are used for the generated files, including .exe, .xclbin, and the reports/logs. BUILD_DIR selection should be matched to SRC_REPO for better structure.
PROJECT_DIR := .

SRC_REPO := $(PROJECT_DIR)/src

BUILD_DIR := $(PROJECT_DIR)/build

## Host Application files repository

HOST_SRC_CPP := $(SRC_REPO)/host.cpp
HOST_SRC_CPP += $(SRC_REPO)/xcl2.cpp
HOST_SRC_H += $(SRC_REPO)/xcl2.hpp



## Kernel Source Files repository

KERNEL_SRC_CPP := $(SRC_REPO)/vector_addition.cl
KERNEL_SRC_H_DIR := $(SRC_REPO)


# Host Compiler Global Settings and Include Libraries

CXXFLAGS += -D__USE_XOPEN2K8
CXXFLAGS += -D__USE_XOPEN2K8
CXXFLAGS += -I$(XILINX_XRT)/include/
CXXFLAGS += -I$(XILINX_VIVADO)/include/
CXXFLAGS += -I$(SRC_REPO)
#CXXFLAGS += -O0 -g -Wall -fmessage-length=0 -std=c++14
CXXFLAGS += -O0 -g -Wall -fmessage-length=0


CXXLDFLAGS := -L$(XILINX_XRT)/lib/
CXXLDFLAGS += -lxilinxopencl -lpthread -lrt


## Kernel Compiler and Linker Flags

XOCCFLAGS := -t $(TARGET)
XOCCFLAGS += --platform $(PLATFORM) -R 1
XOCCFLAGS += --nk vector_add:1
XOCCFLAGS += -I$(KERNEL_SRC_H_DIR)
XOCCFLAGS += --temp_dir $(BUILD_DIR)
XOCCFLAGS += --log_dir $(BUILD_DIR)
XOCCFLAGS += --profile_kernel data:all:all:all:all

## Host Executable File Generation

$(BUILD_DIR)/$(HOST_EXE): $(HOST_SRC_CPP) $(HOST_SRC_H)
	mkdir -p $(BUILD_DIR)
	xcpp $(CXXFLAGS) $(HOST_SRC_CPP) $(CXXLDFLAGS) -o $@
	mv $(BUILD_DIR)/$(HOST_EXE) $(PROJECT_DIR)


## Kernel XO and Xclbin File Generation

$(BUILD_DIR)/$(XO_NAME).xo: $(KERNEL_SRC_CPP) $(KERNEL_SRC_H)
	mkdir -p $(BUILD_DIR)
	xocc $(XOCCFLAGS) -c -k vector_add $(KERNEL_SRC_CPP) -o $@

$(BUILD_DIR)/$(XCLBIN): $(BUILD_DIR)/$(XO_NAME).xo
	mkdir -p $(BUILD_DIR)
	xocc $(XOCCFLAGS) -l -o $@ $(BUILD_DIR)/$(XO_NAME).xo

## Emulation Files Generation

EMCONFIG_FILE = emconfig.json

$(BUILD_DIR)/$(EMCONFIG_FILE):
	 emconfigutil --nd 1  --platform $(PLATFORM) --od $(BUILD_DIR)


#
# primary build targets
#

.PHONY: all clean

## build the design without running host application

#build: $(BUILD_DIR)/$(HOST_EXE) $(BUILD_DIR)/$(XCLBIN) $(BUILD_DIR)/$(EMCONFIG_FILE)
compile_host: $(BUILD_DIR)/$(HOST_EXE) 


## build the design and then run host application
## run with 512x10 input video format for emulation and 1280x720 for hardware

run: build
	./$(HOST_EXE)  ./xclbin/$(XCLBIN) ;


## Clean generated files

clean:
	rm -rf $(BUILD_DIR) xrc.log profile_summary.csv helloworld
