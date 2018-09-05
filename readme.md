# ovrvnc - VNC Viewer for Oculus Go

ovrvnc is a simple VNC client for Oculus Go.

- Performance
	- Supporting Tight encoding (with optimal parameters) and Continuous
	  updates extensions enable us to watch HD video.
- Latency
	- Decoding, rendering and sending input events are all asynchronous.
- Visual
	- Use cylindrical layers with optimal resolution, sRGB gamma-corrected
	  interpolation, chromatic aberration correction and 72Hz refresh rate.
- Support multiple connection.
- Customize with configuration file.

Many features above are provided by the thirdparty libraries:
[TigerVNC](http://tigervnc.org/), [libjpeg-turbo](https://libjpeg-turbo.org/),
[cpptoml](https://github.com/skystrife/cpptoml), etc. Thanks!

## Install prebuilt binary

Currently ovrvnc is in pre-alpha stage.

Download [ovrvnc-release.apk](http://mimosa-pudica.net/tmp/ovrvnc-release.apk).

	$ adb uninstall net.mimosa_pudica.ovrvnc
	$ adb install ovrvnc-release.apk
	$ vi ovrvnc.toml # configure host, password, etc.  See below.
	$ adb push ovrvnc.toml /sdcard/

## Configuration

Minimal example of `/sdcard/ovrvnc.toml`:

	[[screens]]
	host = "hogehoge"

More complex example:

	[background]
	#color = [0.0, 0.0, 0.0]
	image = "/sdcard/Pictures/equirect.jpg"

	[[screens]]
	host = "192.168.179.5"
	#port = 5900
	#password = ""
	latitude  = -15.0
	#longitude = 0.0
	#lossy = true
	use_pointer = false

	[[screens]]
	host = "192.168.179.4"
	password = "hogehoge"
	latitude  = -15.0
	longitude = 180.0

## Performance

Although RFB protocol is old, I think it is still a simple and good protocol.
With Tight encoding and Continuous update extensions, you can watch the HD
video through the VNC without any hardware encoder.

The latency and throughput of ovrvnc heavily depends on a server program and
its configuration.  In many cases, the bottleneck is a CPU, not a network.

**For Xorg users**: x0vncserver >= 1.9 bundled in TigerVNC with following
arguments is recommended.

	x0vncserver --comparefb=1 --maxprocessorusage=100

**For Windows users**: I don't know whether TightVNC (!= Tig*er*VNC, it seems
broken on Windows) or UltraVNC is better.  Note that both of them do not seem
to support continuous updates thus the performance is not optimal.  When using
UltraVNC, "Poll Full Screen" and "Desktop Duplication" options improves the
performance and the stability (I don't know it is the best configuration).

See also [mfxvnc](http://github.com/y-fujii/mfxvnc/).

## Build

XXX: Apply patches.

	export     ANDROID_HOME=...
	export ANDROID_NDK_HOME=...
	export  OCULUS_SDK_PATH=...

	cd thirdparty/libjpeg-turbo
	./build.sh
	cd ../..
	make

## License

The code in this repository except submodules in `thirdparty/` is distributed
under the MIT license.

ovrvnc uses TigerVNC as library and it is distributed under the GPL.  The other
libraries (libjpeg-turbo, cpptoml) are under the MIT/BSD-style license.
