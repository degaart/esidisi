#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <cassert>
#include <string>
#include <string_view>
#include <vector>

extern "C" void NSLog(objc_object *, ...);
extern id *NSApp;
#define NSApplicationActivationPolicyRegular 0
#define NSWindowStyleMaskTitled (1 << 0)
#define NSBackingStoreBuffered 2
#define NSSplitViewDividerStylePaneSplitter 3
#define NSTableColumnAutoresizingMask (1 << 0)
#define NSTableColumnUserResizingMask ( 1 << 1 )

struct TreeItem {
    std::string label;
    std::vector<std::unique_ptr<TreeItem>> children;
    
    TreeItem* addChild(std::string_view label) {
        auto child = std::make_unique<TreeItem>();
        child->label = label;
        TreeItem* result = child.get();
        children.push_back(std::move(child));
        return result;
    }
};

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

id stringWithUTF8String(const char* str) {
    return msgsend(objc_getClass("NSString"), "stringWithUTF8String:", str);
}

void trace(const char* msg) {
    NSLog(stringWithUTF8String(msg));
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
                CFSTR("A")),
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
            CFSTR("RIGHT PANEL")),
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
     id mainMenu = msgsend(alloc("NSMenu"), "initWithTitle:", CFSTR("MainMenu"));
     id menuItem = msgsend(
             mainMenu,
             "addItemWithTitle:action:keyEquivalent:",
             CFSTR("Apple"),
             NULL,
             CFSTR(""));
     id subMenu = msgsend(alloc("NSMenu"), "initWithTitle:", CFSTR("Apple"));
     msgsend(NSApp, "setAppleMenu:", subMenu);

     id appMenuItem = msgsend(subMenu,
             "addItemWithTitle:action:keyEquivalent:",
             CFSTR("About DBATool"),
             sel_getUid("orderFrontStandardAboutPanel:"),
             CFSTR(""));
     msgsend(appMenuItem, "setTarget:", NSApp);
     msgsend(subMenu, "addItem:",
             msgsend(objc_getClass("NSMenuItem"), "separatorItem"));

     id quitMenuItem = msgsend(subMenu,
             "addItemWithTitle:action:keyEquivalent:",
             CFSTR("Quit"),
             sel_getUid("terminate:"),
             CFSTR("q"));
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
        TreeItem* root = new TreeItem;
        root->label = "Root";

        auto child = root->addChild("Item 1");
        child->addChild("Item 1a");
        child->addChild("Item 1b");
        child->addChild("Item 1c");

        child = root->addChild("Item 2");
        child->addChild("Item 2a");
        child->addChild("Item 2b");
        child->addChild("Item 2c");

        child = root->addChild("Item 3");
        child->addChild("Item 3a");
        child->addChild("Item 3b");
        child->addChild("Item 3c");

        setivarp(self, "root", root);
    }
    return self;
}

void TreeViewDataSource_dealloc(id self) {
    printf("%s\n", __PRETTY_FUNCTION__);
    delete(getivar(self, "root"));
    msgsendsuper(self, "dealloc");
}

static
id TreeViewDataSource_outlineView_child_ofItem(id self, SEL _cmd, id view, int child, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    if(!item) {
        TreeItem* root = getivarp<TreeItem*>(self, "root");
        assert(root->label == "Root");

        TreeItem* obj = root->children.at(child).get();
        return msgsend(objc_getClass("NSValue"), "valueWithPointer:", obj);
    } else {
        TreeItem* obj = (TreeItem*)msgsend(item, "pointerValue");
        return msgsend(objc_getClass("NSValue"), "valueWithPointer:", obj->children.at(child).get());
    }
}

static
BOOL TreeViewDataSource_outlineView_isItemExpandable(id self, SEL _cmd, id view, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    if(!item) {
        TreeItem* root = getivarp<TreeItem*>(self, "root");
        assert(root->label == "Root");
        return !root->children.empty();
    } else {
        TreeItem* obj = (TreeItem*)msgsend(item, "pointerValue");
        return !obj->children.empty();
    }
}

static
int TreeViewDataSource_outlineView_numbernumberOfChildrenOfItem(id self, SEL _cmd, id view, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    if(!item) {
        TreeItem* root = getivarp<TreeItem*>(self, "root");
        return root->children.size();
    } else {
        TreeItem* obj = (TreeItem*)msgsend(item, "pointerValue");
        return obj->children.size();
    }
}

static
id TreeViewDataSource_outlineView_objectValueForTableColumn_byItem(id self, SEL _cmd, id view, id column, id item) {
    printf("%s\n", __PRETTY_FUNCTION__);
    if(!item) {
        TreeItem* root = getivarp<TreeItem*>(self, "root");
        return stringWithUTF8String(root->label.c_str());
    } else {
        TreeItem* obj = (TreeItem*)msgsend(item, "pointerValue");
        return stringWithUTF8String(obj->label.c_str());
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
    class_addIvar(treeDataSourceClass, "root",sizeof(TreeItem*), alignof(TreeItem*), "@");
    objc_registerClassPair(treeDataSourceClass);

    auto appDelegate = msgsend(msgsend(msgsend(appDelegateClass, "alloc"), "init"), "autorelease");

    msgsend(NSApp, "setActivationPolicy:", NSApplicationActivationPolicyRegular);
    msgsend(NSApp, "activateIgnoringOtherApps:", YES);
    msgsend(NSApp, "setDelegate:", appDelegate);

    msgsend(NSApp, "run");
    msgsend(pool, "drain");
    return 0;
}

