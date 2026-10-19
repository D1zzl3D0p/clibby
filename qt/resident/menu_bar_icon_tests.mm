#include "menu_bar_icon.h"
#include <QApplication>
#include <QMenu>
#include <iostream>
#import <AppKit/AppKit.h>

// macOS 27 gesture delivery can leave a non-mouse currentEvent. Supply it
// deterministically without sending global input to the user's desktop.
@interface ClibbyMenuTestApplication : NSApplication
@property(strong) NSEvent *testEvent;
@end
@implementation ClibbyMenuTestApplication
- (NSEvent *)currentEvent { return self.testEvent ?: [super currentEvent]; }
@end

int main(int argc, char **argv) {
    [ClibbyMenuTestApplication sharedApplication];
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    if (QGuiApplication::platformName() != "cocoa") return 77;
    QMenu menu;
    auto *action = menu.addAction("Pause");
    auto *nativeMenu = menu.toNSMenu();
    auto *nativeApp = (ClibbyMenuTestApplication *)NSApp;
    nativeApp.testEvent = [NSEvent otherEventWithType:NSEventTypeApplicationDefined
        location:NSZeroPoint modifierFlags:0 timestamp:0 windowNumber:0
        context:nil subtype:0 data1:0 data2:0];
    @try {
        for (int i = 0; i < 3; ++i) {
            pets::MenuBarIcon icon(&menu);
            QApplication::processEvents();
            [NSNotificationCenter.defaultCenter
                postNotificationName:NSMenuDidBeginTrackingNotification object:nativeMenu];
            action->setText("Resume");
            if (![[nativeMenu itemAtIndex:0].title isEqualToString:@"Resume"]) {
                std::cerr << "FAIL native menu action label synchronization\n";
                return 1;
            }
        }
    } @catch (NSException *exception) {
        std::cerr << "FAIL menu tracking with a non-mouse event: "
                  << exception.reason.UTF8String << '\n';
        return 1;
    }
    nativeApp.testEvent = nil;
    std::cout << "PASS non-mouse menu tracking, label synchronization and repeated teardown\n";
}
