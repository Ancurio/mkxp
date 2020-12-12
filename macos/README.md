This folder contains an experimental Xcode project that will build mkxp-z and all of its dependencies.

Currently, building mkxp-z for ARM requires an ARM Mac on hand.
This is primarily because libsigc++ doesn't properly cross-compile when you ask it to.

To get all dependencies, cd to the `Dependencies` folder and run:

```sh
# This will download all the required tools
# for building the dependencies
brew bundle

# Intel Macs
make everything -f .Intel

# ARM Macs
make everything -f .AppleSilicon

# Create universal libraries (For the universal build)
make everything -f .AppleSilicon
arch -x86_64 make everything -f .Intel
./make_macuniversal.command

# Use your own Ruby, and build everything else (for Intel Macs)
make configure-ruby -f .Intel RUBY_PATH="Path to Ruby" RUBY_FLAGS="extra configure arguments" 
make deps-core -f .Intel
```

Afterwards, simply open the Xcode project, select the scheme you'd like to use
and hit Command+B.

If you built a version of ruby >= 2, change the MRI_VERSION build setting to match.

If you built 1.8, select the PlayerLegacy scheme.