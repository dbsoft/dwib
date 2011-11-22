#!/bin/sh
PLATFORM=`uname -s`
APPNAME=$1
BINNAME=$2

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p "$APPNAME.app/Contents/MacOS"
    mkdir -p "$APPNAME.app/Contents/Resources"

    cp -f mac/Info.plist "$APPNAME.app/Contents"
    cp -f mac/PkgInfo "$APPNAME.app/Contents"
    cp -f mac/*.png "$APPNAME.app/Contents/Resources"
    cp -f mac/*.icns "$APPNAME.app/Contents/Resources"
    cp -f $BINNAME "$APPNAME.app/Contents/MacOS"
fi
