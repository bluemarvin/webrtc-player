# These should be defined in .geckopaths
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0

include .config
PLATFORM=Roku
PKG_DIR=pkg
PKG_IMAGE_DIR=$(PKG_DIR)/images
PKG_LIB_DIR=$(PKG_DIR)/lib
PKG_SOURCE_DIR=$(PKG_DIR)/source
PKG_FILE=webrtcplayer_cramfs.bin

include build/common.mk
include build/$(PLATFORM).mk

BUILD_DIR=./obj

LIBS = \
$(GECKO_OBJ)/mfbt/libmfbt.a.desc \
$(GECKO_OBJ)/layout/media/webrtc/libwebrtc.a.desc \
$(GECKO_OBJ)/media/libopus/libmedia_libopus.a.desc \
$(GECKO_OBJ)/media/libvpx/libmedia_libvpx.a.desc \
$(GECKO_OBJ)/media/libjpeg/libmedia_libjpeg.a.desc \
$(GECKO_OBJ)/media/libspeex_resampler/src/libspeex.a.desc \
$(GECKO_OBJ)/netwerk/srtp/src/libnksrtp_s.a.desc \
$(GECKO_OBJ)/media/mtransport/standalone/libmtransport_standalone.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingstandalone/signaling_ecc/libecc.a.desc \
$(GECKO_OBJ)/media/webrtc/signalingstandalone/signaling_sipcc/libsipcc.a.desc \
$(GECKO_OBJ)/media/standalone/libmediastandalone.a.desc \
$(GECKO_OBJ)/media/libyuv/libyuv_libyuv/libyuv.a.desc

LIB_ROLLUP = $(BUILD_DIR)/librollup.a

all: webrtcplayer

webrtcplayer: $(BUILD_DIR)/main.o $(LIB_ROLLUP)
	$(CXX) $(BUILD_DIR)/main.o $(LIB_ROLLUP) $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

clean:
	rm -f $(LIB_ROLLUP) $(BUILD_DIR)/main.o

clobber: clean
	rm -f webrtcplayer
	rm -rf $(BUILD_DIR)
	rm -rf $(PKG_DIR)
	rm -f $(PKG_FILE)

package: webrtcplayer
	echo Creating package...
	@rm -f $(PKG_NAME)
	@rm -rf $(PKG_DIR)
	@mkdir -p $(PKG_IMAGE_DIR)
	@mkdir -p $(PKG_LIB_DIR)
	@mkdir -p $(PKG_SOURCE_DIR)
	@cp manifest $(PKG_DIR)
	@cp webrtcplayer $(PKG_DIR)
	@cp source/main.brs $(PKG_SOURCE_DIR)
	@cp -L $(GECKO_OBJ)/dist/lib/libmozalloc.so $(PKG_LIB_DIR)
	@cp -L $(GECKO_OBJ)/dist/lib/libplc4.so $(PKG_LIB_DIR)
	@cp -L $(GECKO_OBJ)/dist/lib/libnspr4.so $(PKG_LIB_DIR)
	@cp -L $(GECKO_OBJ)/dist/lib/libplds4.so $(PKG_LIB_DIR)
	@cp frame.i420 $(PKG_IMAGE_DIR)
	$(ROKU_NDK)/bin/mkcramfs_roku $(PKG_DIR) $(PKG_FILE)

USERPASS = rokudev:1111
ROKU_DEV_TARGET = 10.252.120.189

install: package
	curl -v --user $(USERPASS) --anyauth -S -F "mysubmit=Install" -F "archive=@$(PKG_FILE)" -F "passwd=" http://$(ROKU_DEV_TARGET)/plugin_install
