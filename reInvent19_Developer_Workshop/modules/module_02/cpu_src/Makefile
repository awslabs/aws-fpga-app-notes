
SRCDIR := .

include common.mk

build: host

run: build
	./host 100000 

run_fpga:
	make --no-print-directory -C ../makefile run STEP=sw_overlap ITER=16 SOLUTION=1

help:
	echo  " "
	@echo  " Makefile Usage:"
	@echo  " "
	@echo  "  Run Part 1 - Step 1 : make run "
