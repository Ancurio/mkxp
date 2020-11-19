CONFIG := Config.xcconfig
XCARGS   := -xcconfig $(CONFIG) -scheme Player

debug: deps
	xcodebuild -configuration Debug $(XCARGS)

release: deps
	xcodebuild -configuration Release $(XCARGS)

deps:
	@make -f Deps.make

clean:
	@make -f Deps.make clean
	rm -rf "DerivedData"