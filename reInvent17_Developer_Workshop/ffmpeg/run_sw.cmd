
./ffmpeg -f rawvideo -pix_fmt yuv420p -s:v 1920x1080 \
	-i /home/centos/vectors/crowd8_420_1920x1080_50.yuv \
	-an -frames 1000 -f hevc \
	-c:v libx265 -preset medium -g 30 -q 40 \
	-y ./crowd8_420_1920x1080_50_libx265_out0_qp40.hevc

