#!/bin/sh

# Build
ant debug
[ $? -ne 0 ] && echo "Build failed" && exit 1

# Load
adb install -r bin/qr_app-debug.apk
[ $? -ne 0 ] && echo "Load failed" && exit 1

# Launch
adb shell am start -n nz.co.dcollins.qr_app/nz.co.dcollins.qr_app.MainActivity
[ $? -ne 0 ] && echo "Launch failed" && exit 1
