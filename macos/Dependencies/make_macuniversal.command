#!/usr/bin/ruby

# Simple script that takes all libraries in the build directories
# and dumps out fat binaries to use in xcode

# Will probably have to extend it a bit to do iOS if I ever get that
# working

require 'FileUtils'

BUILD_ARM = File.join(Dir.pwd, "build-macosx-arm64", "lib")
BUILD_X86 = File.join(Dir.pwd, "build-macosx-x86_64", "lib")

DESTINATION = File.join(Dir.pwd, "build-macosx-universal", "lib")


armfiles = Dir[File.join(BUILD_ARM, "*.{a,dylib}")]
x86files = Dir[File.join(BUILD_X86, "*.{a,dylib}")]

basenames = {
    :a => armfiles.map{|f| File.basename(f)},
    :b => x86files.map{|f| File.basename(f)}
}

def lipo(a, b)
    FileUtils.mkdir_p(DESTINATION) if !Dir.exists?(DESTINATION)
    if `lipo -info #{a}`[/is architecture: arm64/] && `lipo -info #{b}`[/is architecture: x86_64/]
        `lipo -create #{a} #{b} -o #{File.join(DESTINATION, File.basename(a))}`
    end
end

armfiles.length.times{|i|
    if basenames[:b].include?(basenames[:a][i])
        linkedfile = x86files[basenames[:b].index(basenames[:a][i])]

        lipo(armfiles[i], linkedfile)
    end
}