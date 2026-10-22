#include "test_color_utils.h"
#include "test_profile_manager.h"
#include "test_terminal_widget.h"
#include "test_ui_dialogs.h"
#include <QApplication>
#include <QTest>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(resources);
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_Use96Dpi, true);

    int status = 0;

    {
        TestProfileManager testProfile;
        status |= QTest::qExec(&testProfile, argc, argv);
    }
    {
        TestColorUtils testColor;
        status |= QTest::qExec(&testColor, argc, argv);
    }
    {
        TestTerminalWidget testTerm;
        status |= QTest::qExec(&testTerm, argc, argv);
    }
    {
        TestUiDialogs testUi;
        status |= QTest::qExec(&testUi, argc, argv);
    }

    return status;
}
