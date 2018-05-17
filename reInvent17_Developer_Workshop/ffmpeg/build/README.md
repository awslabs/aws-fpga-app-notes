

Instruction for building the FFmpeg executable included in this repository.


--------------------------------------------------------------
Setup the F1 instance
--------------------------------------------------------------
1. Log into an AWS F1 instance
2. git clone https://github.com/aws/aws-fpga -b RC_v1_3_3
3. git clone https://github.com/awslabs/aws-fpga-app-notes.git
4. cd aws-fpga
5. source sdaccel_setup.sh 
6. source $XILINX_SDX/settings64.sh
7. cd ~/aws-fpga-app-notes/reInvent17_Developer_Workshop/ffmpeg/plugin
8. Setup F1 environment to run application
	sudo sh
	source /opt/Xilinx/SDx/2017.1.rte/setup.sh

--------------------------------------------------------------
Install FFmpeg and its dependencies
--------------------------------------------------------------
1. Run build_dependencies.sh
        ./build_dependencies.sh
2. Run get_ffmpeg.sh
        ./get_ffmpeg.sh
3. Copy x86_64 to ffmpeg_hevc/build/lib
        cp -r x86_64/ ffmpeg_hevc/build/lib/
4. Copy CL to ffmpeg_hevc/build/include/
        cp -r CL/ ffmpeg_hevc/build/include/
5. Run build_ffmpeg.sh.
        ./build_ffmpeg.sh

--------------------------------------------------------------
Apply custom encoder patch
--------------------------------------------------------------
1. Create a copy branch
	cd ffmpeg_hevc/sources/ffmpeg/
        git checkout -b ngcodec_hevc
2. Apply custom encoder patch to FFmpeg
	rm -rf .gitignore
	git apply ../../../NGcodec_hevc_10202017.patch
3. Run build_ffmpeg.sh
        cd ../../../
        ./build_ffmpeg.sh		
