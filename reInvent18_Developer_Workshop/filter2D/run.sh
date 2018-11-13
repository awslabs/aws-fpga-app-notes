#!/bin/bash

function run_on_f1 {
sudo sh -s -- <<EOF
        source /opt/xilinx/xrt/setup.sh
	./Filter2D.exe -x ./xclbin/fpga1k.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin -i ./img/picadilly_1080p.bmp -n 24
	./Filter2D.exe -x ./xclbin/fpga3k.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin -i ./img/picadilly_1080p.bmp -n 24
	./Filter2D.exe -x ./xclbin/fpga6k.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin -i ./img/picadilly_1080p.bmp -n 24
EOF
}

make
run_on_f1

