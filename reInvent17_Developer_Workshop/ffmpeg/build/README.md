## Instruction for building the FFmpeg executable included in this repository.

### Prerequisite
Log into an AWS EC2 instance loaded with the [FPGA Developer AMI](https://aws.amazon.com/marketplace/pp/B06VVYBLZZ) v1.3.5


### Build FFmpeg with custom FGPA-accelerated encoder

```sh
sudo sh

# Install necessary packages
yum install cmake freetype-devel mercurial

# Go to build directory
cd /home/centos/aws-fpga-app-notes/reInvent17_Developer_Workshop/ffmpeg/build
chmod +x *.sh

# Build dependencies
./build_dependencies.sh       

# Get FFmpeg sources
./get_ffmpeg.sh

# Copy x86_64 to ffmpeg_hevc/build/lib
cp -r /opt/Xilinx/SDx/2017.1.op/runtime/lib/x86_64 ffmpeg_hevc/build/lib/

# Copy CL to ffmpeg_hevc/build/include/
cp -r /opt/Xilinx/SDx/2017.1.op/runtime/include/1_2/CL ffmpeg_hevc/build/include/

# Build FFmpeg
./build_ffmpeg.sh

# Create a copy branch
cd ffmpeg_hevc/sources/ffmpeg/
git checkout -b ngcodec_hevc

# Apply copy encoder patch to FFmpeg
rm -rf .gitignore
git apply ../../../0001-Xilinx-NGcodec-hevc-ffmpeg-plugin.patch

# Build FFmpeg with patch
cd ../../../
./build_ffmpeg.sh
```

The newly compiled FFmpeg executable can be found here:

/home/centos/aws-fpga-app-notes/reInvent17_Developer_Workshop/ffmpeg/build/ffmpeg_hevc/sources/ffmpeg/ffmpeg_build/ffmpeg

