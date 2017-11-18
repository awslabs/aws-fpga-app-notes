
./ffmpeg -f rawvideo -pix_fmt yuv420p -s:v 1920x1080 \
	-i /home/centos/vectors/crowd8_420_1920x1080_50.yuv \
	-an -frames 1000 -f hevc \
	-c:v xlnx_hevc_enc -psnr -g 30 -global_quality 40 \
	-y ./crowd8_420_1920x1080_50_NGcodec_out0_g30_gq40.hevc 
