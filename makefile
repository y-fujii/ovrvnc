.PHONY: test clean

test:
	cd android && \
	gradle installRelease && \
	adb push ../ovrvnc.toml /sdcard/ && \
	adb shell am start -n net.mimosa_pudica.ovrvnc/.MainActivity

clean:
	cd android && gradle clean

tags:
	ctags -R --extra=+q . $(OCULUS_SDK_PATH)
