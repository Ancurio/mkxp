#!/bin/bash 
# Author : Hemanth.HM
# Email : hemanth[dot]hm[at]gmail[dot]com
# License : GNU GPLv3
#

function useage()
{
    cat << EOU
Useage: bash $0 <path to the binary> <path to copy the dependencies>
EOU
exit 1
}

INSTALLDIR=${MESON_INSTALL_PREFIX}/usr/lib

#Validate the inputs
[[ $# < 2 ]] && useage

#Check if the paths are vaild
[[ ! -e $1 ]] && echo "Not a vaild input $1" && exit 1 
[[ -d $2 ]] || echo "No such directory $INSTALLDIR creating..."&& mkdir -p "$INSTALLDIR"

#Get the library dependencies
echo "Collecting the shared library dependencies for $1..."
deps=$(ldd $1 | awk 'BEGIN{ORS=" "}$1\
~/^\//{print $1}$3~/^\//{print $3}'\
 | sed 's/,$/\n/')
echo "Copying the dependencies to $INSTALLDIR"

#Copy the deps
for dep in $deps
do
    echo "Copying $dep to $INSTALLDIR"
    cp "$dep" "$INSTALLDIR"
done

cp ${MESON_INSTALL_PREFIX}/share/mkxp-z/* ${MESON_INSTALL_PREFIX}
rm -rf ${MESON_INSTALL_PREFIX}/share
$2 ${MESON_INSTALL_PREFIX}
