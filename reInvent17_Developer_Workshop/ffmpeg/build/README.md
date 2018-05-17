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
3. Copy x87_64 to ffmpeg_hevc/build/lib
        cp -r /opt/Xilinx/SDx/2017.1.op/runtime/lib/x86_64 ffmpeg_hevc/build/lib/
4. Copy CL to ffmpeg_hevc/build/include/
        cp -r /opt/Xilinx/SDx/2017.1.op/runtime/include/1_2/CL ffmpeg_hevc/build/include/
5. Run build_ffmpeg.sh.
        ./build_ffmpeg.sh

--------------------------------------------------------------
Apply custom encoder patch
--------------------------------------------------------------
1. Creat a copy branch
        cd ffmpeg_hevc/sources/ffmpeg/
        git checkout -b ngcodec_hevc
2. Apply copy encoder patch in ffmpeg.
        rm -rf .gitignore
        git apply ../../../0001-Xilinx-NGcodec-hevc-ffmpeg-plugin.patch
3. Run build_ffmpeg.sh
        cd ../../../
        ./build_ffmpeg.sh
