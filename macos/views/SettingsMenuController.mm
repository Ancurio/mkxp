//
//  SettingsMenuController.m
//  mkxp-z
//
//  Created by ゾロアーク on 1/15/21.
//

#import <SDL_scancode.h>
#import <SDL_keyboard.h>
#import <SDL_video.h>

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
    
    // Binding buttons
    __weak IBOutlet NSButton *bindingButton1;
    __weak IBOutlet NSButton *bindingButton2;
    __weak IBOutlet NSButton *bindingButton3;
    __weak IBOutlet NSButton *bindingButton4;
    
    
    // MKXP Keybindings
    BDescVec *binds;
    int currentButtonCode;
    
    // NSNumber (ButtonCode) -> NSArray (of BindingDesc pointers)
    NSMutableDictionary<NSNumber*, BindingIndexArray*> *nsbinds;
    NSMutableDictionary<NSNumber*, NSString*> *bindingNames;
}

+(SettingsMenu*)openWindow {
    SettingsMenu *s = [[SettingsMenu alloc] initWithNibName:@"settingsmenu" bundle:NSBundle.mainBundle];
    [NSApplication.sharedApplication.mainWindow beginSheet:s.view.window completionHandler:^(NSModalResponse _){}];
}

-(void)closeWindow {
    [_window close];
}

- (IBAction)acceptButton:(NSButton *)sender {
    shState->rtData().bindingUpdateMsg.post(*binds);
    storeBindings(*binds, shState->config());
}
- (IBAction)cancelButton:(NSButton *)sender {
    [self closeWindow];
}
- (IBAction)resetBindings:(NSButton *)sender {
    binds->clear();
    BDescVec tmp = genDefaultBindings(shState->config());
    binds->assign(tmp.begin(), tmp.end());
    
    [self loadBinds];
    [_table reloadData];
    if (currentButtonCode) [self setButtonNames:currentButtonCode];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self initDelegateWithTable:_table];
    _table.delegate = self;
    _table.dataSource = self;
    [_table reloadData];
}

- (void)keyDown:(NSEvent *)event {
    NSLog([NSString stringWithFormat:@"%d", event.keyCode]);
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
    int buttonCode = inputMapRowToCode[_table.selectedRow];
    currentButtonCode = buttonCode;
    
    [self setButtonNames:buttonCode];
}

- (int)setButtonNames:(int)input {
    BindingIndexArray *nsbind = nsbinds[@(input)];
    NSMutableArray<NSString*> *pnames = [NSMutableArray new];
    for (int i = 0; i < 4; i++) {
        if (i > nsbind.count - 1) {
            [pnames addObject:@"N/A"];
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
    bindingButton2.enabled = defaultSetting;
    
    bindingButton3.enabled = defaultSetting && currentBind.count > 1;
    bindingButton4.enabled = defaultSetting && currentBind.count > 2;
}

- (IBAction)binding1Clicked:(NSButton *)sender {
    [self removeBinding:0 forInput:currentButtonCode];
}
- (IBAction)binding2Clicked:(NSButton *)sender {
    [self removeBinding:1 forInput:currentButtonCode];
}
- (IBAction)binding3Clicked:(NSButton *)sender {
    [self removeBinding:2 forInput:currentButtonCode];
}
- (IBAction)binding4Clicked:(NSButton *)sender {
    [self removeBinding:3 forInput:currentButtonCode];
}

- (void)removeBinding:(int)bindIndex forInput:(int)input {
    NSMutableArray<NSNumber*> *bind = nsbinds[@(input)];
    int bi = bind[bindIndex].intValue;
    binds->erase(binds->begin() + bi);
    
    [self loadBinds];
    [_table reloadData];
    [self setButtonNames: input];
}

-(void)dealloc {
    delete binds;
}

@end
