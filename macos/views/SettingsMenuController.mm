//
//  SettingsMenuController.m
//  mkxp-z
//
//  Created by ゾロアーク on 1/15/21.
//

// This is a pretty rudimentary keybinding menu, and it replaces the normal one
// for macOS. The normal one basically just doesn't seem to work with ANGLE,
// so I cooked this one up in a hurry despite knowing next to zero about Xcode's
// interface builder in general.

// Yes, it is still a mess, but it is working.

#import <GameController/GameController.h>

#import <SDL_scancode.h>
#import <SDL_keyboard.h>
#import <SDL_video.h>

#import "sdl_codes.h"

#import "SettingsMenuController.h"

#import "input/input.h"
#import "input/keybindings.h"

#import "eventthread.h"
#import "sharedstate.h"
#import "config.h"

#import "assert.h"

static const int inputMapRowToCode[] {
    Input::Down, Input::Left, Input::Right, Input::Up,
    Input::A, Input:: B, Input::ZL, Input::X, Input::Y, Input::ZR,
    Input::L, Input::R
};

typedef NSMutableArray<NSNumber*> BindingIndexArray;

@implementation SettingsMenu {
    __weak IBOutlet NSWindow *_window;
    __weak IBOutlet NSTableView *_table;
    __weak IBOutlet NSBox *bindingBox;
    
    // Binding buttons
    __weak IBOutlet NSButton *bindingButton1;
    __weak IBOutlet NSButton *bindingButton2;
    __weak IBOutlet NSButton *bindingButton3;
    __weak IBOutlet NSButton *bindingButton4;
    
    
    // MKXP Keybindings
    BDescVec *binds;
    int currentButtonCode;
    
    // Whether currently waiting for some kind of input
    bool isListening;
    
    // For the current binding selection when the table is
    // reloaded from deleting/adding keybinds
    bool keepCurrentButtonSelection;
    
    NSMutableDictionary<NSNumber*, BindingIndexArray*> *nsbinds;
    NSMutableDictionary<NSNumber*, NSString*> *bindingNames;
}

+(SettingsMenu*)openWindow {
    SettingsMenu *s = [[SettingsMenu alloc] initWithNibName:@"settingsmenu" bundle:NSBundle.mainBundle];
    // Show the window as a sheet, window events will be sucked up by SDL though
    //[NSApplication.sharedApplication.mainWindow beginSheet:s.view.window completionHandler:^(NSModalResponse _){}];
    
    // Show the view in a new window instead, so key and controller events
    // can be captured without SDL's interference
    NSWindow *win = [NSWindow windowWithContentViewController:s];
    win.styleMask &= ~NSWindowStyleMaskResizable;
    win.styleMask &= ~NSWindowStyleMaskFullScreen;
    win.styleMask &= NSWindowStyleMaskTitled;
    win.title = @"Keybindings";
    [s setWindow:win];
    [win orderFront:self];
    
    return s;
}

-(void)raise {
    if (_window)
        [_window orderFront:self];
}

-(void)closeWindow {
    [self setNotListening:true];
    [_window close];
}

-(SettingsMenu*)setWindow:(NSWindow*)window {
    _window = window;
}

- (IBAction)acceptButton:(NSButton *)sender {
    shState->rtData().bindingUpdateMsg.post(*binds);
    storeBindings(*binds, shState->config());
    [self closeWindow];
}
- (IBAction)cancelButton:(NSButton *)sender {
    [self closeWindow];
}
- (IBAction)resetBindings:(NSButton *)sender {
    binds->clear();
    BDescVec tmp = genDefaultBindings(shState->config());
    binds->assign(tmp.begin(), tmp.end());
    
    [self setNotListening:false];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    isListening = false;
    keepCurrentButtonSelection = false;
    
    [self initDelegateWithTable:_table];
    _table.delegate = self;
    _table.dataSource = self;
    [self setNotListening:true];
    _table.enabled = true;
    
    bindingBox.title = @"";
    [self setButtonNames:0];
}

- (void)keyDown:(NSEvent *)event {
    [super keyDown:event];
    if (!isListening) return;
    
    BindingDesc d;
    d.target = (Input::ButtonCode)currentButtonCode;
    SourceDesc s;
    s.type = Key;
    s.d.scan = darwin_scancode_table[event.keyCode];
    d.src = s;
    binds->push_back(d);
    [self setNotListening:true];
}

#define checkButtonStart if (0) {}
#define checkButtonEnd else { return; }
#define checkButtonElement(e, b, n) \
else if (element == gamepad.e && gamepad.b.isPressed) { \
s.type = JButton; \
s.d.jb = n; \
}

#define checkButtonAlt(b, n) checkButtonElement(b, b, n)

#define checkButton(b, n) checkButtonAlt(button##b, n)

#define setAxisData(a, n) \
GCControllerAxisInput *axis = gamepad.a; \
s.type = JAxis; \
s.d.ja.axis = n; \
s.d.ja.dir = (axis.value >= 0) ? AxisDir::Positive : AxisDir::Negative;

#define checkAxis(el, a, n) else if (element == gamepad.el && (gamepad.el.a.value >= 0.5 || gamepad.el.a.value <= -0.5)) { setAxisData(el.a, n); }

- (void)registerJoystickAction:(GCExtendedGamepad*)gamepad element:(GCControllerElement*)element {
    if (!isListening) return;
    
    BindingDesc d;
    d.target = (Input::ButtonCode)currentButtonCode;
    SourceDesc s;
    
    checkButtonStart
    checkButton(A, 0)
    checkButton(B, 1)
    checkButton(X, 2)
    checkButton(Y, 3)
    checkButtonElement(dpad, dpad.up, 11)
    checkButtonElement(dpad, dpad.down, 12)
    checkButtonElement(dpad, dpad.left, 13)
    checkButtonElement(dpad, dpad.right, 14)
    checkButtonAlt(leftShoulder, 9)
    checkButtonAlt(rightShoulder, 10)
    checkButtonAlt(leftThumbstickButton, 7)
    checkButtonAlt(rightThumbstickButton, 8)
    checkButton(Home, 5)
    checkButton(Menu, 6)
    checkButton(Options, 4)
    
    checkAxis(leftThumbstick, xAxis, 0)
    checkAxis(leftThumbstick, yAxis, 1)
    checkAxis(rightThumbstick, xAxis, 2)
    checkAxis(rightThumbstick, yAxis, 3)
    
    else if (element == gamepad.leftTrigger && (gamepad.leftTrigger.value >= 0.5 || gamepad.leftTrigger.value <= -0.5)) {
        GCControllerButtonInput *trigger = gamepad.leftTrigger;
        s.type = JAxis;
        s.d.ja.axis = 4;
        s.d.ja.dir = AxisDir::Positive;
    }
    
    else if (element == gamepad.rightTrigger && (gamepad.rightTrigger.value >= 0.5 || gamepad.rightTrigger.value <= -0.5)) {
        GCControllerButtonInput *trigger = gamepad.rightTrigger;
        s.type = JAxis;
        s.d.ja.axis = 5;
        s.d.ja.dir = AxisDir::Positive;
    }
    
    checkButtonEnd;
    
    d.src = s;
    binds->push_back(d);
    [self setNotListening:true];
}

+(NSString*)nameForBinding:(SourceDesc&)desc {
    switch (desc.type) {
        case Key:
            if (desc.d.scan == SDL_SCANCODE_LSHIFT) return @"Shift";
            return @(SDL_GetScancodeName(desc.d.scan));
        case JHat:
            const char *dir;
            switch (desc.d.jh.pos) {
                case SDL_HAT_UP:
                    dir = "Up";
                    break;
                case SDL_HAT_DOWN:
                    dir = "Down";
                    break;
                case SDL_HAT_LEFT:
                    dir = "Left";
                    break;
                case SDL_HAT_RIGHT:
                    dir = "Right";
                    break;
                default:
                    dir = "-";
                    break;
            }
            return [NSString stringWithFormat:@"JS Hat %d:%s", desc.d.jh.hat, dir];
        case JAxis:
            return [NSString stringWithFormat:@"JS Axis %d%s", desc.d.ja.axis, desc.d.ja.dir == Negative ? "-" : "+"];
        case JButton:
            return [NSString stringWithFormat:@"JS Button %i", desc.d.jb];
        default:
            return @"Invalid";
    }
}

-(id)initDelegateWithTable:(NSTableView*)tbl {
    binds = new BDescVec;
    nsbinds = [NSMutableDictionary new];
    bindingNames = [NSMutableDictionary new];
    
    RGSSThreadData &data = shState->rtData();
    
#define SET_BINDING(code) bindingNames[@(Input::code)] = @(#code)
#define SET_BINDING_CUSTOM(code, value) bindingNames[@(Input::code)] = @(value)
    SET_BINDING(Down);
    SET_BINDING(Left);
    SET_BINDING(Right);
    SET_BINDING(Up);
    SET_BINDING(A);
    SET_BINDING(B);
    SET_BINDING_CUSTOM(ZL, "C");
    SET_BINDING(X);
    SET_BINDING(Y);
    SET_BINDING_CUSTOM(ZR, "Z");
    SET_BINDING(L);
    SET_BINDING(R);
    
#define SET_BINDING_CONF(code, value) \
if (!data.config.kbActionNames.value.empty()) bindingNames[@(Input::code)] = \
    @(data.config.kbActionNames.value.c_str())
    SET_BINDING_CONF(A,a);
    SET_BINDING_CONF(B,b);
    SET_BINDING_CONF(ZL,c);
    SET_BINDING_CONF(X,x);
    SET_BINDING_CONF(Y,y);
    SET_BINDING_CONF(ZR,z);
    SET_BINDING_CONF(L,l);
    SET_BINDING_CONF(R,r);
    
    BDescVec oldBinds;
    data.bindingUpdateMsg.get(oldBinds);
    
    if (oldBinds.size() <= 0) {
        BDescVec defaults = genDefaultBindings(data.config);
        binds->assign(defaults.begin(), defaults.end());
    }
    else {
        binds->assign(oldBinds.begin(), oldBinds.end());
    }
    
    [self loadBinds];
    [self enableButtons:false];
    
    return self;
}

- (void) loadBinds {
    [nsbinds removeAllObjects];
    
    for (int i = 0; i < sizeof(inputMapRowToCode) / sizeof(Input::ButtonCode); i++)
        nsbinds[@(inputMapRowToCode[i])] = [NSMutableArray new];
    
    for (int i = 0; i < binds->size(); i++) {
        NSNumber *key = @(binds->at(i).target);
        if (nsbinds[key] == nil) {
            nsbinds[key] = [NSMutableArray new];
        }
        NSMutableArray *b = nsbinds[key];
        [b addObject:@(i)];
    }
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return nsbinds.count;
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString *tableID = tableColumn.identifier;
    int buttonCode = inputMapRowToCode[row];
    
    NSTableCellView *cell = [tableView makeViewWithIdentifier:tableID owner:self];
    if ([tableID isEqualToString:@"action"]) {
        cell.textField.stringValue = bindingNames[@(inputMapRowToCode[row])];
        cell.textField.font = [NSFont boldSystemFontOfSize:cell.textField.font.pointSize];
        return cell;
    }
    
    int col = tableID.integerValue;
    
    if (nsbinds[@(buttonCode)].count < col) {
        cell.textField.stringValue = @"";
        return cell;
    }
    
    NSNumber *d = nsbinds[@(buttonCode)][col-1];
    BindingDesc &bind = binds->at(d.intValue);
    cell.textField.stringValue = [SettingsMenu nameForBinding:bind.src];
    return cell;
}

- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    NSString *tableID = tableColumn.identifier;
    int buttonCode = inputMapRowToCode[row];
    
    if ([tableID isEqualToString:@"action"]) {
        return @(buttonCode);
    }
    
    int col = tableID.integerValue;
    
    if (nsbinds[@(buttonCode)].count < col) {
        return 0;
    }
    
    NSNumber *d = nsbinds[@(buttonCode)][col-1];
    BindingDesc &bind = binds->at(d.intValue);
    return @(bind.src.d.scan);
}

- (void)tableViewSelectionDidChange:(NSNotification *)notification {
    if (isListening)
        isListening = false;
    
    if (keepCurrentButtonSelection) {
        if (keepCurrentButtonSelection) keepCurrentButtonSelection = false;
        [_table deselectRow:_table.selectedRow];
        [self setButtonNames:currentButtonCode];
        bindingBox.title = bindingNames[@(currentButtonCode)];
        return;
    }
    
    if (_table.selectedRow > -1) {
        int buttonCode = inputMapRowToCode[_table.selectedRow];
        currentButtonCode = buttonCode;
        [self setButtonNames:buttonCode];
        bindingBox.title = bindingNames[@(currentButtonCode)];
    }
    else {
        [self setButtonNames:0];
        bindingBox.title = @"";
    }
}

- (int)setButtonNames:(int)input {
    if (!input) {
        bindingButton1.title = @"";
        bindingButton2.title = bindingButton1.title;
        bindingButton3.title = bindingButton1.title;
        bindingButton4.title = bindingButton1.title;
        [self enableButtons:false];
        return 0;
    }
    BindingIndexArray *nsbind = nsbinds[@(input)];
    NSMutableArray<NSString*> *pnames = [NSMutableArray new];
    for (int i = 0; i < 4; i++) {
        if (!nsbind.count || i > nsbind.count - 1) {
            [pnames addObject:@"(Empty)"];
        }
        else {
            BindingDesc &b = binds->at(nsbind[i].intValue);
            NSString *bindingName = [SettingsMenu nameForBinding:b.src];
            [pnames addObject:bindingName];
        }
    }
    
    bindingButton1.title = pnames[0];
    bindingButton2.title = pnames[1];
    bindingButton3.title = pnames[2];
    bindingButton4.title = pnames[3];
    [self enableButtons:true];
    
    return pnames.count;
}

- (void)enableButtons:(bool)defaultSetting {
    BindingIndexArray *currentBind = nsbinds[@(currentButtonCode)];
    bindingButton1.enabled = defaultSetting;
    bindingButton2.enabled = defaultSetting && currentBind.count > 0;
    
    bindingButton3.enabled = defaultSetting && currentBind.count > 1;
    bindingButton4.enabled = defaultSetting && currentBind.count > 2;
}

- (IBAction)binding1Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 0) {
        [self removeBinding:0 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding2Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 1) {
        [self removeBinding:1 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding3Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 2) {
        [self removeBinding:2 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}
- (IBAction)binding4Clicked:(NSButton *)sender {
    if (nsbinds[@(currentButtonCode)].count > 3) {
        [self removeBinding:3 forInput:currentButtonCode];
        return;
    }
    [self setListening:sender];
}

- (void)removeBinding:(int)bindIndex forInput:(int)input {
    NSMutableArray<NSNumber*> *bind = nsbinds[@(input)];
    int bi = bind[bindIndex].intValue;
    binds->erase(binds->begin() + bi);
    
    [self setNotListening:true];
}

- (void)setListening:(NSButton*)src {
    if (isListening) {
        [self setNotListening:true];
        return;
    }
    
    // Stops receiving keyDown events if it's disabled, apparently
    //_table.enabled = false;
    
    [self enableButtons:false];
    
    if (src == nil) return;
    
    src.title = @"Click to Cancel...";
    src.enabled = true;
    isListening = true;
    
    NSArray<GCController*>* controllers = [GCController controllers];
    if (controllers.count <= 0) return;
    GCController *gamepad = controllers[0];
    if (gamepad.extendedGamepad == nil || gamepad.extendedGamepad.valueChangedHandler != nil) return;
    gamepad.extendedGamepad.valueChangedHandler = ^(GCExtendedGamepad *gamepad, GCControllerElement *element)
    {[self registerJoystickAction:gamepad element:element];};
}

- (void)setNotListening:(bool)keepCurrentSelection {
    [self loadBinds];
    
    keepCurrentButtonSelection = keepCurrentSelection;
    isListening = false;
    [_table reloadData];
    [self setButtonNames:currentButtonCode];
    
}

-(void)dealloc {
    delete binds;
}

@end
