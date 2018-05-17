H=$PWD/ffmpeg_hevc

echo ======================== BUILDING ffmpeg =========================
cd $H/sources
rm -rf ffmpeg
git clone https://git.ffmpeg.org/ffmpeg.git ffmpeg -b release/3.3
cd ffmpeg
mkdir ffmpeg_build
