cmake_minimum_required(VERSION 3.10)
project(phone C)

set(CMAKE_C_STANDARD 11)

include_directories(src/)

add_executable(phone app.c src/phone.h src/config.h src/utils.c src/utils.h src/init.c src/init.h src/callback.c src/callback.h src/rtp.c src/rtp.h src/sip.c src/sip.h src/logger.c src/logger.h)

target_link_libraries(phone -lpjsip-ua-x86_64-unknown-linux-gnu -lpjsip-simple-x86_64-unknown-linux-gnu -lpjsip-x86_64-unknown-linux-gnu -lpjmedia-codec-x86_64-unknown-linux-gnu -lpjmedia-x86_64-unknown-linux-gnu -lpjmedia-videodev-x86_64-unknown-linux-gnu -lpjmedia-audiodev-x86_64-unknown-linux-gnu -lpjmedia-x86_64-unknown-linux-gnu -lpjnath-x86_64-unknown-linux-gnu -lpjlib-util-x86_64-unknown-linux-gnu  -lsrtp-x86_64-unknown-linux-gnu -lresample-x86_64-unknown-linux-gnu -lgsmcodec-x86_64-unknown-linux-gnu -lspeex-x86_64-unknown-linux-gnu -lilbccodec-x86_64-unknown-linux-gnu -lg7221codec-x86_64-unknown-linux-gnu -lyuv-x86_64-unknown-linux-gnu -lwebrtc-x86_64-unknown-linux-gnu  -lpj-x86_64-unknown-linux-gnu -lm -lrt -lpthread  -lasound)
