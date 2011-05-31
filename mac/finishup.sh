#!/bin/sh
PLATFORM=`uname -s`

if [ $PLATFORM = "Darwin" ]
then
    mkdir -p "Dynamic Windows Interface Builder.app/Contents/MacOS"
    mkdir -p "Dynamic Windows Interface Builder.app/Contents/Resources"

    cp -f mac/Info.plist "Dynamic Windows Interface Builder.app/Contents"
    cp -f mac/PkgInfo "Dynamic Windows Interface Builder.app/Contents"
    cp -f mac/*.png "Dynamic Windows Interface Builder.app/Contents/Resources"
    cp -f mac/*.icns "Dynamic Windows Interface Builder.app/Contents/Resources"
    cp -f dwib "Dynamic Windows Interface Builder.app/Contents/MacOS"
fi
