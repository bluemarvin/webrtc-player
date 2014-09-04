# These should be defined in .config
#GECKO_ROOT = /some/path/gecko-dev
#GECKO_OBJ = $(GECKO_ROOT)/obj-xxx-xxxxx-xxxx
#ROKU_NDK = /some/path/roku
#ROKU_DEV_TARGET = Roku device IP address
#ROKU_USERPASS = username:passwd for Roku device

include .config
PLATFORM=Roku
PKG_DIR=pkg
PKG_IMAGE_DIR=$(PKG_DIR)/images
PKG_LIB_DIR=$(PKG_DIR)/lib
PKG_SOURCE_DIR=$(PKG_DIR)/source
PKG_FILE=webrtcplayer_cramfs.bin
GECKO_DIST=$(GECKO_OBJ)/dist/lib

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

OBJ_FILES = $(BUILD_DIR)/main.o $(BUILD_DIR)/renderGL.o

all: webrtcplayer

webrtcplayer: $(OBJ_FILES) $(LIB_ROLLUP)
	$(CXX) $(OBJ_FILES) $(LIB_ROLLUP) $(LFLAGS) -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(SDL_CFLAGS) $(INCLUDE) $^ -c -o $@

$(LIB_ROLLUP): $(LIBS)
	@mkdir -p $(BUILD_DIR)
	$(AR) cr $@ `python ./tools/expand.py $(LIBS)`

#-include $(OBJ_FILES:.o=.d)

clean:
	rm -f $(LIB_ROLLUP) $(BUILD_DIR)/main.o

clobber: clean
	rm -f webrtcplayer
	rm -rf $(BUILD_DIR)
	rm -rf $(PKG_DIR)
	rm -f $(PKG_FILE)

PKG_LIBS = \
$(GECKO_DIST)/libfreebl3.so \
$(GECKO_DIST)/libmozalloc.so \
$(GECKO_DIST)/libmozsqlite3.so \
$(GECKO_DIST)/libsmime3.so \
$(GECKO_DIST)/libsoftokn3.so \
$(GECKO_DIST)/libnssutil3.so \
$(GECKO_DIST)/libplc4.so \
$(GECKO_DIST)/libplds4.so \
$(GECKO_DIST)/libnspr4.so

# $(GECKO_DIST)/libnss3.so \
# $(GECKO_DIST)/libnssckbi.so \
# $(GECKO_DIST)/libnssdbm3.so \
# $(GECKO_DIST)/libssl3.so

prepackage: webrtcplayer
	echo Creating package...
	@rm -f $(PKG_NAME)
	@rm -rf $(PKG_DIR)
	@mkdir -p $(PKG_IMAGE_DIR)
	@mkdir -p $(PKG_LIB_DIR)
	@mkdir -p $(PKG_SOURCE_DIR)
	@cp manifest $(PKG_DIR)
	@cp webrtcplayer $(PKG_DIR)
	cp -L $(PKG_LIBS) $(PKG_LIB_DIR)

package: prepackage
	$(ROKU_NDK)/platforms/Roku2/toolchain/bin/arm-linux-strip $(PKG_DIR)/webrtcplayer
	@cp source/main.brs $(PKG_SOURCE_DIR)
	$(ROKU_NDK)/bin/mkcramfs_roku $(PKG_DIR) $(PKG_FILE)

dbpackage: prepackage
	cp $(ROKU_NDK)/platforms/Roku2/toolchain/arm-brcm-linux-gnueabi/debug-root/usr/bin/gdbserver $(PKG_DIR)
	@cp source/dbmain.brs $(PKG_SOURCE_DIR)/main.brs
	tools/paxctl.sh $(PKG_DIR) $(ROKU_NDK) webrtcplayer
	$(ROKU_NDK)/bin/mkcramfs_roku $(PKG_DIR) $(PKG_FILE)

install: package
	curl -v --user $(ROKU_USERPASS) --anyauth -S -F "mysubmit=Install" -F "archive=@$(PKG_FILE)" -F "passwd=" http://$(ROKU_DEV_TARGET)/plugin_install

dbinstall: dbpackage
	curl -v --user $(ROKU_USERPASS) --anyauth -S -F "mysubmit=Install" -F "archive=@$(PKG_FILE)" -F "passwd=" http://$(ROKU_DEV_TARGET)/plugin_install
