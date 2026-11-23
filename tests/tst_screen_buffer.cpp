#include <QtTest>
#include "device/screen_buffer.h"

class TestScreenBuffer : public QObject {
    Q_OBJECT

private slots:
    void rejectsWrongSize()
    {
        ScreenBuffer sb;
        QVERIFY(!sb.update(QByteArray(1023, 0)));
        QVERIFY(!sb.update(QByteArray(1025, 0)));
        QVERIFY(sb.update(QByteArray(ScreenBuffer::FB_SIZE, 0)));
    }

    // u8g2 tile layout: byte = (y / 8) * 128 + x, bit = y % 8 (LSB is topmost).
    void pixelMapping()
    {
        QByteArray fb(ScreenBuffer::FB_SIZE, 0);
        fb[0] = char(0x01);                 // (0, 0)
        fb[128 * 3 + 17] = char(0x80);      // x = 17, y = 3*8 + 7 = 31
        fb[128 * 7 + 127] = char(0x02);     // (127, 57)
        ScreenBuffer sb;
        QVERIFY(sb.update(fb));
        QVERIFY(sb.pixel(0, 0));
        QVERIFY(sb.pixel(17, 31));
        QVERIFY(sb.pixel(127, 57));
        QVERIFY(!sb.pixel(1, 0));
        QVERIFY(!sb.pixel(0, 1));
        QVERIFY(!sb.pixel(17, 30));
    }

    void outOfRangeIsOff()
    {
        ScreenBuffer sb;
        QVERIFY(sb.update(QByteArray(ScreenBuffer::FB_SIZE, char(0xFF))));
        QVERIFY(!sb.pixel(-1, 0));
        QVERIFY(!sb.pixel(0, -1));
        QVERIFY(!sb.pixel(128, 0));
        QVERIFY(!sb.pixel(0, 64));
        QVERIFY(sb.pixel(127, 63));
    }

    void imageSizesAndColours()
    {
        ScreenBuffer sb;
        sb.setColors(Qt::white, Qt::black);
        QByteArray fb(ScreenBuffer::FB_SIZE, 0);
        fb[0] = char(0x01);
        QVERIFY(sb.update(fb));
        const QImage native = sb.toNativeImage();
        QCOMPARE(native.size(), QSize(ScreenBuffer::WIDTH, ScreenBuffer::HEIGHT));
        QCOMPARE(native.pixel(0, 0), QColor(Qt::white).rgb());
        QCOMPARE(native.pixel(1, 0), QColor(Qt::black).rgb());
        const QImage big = sb.toImage();
        QCOMPARE(big.size(), QSize(ScreenBuffer::DISPLAY_WIDTH, ScreenBuffer::DISPLAY_HEIGHT));
        // Nearest-neighbour upscale: the whole 4x4 block of pixel (0,0) is on.
        QCOMPARE(big.pixel(3, 3), QColor(Qt::white).rgb());
        QCOMPARE(big.pixel(4, 0), QColor(Qt::black).rgb());
    }
};

QTEST_GUILESS_MAIN(TestScreenBuffer)
#include "tst_screen_buffer.moc"
