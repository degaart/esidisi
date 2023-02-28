#include <Cocoa/Cocoa.h>
#include <objc/NSObjCRuntime.h>

@interface TreeItem: NSObject {
    NSString* label;
    NSMutableArray* children;
}
@end

@implementation TreeItem

- (id)initWithLabel:(NSString*)aLabel {
    if(self = [super init]) {
        label = [aLabel retain];
        children = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)dealloc {
    [children release];
    [label release];
    [super dealloc];
}

- (TreeItem*)addChild:(NSString*)aLabel {
    TreeItem* item = [[TreeItem alloc] initWithLabel:aLabel];
    [children addObject:item];
    return item;
}

- (TreeItem*)childAtIndex:(NSUInteger)index {
    return [children objectAtIndex:index];
}

- (NSUInteger)childCount {
    return [children count];
}

- (NSString*)label {
    return self->label;
}

@end

@interface TreeViewDataSource: NSObject<NSOutlineViewDataSource> {
    TreeItem* root;
}
@end

@implementation TreeViewDataSource

- (id)init {
    if(self = [super init]) {
        root = [[TreeItem alloc] initWithLabel:@"Root"];

        TreeItem* child = [root addChild:@"Item 1"];
        [child addChild:@"Item 1a"];
        [child addChild:@"Item 1b"];
        [child addChild:@"Item 1c"];

        child = [root addChild:@"Item 2"];
        [child addChild:@"Item 2a"];
        [child addChild:@"Item 2b"];
        [child addChild:@"Item 2c"];

        child = [root addChild:@"Item 3"];
        [child addChild:@"Item 3a"];
        [child addChild:@"Item 3b"];
        [child addChild:@"Item 3c"];
    }
    return self;
}

- (void)dealloc {
    [root release];
    [super dealloc];
}

- (id)outlineView:(NSOutlineView*)view child:(NSInteger)child ofItem:(id)item {
    if(!item) {
        return [root childAtIndex:child];
    } else {
        return [item childAtIndex:child];
    }
}

- (BOOL)outlineView:(NSOutlineView*)view isItemExpandable:(id)item {
    if(!item) {
        return [root childCount] > 0;
    } else {
        return [item childCount] > 0;
    }
}

- (NSInteger)outlineView:(NSOutlineView*)view numberOfChildrenOfItem:(id)item {
    if(!item) {
        return [root childCount];
    } else {
        return [item childCount];
    }
}

- (id)outlineView:(NSOutlineView*)view objectValueForTableColumn:(NSTableColumn*)column byItem:(id)item {
    if(!item) {
        return [root label];
    } else {
        return [item label];
    }
}

@end

@interface TableViewDataSource: NSObject<NSTableViewDataSource> {
}
@end

@implementation TableViewDataSource

- (id)init {
    if(self = [super init]) {
    }
    return self;
}

- (void)dealloc {
    [super dealloc];
}

- (NSInteger)numberOfRowsInTableView:(id)sender {
    return 4;
}

- (id)tableView:(NSTableView*)tableView objectValueForTableColumn:(NSTableColumn*)col row:(NSInteger)row {
    return [NSString stringWithFormat:@"col %@ item #%d", col, (int)row];
}

@end

@interface AppDelegate: NSObject {
    NSWindow* window;
    TreeViewDataSource* treeDataSource;
    TableViewDataSource* tableDataSource;
}
@end

@implementation AppDelegate

- (id)init {
    if(self = [super init]) {
        window = [self initWindow];
    }
    return self;
}

- (void)dealloc {
    [window release];
    [tableDataSource release];
    [treeDataSource release];
    [super dealloc];
}

- (NSWindow*)initWindow {
    NSRect frame = NSMakeRect(0, 0, 640, 480);
    NSWindow* win = [[[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable
                    backing:NSBackingStoreBuffered
                      defer:YES] autorelease];
    NSSplitView* splitView = [[[NSSplitView alloc] initWithFrame:frame] autorelease];
    [splitView setVertical:YES];
    [splitView setDividerStyle:NSSplitViewDividerStylePaneSplitter];

    treeDataSource = [[TreeViewDataSource alloc] init];
    NSOutlineView* treeView = [[[NSOutlineView alloc] init] autorelease];

    NSTableColumn* col = [[[NSTableColumn alloc] initWithIdentifier:@"col1"] autorelease];
    [col setResizingMask:NSTableColumnAutoresizingMask];

    [treeView addTableColumn:col];
    [treeView setStronglyReferencesItems:YES];
    [treeView setDataSource:treeDataSource];
    [splitView addSubview:treeView];

    NSTableView* tableView = [[[NSTableView alloc] init] autorelease];
    tableDataSource = [[TableViewDataSource alloc] init];
    [tableView setDataSource:tableDataSource];

    col = [[[NSTableColumn alloc] initWithIdentifier:@"col1"] autorelease];
    [col setTitle:@"Column 1"];
    [col setEditable:YES];
    [tableView addTableColumn:col];

    col = [[[NSTableColumn alloc] initWithIdentifier:@"col2"] autorelease];
    [col setTitle:@"Column 2"];
    [tableView addTableColumn:col];

    NSTableHeaderView* headerView = [[[NSTableHeaderView alloc] init] autorelease];
    [tableView setHeaderView:headerView];

    [splitView addSubview:tableView];

    [win setContentView:splitView];
    return win;
}

- (void)applicationDidFinishLaunching:(id)sender {
    [self populateMainMenu];
    [window makeKeyAndOrderFront:self];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(id)sender {
    return YES;
}

- (void)populateMainMenu {
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@"MainMenu"];
    NSMenuItem* menuItem = [mainMenu addItemWithTitle:@"Apple"
                                               action:NULL
                                        keyEquivalent:@""];
    NSMenu* subMenu = [[NSMenu alloc] initWithTitle:@"Apple"];
    [NSApp performSelector:@selector(setAppleMenu:) withObject:subMenu];
    NSMenuItem* appMenuItem = [subMenu addItemWithTitle:@"About DBATool"
                                                 action:@selector(orderFrontStandardAboutPanel:)
                                          keyEquivalent:@""];
    [appMenuItem setTarget:NSApp];
    [subMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* quitMenuItem = [subMenu addItemWithTitle:@"Quit"
                                                  action:@selector(terminate:)
                                           keyEquivalent:@"q"];
    [quitMenuItem setTarget:NSApp];

    [mainMenu setSubmenu:subMenu
                 forItem:menuItem];
    [NSApp setMainMenu:mainMenu];
}

@end

int main() {
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    [NSApplication sharedApplication];

    id appDelegate = [[[AppDelegate alloc] init] autorelease];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp setDelegate:appDelegate];
    [NSApp run];

    [pool drain];
    return 0;
}

