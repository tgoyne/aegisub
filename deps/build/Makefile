PLATS=linux linux-clang macosx

none:
	@echo "Please do 'make PLATFORM'"
	@echo "Where PLATFORM is one of:"
	@echo "    $(PLATS)"
	@echo
	@echo "For debug builds, add DEBUG=true"

.PHONY: all $(PLATS) clean none

COMMON_DEBUG=--enable-debug --enable-shared --disable-static

build/ffmpeg-debug: ../ffmpeg
	mkdir -p build/ffmpeg-debug
	cd build/ffmpeg-debug; ../../../ffmpeg/configure $(COMMON_DEBUG) --disable-filters --disable-network --disable-devices --enable-gpl --disable-encoders --enable-small --disable-muxers --disable-postproc --disable-avfilter --disable-ffprobe --disable-ffmpeg --disable-ffplay --disable-ffserver --disable-avx --disable-outdevs --disable-indevs --enable-avresample --cc=clang --prefix=/src/aegisub/deps/build/output/debug
	cd build/ffmpeg-debug; make -j5

output/debug: build/ffmpeg-debug
	cd build/ffmpeg-debug; make install

$(PLATS): output/debug
