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

template<typename T, typename... Args>
id msgsend(T* obj, const char* sel, Args... args) {
    using fntype = id(*)(T*,SEL,decltype(args)...);
    return ((fntype)objc_msgSend)(obj, sel_getUid(sel), args...);
}

template<typename O>
id msgsend(O obj, const char* sel) {
    using fntype = id (*)(O,SEL);
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

template<typename O>
id msgsendsuper(O obj, const char* sel) {
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

#define STR(s) \
    msgsend(objc_getClass("NSString"), "stringWithUTF8String:", s)

void trace(const char* msg) {
    NSLog(STR(msg));
}

static
id AppDelegate_init(id self, SEL _cmd) {
    self = msgsendsuper(self, "init");
    if(self) {
        auto rc = CGRectMake(0.0, 0.0, 640.0, 480.0);
        auto window = msgsend(
            alloc("NSWindow"),
            "initWithContentRect:styleMask:backing:defer:",
            rc,
            NSWindowStyleMaskTitled,
            NSBackingStoreBuffered,
            YES);

        Ivar windowVar = class_getInstanceVariable(object_getClass(self), "window");
        object_setIvar(self, windowVar, window);
    }
    return self;
}

static
void AppDelegate_dealloc(id self, SEL _cmd) {
    id window = getivar(self, "window");
    msgsend(window, "release");
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
    objc_registerClassPair(appDelegateClass);
    auto appDelegate = msgsend(msgsend(msgsend(appDelegateClass, "alloc"), "init"), "autorelease");

    msgsend(NSApp, "setActivationPolicy:", NSApplicationActivationPolicyRegular);
    msgsend(NSApp, "activateIgnoringOtherApps:", YES);
    msgsend(NSApp, "setDelegate:", appDelegate);

    msgsend(NSApp, "run");
    msgsend(pool, "drain");
    return 0;
}

