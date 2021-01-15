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

@implementation SettingsMenu {
    __weak IBOutlet NSWindow *_window;
    
    SettingsMenuDelegate *_sdelegate;
    __weak IBOutlet NSTableView *_table;
}

+(SettingsMenu*)openWindow {
    SettingsMenu *s = [[SettingsMenu alloc] initWithNibName:@"settingsmenu" bundle:NSBundle.mainBundle];
    [NSApplication.sharedApplication.mainWindow beginSheet:s.view.window completionHandler:^(NSModalResponse _){}];
}

-(void)closeWindow {
    [_window close];
}

- (IBAction)acceptButton:(NSButton *)sender {
}
- (IBAction)cancelButton:(NSButton *)sender {
    [self closeWindow];
}
- (IBAction)resetBindings:(NSButton *)sender {
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    _sdelegate = [[SettingsMenuDelegate alloc] initWithThreadData:shState->rtData()];
    _table.delegate = _sdelegate;
    _table.dataSource = _sdelegate;
    [_table reloadData];
}

@end

@implementation SettingsMenuDelegate {
    BDescVec *binds;
    
    // NSNumber (ButtonCode) -> NSArray (of BindingDesc pointers)
    NSMutableDictionary<NSNumber*, NSArray<NSNumber*>*> *nsbinds;
    NSMutableDictionary<NSNumber*, NSString*> *bindingNames;
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
            return [NSString stringWithFormat:@"JoyHat %d:%s", desc.d.jh.hat, dir];
        case JAxis:
            return [NSString stringWithFormat:@"JS Axis %d%s", desc.d.ja.axis, desc.d.ja.dir == Negative ? "-" : "+"];
        case JButton:
            return [NSString stringWithFormat:@"JS Button %i", desc.d.jb];
        default:
            return @"Invalid";
    }
}

-(id)initWithThreadData:(RGSSThreadData&)data {
    self = [super init];
    binds = new BDescVec;
    
    BDescVec oldBinds;
    data.bindingUpdateMsg.get(oldBinds);
    
    if (oldBinds.size() <= 0) {
        BDescVec defaults = genDefaultBindings(data.config);
        binds->assign(defaults.begin(), defaults.end());
        return self;
    }
    
    binds->assign(oldBinds.begin(), oldBinds.end());
    
    nsbinds = [NSMutableDictionary new];
    
    for (int i = 0; i < binds->size(); i++) {
        NSNumber *key = @(binds->at(i).target);
        if (nsbinds[key] == nil) nsbinds[key] = [NSMutableArray new];
        NSMutableArray *b = nsbinds[key];
        [b addObject:@(i)];
    }
    
    bindingNames = [NSMutableDictionary new];
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
    return self;
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
    cell.textField.stringValue = [SettingsMenuDelegate nameForBinding:bind.src];
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

-(void)dealloc {
    delete binds;
}

@end
