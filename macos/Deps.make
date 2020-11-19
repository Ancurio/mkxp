GOODLS_URL := https://github.com/tanaikech/goodls/releases/download/v1.2.7/goodls_darwin_amd64
DMG_URL := https://drive.google.com/file/d/1-9Pt5bxvFUuO_yPFzC_h3WvQ9LDA6Q5K/view?usp=sharing
DMG_NAME := Dependencies
GOODLS := gls

DEP_FOLDER := MKXPZ-Dependencies

deps: $(DMG_NAME).dmg
	hdiutil attach "$(DMG_NAME).dmg" -mountroot . -quiet

$(DMG_NAME).dmg: $(GOODLS)
	"./$(GOODLS)" -u "$(DMG_URL)" -f "$(DMG_NAME).dmg"

$(GOODLS):
	curl -L "$(GOODLS_URL)" > "$(GOODLS)"
	chmod +x "$(GOODLS)"

unmount:
	-hdiutil detach $(DEP_FOLDER) -quiet

clean: unmount
	rm -rf "$(GOODLS)" "$(DMG_NAME).dmg"

