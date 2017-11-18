H=$PWD
export XLNX_XCLBIN_PATH=$H/xclbin
export XLNX_TARGET_DEVICE=xilinx:xilinx:aws-vu9p-f1:4ddr-xpr-2pr:4.0
export PATH=$PATH:$H/ffmpeg/bin
export VU9P_HEVC_ENC_NUM_CH=1

chmod +x ./ffmpeg
