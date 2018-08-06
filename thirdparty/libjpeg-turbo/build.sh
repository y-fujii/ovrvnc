#!/bin/sh


build() {
	mkdir "$1" &&
	cd "$1" &&
	cmake -G "Unix Makefiles" \
		"-DANDROID_ABI=$1" \
		"-DANDROID_NDK=$ANDROID_NDK_HOME" \
		"-DCMAKE_BUILD_TYPE=Release" \
		"-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK_HOME/build/cmake/android.toolchain.cmake" \
		"-DANDROID_NATIVE_API_LEVEL=28" \
		"../libjpeg-turbo" &&
	make &&
	cd ..
}

build arm64-v8a
build armeabi-v7a
