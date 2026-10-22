#include "test_color_utils.h"
#include "ui/ProfileSettingsDialog.h"

void TestColorUtils::testColorNameToQColorNamedColors() {
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Black"), QColor(Qt::black));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("black"), QColor(Qt::black));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("White"), QColor(Qt::white));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("white"), QColor(Qt::white));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Gray"), QColor(112, 112, 112));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Grey"), QColor(112, 112, 112));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Blue"), QColor(Qt::blue));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Aqua"), QColor(0, 255, 255));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Cyan"), QColor(0, 255, 255));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Red"), QColor(Qt::red));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Green"), QColor(Qt::green));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Yellow"), QColor(Qt::yellow));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("Magenta"), QColor(Qt::magenta));
}

void TestColorUtils::testColorNameToQColorHexCodes() {
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("#FAFBFC"), QColor(0xFA, 0xFB, 0xFC));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("#1F2328"), QColor(0x1F, 0x23, 0x28));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("#123456"), QColor(0x12, 0x34, 0x56));
}

void TestColorUtils::testColorNameToQColorFallbacks() {
    // 空字符串回退为白色
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor(""), QColor(Qt::white));
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("   "), QColor(Qt::white));

    // 未知命名回退为白色
    QCOMPARE(ProfileSettingsDialog::colorNameToQColor("NonExistentColor123"), QColor(Qt::white));
}

void TestColorUtils::testQColorToColorName() {
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::black), QString("Black"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(0x1F, 0x23, 0x28)), QString("Black"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::white), QString("White"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(0xFA, 0xFB, 0xFC)), QString("White"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(112, 112, 112)), QString("Gray"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::gray), QString("Gray"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::blue), QString("Blue"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(0x09, 0x69, 0xDA)), QString("Blue"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(0, 255, 255)), QString("Aqua"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::cyan), QString("Aqua"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::red), QString("Red"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::green), QString("Green"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::yellow), QString("Yellow"));
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(Qt::magenta), QString("Magenta"));

    // 自定义颜色输出其十六进制
    QCOMPARE(ProfileSettingsDialog::qcolorToColorName(QColor(12, 34, 56)), QString("#0C2238"));
}
