# These should be defined in .geckopaths
#GECKO_ROOT = /Volumes/fennec/gecko-desktop
#GECKO_OBJ = $(GECKO_ROOT)/obj-x86_64-apple-darwin12.5.0

include .config
PLATFORM=Roku
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
