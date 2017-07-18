#!/bin/bash
# meson resolves paths such as processing assets/icon.png's command line
# ends up being `xxd -i ../assets/icon.png icon.png.xxd`, resulting in a
# prefix of `___` on all variable names. We postprocess xxd with sed for
# now in order to have proper variable names.
# ./xxd.sh /path/to/xxd /path/to/sed infile outfile

$1 -i $3 $3.tmp
$2 's:___::g' $3.tmp > $4
rm $3.tmp
