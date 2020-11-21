This folder contains an experimental Xcode project that will build mkxp-z and all of its dependencies.

To get all dependencies, run:

```sh
# From the project's root;
# This will download all the required tools
# for building the dependencies
cd macos/Dependencies
brew bundle install

# Intel Macs
make -f .Intel everything

# Apple Silicon Macs
make -f .AppleSilicon everything

# Make individual targets
make -f .Intel ruby sdl2 objfw 

# Use your own Ruby, and build everything else (for Intel Macs)
make -f .Intel configure-ruby RUBY_PATH="Path to Ruby" RUBY_ARGS="extra configure arguments" 
make -f .Intel deps-core
```

Afterwards, simply open the Xcode project and hit Command+B.

If you built a version of ruby >= 2, change the MRI_VERSION build setting to match.

If you built 1.8 or 1.9, you will probably need to redefine the header+library search paths.