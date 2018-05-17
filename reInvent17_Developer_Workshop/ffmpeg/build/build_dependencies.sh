H=$PWD/ffmpeg_hevc
mkdir -p $H/sources

# Build NASM
echo ======================== BUILDING NASM =========================
cd $H/sources
wget http://www.nasm.us/pub/nasm/releasebuilds/2.13.01/nasm-2.13.01.tar.gz
tar xzvf nasm-2.13.01.tar.gz
cd nasm-2.13.01
./configure --prefix="$H/build" --bindir="$H/bin"
make
make install

# Build YASM
echo ======================== BUILDING YASM =========================
cd $H/sources
wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz
tar xzvf yasm-1.3.0.tar.gz
cd yasm-1.3.0
./configure --prefix="$H/build" --bindir="$H/bin"
make
make install  

# Build libx265
echo ======================== BUILDING libx265 =========================
cd $H/sources
hg clone https://bitbucket.org/multicoreware/x265
cd $H/sources/x265/build/linux
PATH="$H/bin:$PATH" cmake -G "Unix Makefiles" -DCMAKE_INSTALL_PREFIX="$H/build" -DENABLE_SHARED:bool=off ../../source
make
make install
