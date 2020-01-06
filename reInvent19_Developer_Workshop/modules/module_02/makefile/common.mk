SHELL := /bin/sh
host: $(SRCDIR)/*.cpp $(SRCDIR)/*.c $(SRCDIR)/*.h
	mkdir -p $(BUILDDIR)
	g++ -D__USE_XOPEN2K8 -D__USE_XOPEN2K8 -I$(XILINX_XRT)/include -I$(SRCDIR) -O3 -Wall -fmessage-length=0 -std=c++11 \
	$(HOST_SRC_CPP) \
	-L$(XILINX_XRT)/lib/ \
	-lxilinxopencl -lpthread -lrt \
	-o $(BUILDDIR)/host

emconfig.json:
	cp $(SRCDIR)/emconfig.json .

xclbin: runOnfpga_$(TARGET).xclbin

xo: runOnfpga_$(TARGET).xo

clean:
	rm -rf temp_dir log_dir ../build report_dir *log host *.csv *summary .run .Xil vitis* *jou xilinx*
