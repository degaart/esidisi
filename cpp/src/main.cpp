#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <stdio.h>
#include <cassert>

extern "C" void NSLog(objc_object *, ...);
extern id *NSApp;
#define NSApplicationActivationPolicyRegular 0
#define NSWindowStyleMaskTitled (1 << 0)
#define NSBackingStoreBuffered 2
#define NSSplitViewDividerStylePaneSplitter 3
#define NSTableColumnAutoresizingMask (1 << 0)
#define NSTableColumnUserResizingMask ( 1 << 1 )

template<typename T, typename... Args>
id msgsend(T* obj, const char* sel, Args... args) {
    using fntype = id(*)(T*,SEL,decltype(args)...);
    return ((fntype)objc_msgSend)(obj, sel_getUid(sel), args...);
}

template<typename T>
id msgsend(T obj, const char* sel) {
    using fntype = id (*)(T,SEL);
    return ((fntype)objc_msgSend)(obj, sel_getUid(sel));
}

template<typename T, typename... Args>
id msgsendsuper(T obj, const char* sel, Args... args) {
    using fntype = id(*)(objc_super*,SEL,decltype(args)...);

    objc_super super;
    super.receiver = obj;
    super.super_class = class_getSuperclass(object_getClass(obj));
    return ((fntype)objc_msgSendSuper)(&super, sel_getUid(sel), args...);
}

template<typename T>
id msgsendsuper(T obj, const char* sel) {
    using fntype = id(*)(objc_super*, SEL);

    objc_super super;
    super.receiver = obj;
    super.super_class = class_getSuperclass(object_getClass(obj));
    return ((fntype)objc_msgSendSuper)(&super, sel_getUid(sel));
}

id alloc(const char* cls) {
    return msgsend(objc_getClass(cls), "alloc");
}

id getivar(id obj, const char* name) {
    Ivar var = class_getInstanceVariable(object_getClass(obj), name);
    assert(var != nullptr);
    return object_getIvar(obj, var);
}

void setivar(id obj, const char* name, id value) {
    Ivar ivar = class_getInstanceVariable(object_getClass(obj), name);
    assert(ivar != nullptr);
    object_setIvar(obj, ivar, value);
}

template<typename T>
T getivarp(id obj, const char* name) {
    Ivar ivar = class_getInstanceVariable(object_getClass(obj), name);
    assert(ivar != nullptr);

    auto offset = ivar_getOffset(ivar);
    return  *((const T*)((const char*)obj + offset));
}

template<typename T>
void setivarp(id obj, const char* name, T value) {
    Ivar ivar = class_getInstanceVariable(object_getClass(obj), name);
    assert(ivar != nullptr);

    auto offset = ivar_getOffset(ivar);
    *((T*)((char*)obj + offset)) = value;
}

#define STR(s) \
    msgsend(objc_getClass("NSString"), "stringWithUTF8String:", s)

void trace(const char* msg) {
    NSLog(STR(msg));
}

static
id initWindow(id self) {
    auto frame = CGRectMake(0.0, 0.0, 640.0, 480.0);
    auto window = msgsend(
        alloc("NSWindow"),
        "initWithContentRect:styleMask:backing:defer:",
        frame,
        NSWindowStyleMaskTitled,
        NSBackingStoreBuffered,
        YES);

    id splitView = msgsend(
        msgsend(
            alloc("NSSplitView"),
            "initWithFrame:",
            frame),
        "autorelease");
    msgsend(splitView, "setVertical:", YES);
    msgsend(splitView,
            "setDividerStyle:",
            NSSplitViewDividerStylePaneSplitter);

    id treeDataSource = msgsend(
            alloc("TreeViewDataSource"),
            "init");
    setivar(self, "treeDataSource", treeDataSource);

    id treeView = msgsend(
            msgsend(alloc("NSOutlineView"), "init"),
            "autorelease");

    id tableColumn = msgsend(
            msgsend(alloc("NSTableColumn"), 
                "initWithIdentifier:",
                STR("A")),
            "autorelease");
    msgsend(tableColumn, "setResizingMask:", NSTableColumnAutoresizingMask);

    msgsend(treeView, "addTableColumn:", tableColumn);
    msgsend(treeView, "setStronglyReferencesItems:", YES);
    msgsend(treeView, "setDataSource:", treeDataSource);
    msgsend(splitView, "addSubview:", treeView);

    id pane2 = msgsend(
        msgsend(
            objc_getClass("NSTextField"),
            "labelWithString:",
            STR("RIGHT PANEL")),
        "autorelease");
    msgsend(splitView, "addSubview:", pane2);

    msgsend(window, "setContentView:", splitView);
    return window;
}

static
id AppDelegate_init(id self, SEL _cmd) {
    self = msgsendsuper(self, "init");
    if(self) {
        id window = initWindow(self);
        setivar(self, "window", window);
    }
    return self;
}

static
void AppDelegate_dealloc(id self, SEL _cmd) {
    msgsend(getivar(self, "treeDataSource"), "release");
    msgsend(getivar(self, "window"), "release");
    msgsendsuper(self, "dealloc");
}

static
void AppDelegate_applicationDidFinishLaunching(id self, SEL _cmd, id aNotification) {
    msgsend(self, "populateMainMenu");
    msgsend(getivar(self, "window"), "makeKeyAndOrderFront:", self);
}

static
void AppDelegate_populateMainMenu(id self, SEL _cmd) {
     id mainMenu = msgsend(alloc("NSMenu"), "initWithTitle:", STR("MainMenu"));
     id menuItem = msgsend(
             mainMenu,
             "addItemWithTitle:action:keyEquivalent:",
             STR("Apple"),
             NULL,
             STR(""));
     id subMenu = msgsend(alloc("NSMenu"), "initWithTitle:", STR("Apple"));
     msgsend(NSApp, "setAppleMenu:", subMenu);

     id appMenuItem = msgsend(subMenu,
             "addItemWithTitle:action:keyEquivalent:",
             STR("About DBATool"),
             sel_getUid("orderFrontStandardAboutPanel:"),
             STR(""));
     msgsend(appMenuItem, "setTarget:", NSApp);
     msgsend(subMenu, "addItem:",
             msgsend(objc_getClass("NSMenuItem"), "separatorItem"));

     id quitMenuItem = msgsend(subMenu,
             "addItemWithTitle:action:keyEquivalent:",
             STR("Quit"),
             sel_getUid("terminate:"),
             STR("q"));
     msgsend(quitMenuItem, "setTarget:", NSApp);

     msgsend(
             mainMenu,
             "setSubmenu:forItem:",
             subMenu,
             menuItem);
     msgsend(NSApp, "setMainMenu:", mainMenu);
}

static
id TreeViewDataSource_init(id self, SEL _cmd) {
    if((self = msgsendsuper(self ,"init"))) {
        id items = msgsend(alloc("NSMutableArray"), "init");
        msgsend(items, "addObject:", STR("Item 1"));
        msgsend(items, "addObject:", STR("Item 2"));
        msgsend(items, "addObject:", STR("Item 3"));
        setivar(self, "items", items);
    }
    return self;
}

void TreeViewDataSource_dealloc(id self) {
    msgsend(getivar(self, "items"), "release");
    msgsendsuper(self, "dealloc");
}

static
id TreeViewDataSource_outlineView_child_ofItem(id self, SEL _cmd, id view, int child, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    if(!item) {
        id items = getivar(self, "items");
        return msgsend(items, "objectAtIndex:", child);
    } else {
        return nullptr;
    }
}

static
BOOL TreeViewDataSource_outlineView_isItemExpandable(id self, SEL _cmd, id view, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    // if(!item) {
    //     return YES;
    // } else {
        return NO;
    // }
}

static
int TreeViewDataSource_outlineView_numbernumberOfChildrenOfItem(id self, SEL _cmd, id view, id item) {
    printf("%s(%p)\n", __PRETTY_FUNCTION__, item);
    if(!item) {
        return 3;
    } else {
        return 0;
    }
}

static
id TreeViewDataSource_outlineView_objectValueForTableColumn_byItem(id self, SEL _cmd, id view, id column, id item) {
    printf("%s(%p, %p)\n", __PRETTY_FUNCTION__, column, item);
    if(item) {
        return item;
    } else {
        return STR("/");
    }
}

int main() {
    auto pool = msgsend(objc_getClass("NSAutoreleasePool"), "new");

    /* Ensure shared application instance is initialized */
    msgsend(objc_getClass("NSApplication"), "sharedApplication");

    Class appDelegateClass = objc_allocateClassPair(objc_getClass("NSObject"), "AppDelegate", 0);
    class_addMethod(appDelegateClass, sel_getUid("init"), (IMP)AppDelegate_init, "i@:");
    class_addMethod(appDelegateClass, sel_getUid("dealloc"), (IMP)AppDelegate_dealloc, "v@:");
    class_addMethod(appDelegateClass, sel_getUid("applicationDidFinishLaunching:"), (IMP)AppDelegate_applicationDidFinishLaunching, "v@:@");
    class_addMethod(appDelegateClass, sel_getUid("populateMainMenu"), (IMP)AppDelegate_populateMainMenu, "v@:");
    class_addIvar(appDelegateClass, "window", sizeof(id), alignof(id), "@");
    class_addIvar(appDelegateClass, "treeDataSource", sizeof(id), alignof(id), "@");
    objc_registerClassPair(appDelegateClass);

    Class treeDataSourceClass = objc_allocateClassPair(objc_getClass("NSObject"), "TreeViewDataSource", 0);
    class_addMethod(treeDataSourceClass, sel_getUid("init"), (IMP)TreeViewDataSource_init, "@@:");
    class_addMethod(treeDataSourceClass, sel_getUid("dealloc"), (IMP)TreeViewDataSource_dealloc, "v@:");
    class_addMethod(treeDataSourceClass, sel_getUid("outlineView:child:ofItem:"), (IMP)TreeViewDataSource_outlineView_child_ofItem, "@@:@i@");
    class_addMethod(treeDataSourceClass, sel_getUid("outlineView:isItemExpandable:"), (IMP)TreeViewDataSource_outlineView_isItemExpandable, "i@:@");
    class_addMethod(treeDataSourceClass, sel_getUid("outlineView:numberOfChildrenOfItem:"), (IMP)TreeViewDataSource_outlineView_numbernumberOfChildrenOfItem, "i@:@");
    class_addMethod(treeDataSourceClass, sel_getUid("outlineView:objectValueForTableColumn:byItem:"), (IMP)TreeViewDataSource_outlineView_objectValueForTableColumn_byItem, "@@:@@@");
    class_addIvar(treeDataSourceClass, "items", sizeof(id), alignof(id), "@");
    objc_registerClassPair(treeDataSourceClass);

    auto appDelegate = msgsend(msgsend(msgsend(appDelegateClass, "alloc"), "init"), "autorelease");

    msgsend(NSApp, "setActivationPolicy:", NSApplicationActivationPolicyRegular);
    msgsend(NSApp, "activateIgnoringOtherApps:", YES);
    msgsend(NSApp, "setDelegate:", appDelegate);

    msgsend(NSApp, "run");
    msgsend(pool, "drain");
    return 0;
}

