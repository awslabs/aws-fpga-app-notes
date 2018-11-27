
Version with 1 CU
- make check NKERNEL=1 TARGET=sw_emu
- make check NKERNEL=1 TARGET=hw_emu
- make build NKERNEL=1 TARGET=hw
- 250Mhz
- ~31x faster than SW

Version with 3 CUs
- make check NKERNEL=3 TARGET=sw_emu
- make check NKERNEL=3 TARGET=hw_emu
- make build NKERNEL=3 TARGET=hw
- 250Mhz
- ~91x faster than SW

Version with 6 CUs
- make check NKERNEL=6 TARGET=sw_emu
- make check NKERNEL=6 TARGET=hw_emu
- make build NKERNEL=6 TARGET=hw
- 213Mhz
- ~135x faster than SW

Note:
- The run.sh script will run all 3 version back to back on the instance
- It takes care of running as sudo, so no need to do sudo ./run.sh

Note:
- All kernel ports connected to default DDR bank
- The more CUs, the more traffic on that DDR interface

Note:
- The SW version uses OpenMP pragma to parallelize computation, taking advantage of all CPU cores
