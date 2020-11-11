#!/bin/sh

OSX_App="$pwd/mkxp.app"
ContentsDir="$OSX_App/Contents"
LibrariesDir="$OSX_App/Contents/Libraries"

# create directories in the @target@.app bundle
if [ ! -e $LibrariesDir ]
	then
	mkdir -p "$LibrariesDir"
fi
