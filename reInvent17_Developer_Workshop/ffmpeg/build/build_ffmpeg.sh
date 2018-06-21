H=$PWD/ffmpeg_hevc

echo ======================== BUILDING ffmpeg =========================
cd $H/sources
cd ffmpeg
cd ffmpeg_build
PATH="$H/bin:$PATH" PKG_CONFIG_PATH="$H/build/lib/pkgconfig" ../configure \
  --prefix="$H/build" \
  --pkg-config-flags="--static" \
  --extra-cflags="-I$H/build/include" \
  --extra-ldflags="-L$H/build/lib -L$H/build/lib/x86_64 -ldl" \
  --bindir="$H/bin" \
  --enable-static \
  --enable-gpl \
  --enable-libfreetype \
  --enable-libx265 \
  --enable-opencl \
  --enable-debug 
PATH="$H/bin:$PATH" make -j 4
make install
hash -r
