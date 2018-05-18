Instruction for building the FFmpeg executable included in this repository.

--------------------------------------------------------------
Prerequisite
--------------------------------------------------------------
Log into an AWS EC2 instance loaded with the [FPGA Developer AMI](https://aws.amazon.com/marketplace/pp/B06VVYBLZZ) v1.3.5

--------------------------------------------------------------
Configure the EC2 instance
--------------------------------------------------------------
```sh
curl -s https://s3.amazonaws.com/aws-ec2-f1-reinvent-17/github_repo/aws-fpga.tar.gz -o aws-fpga.tar.gz
tar -xzf aws-fpga.tar.gz
rm aws-fpga.tar.gz
git clone https://github.com/awslabs/aws-fpga-app-notes.git
cd aws-fpga
source sdaccel_setup.sh
source $XILINX_SDX/settings64.sh
cd ~/aws-fpga-app-notes/reInvent17_Developer_Workshop/ffmpeg/build
sudo sh
source /opt/Xilinx/SDx/2017.1.rte/setup.sh
```
--------------------------------------------------------------
Install FFmpeg and its dependencies
--------------------------------------------------------------
```
# Run build_dependencies.sh
./build_dependencies.sh       

# Run get_ffmpeg.sh
./get_ffmpeg.sh

# Copy x87_64 to ffmpeg_hevc/build/lib
cp -r /opt/Xilinx/SDx/2017.1.op/runtime/lib/x86_64 ffmpeg_hevc/build/lib/

# Copy CL to ffmpeg_hevc/build/include/
cp -r /opt/Xilinx/SDx/2017.1.op/runtime/include/1_2/CL ffmpeg_hevc/build/include/

# Run build_ffmpeg.sh
./build_ffmpeg.sh
```

--------------------------------------------------------------
Apply custom encoder patch and build FFmpeg
--------------------------------------------------------------
```
# Create a copy branch
cd ffmpeg_hevc/sources/ffmpeg/
git checkout -b ngcodec_hevc

# Apply copy encoder patch to FFmpeg
rm -rf .gitignore
git apply ../../../0001-Xilinx-NGcodec-hevc-ffmpeg-plugin.patch

# Run build_ffmpeg.sh
cd ../../../
./build_ffmpeg.sh
```
