#include <QtTest>
#include "protocol/crc16.h"

using namespace rpc;

class TestCrc16 : public QObject {
    Q_OBJECT

private slots:
    void init() { resetCrcDialect(); }

    void knownVector()
    {
        // CRC-16/CCITT-FALSE check value for "123456789".
        const QByteArray in("123456789");
        QCOMPARE(crc16_ccitt(reinterpret_cast<const uint8_t *>(in.constData()),
                             static_cast<size_t>(in.size())),
                 uint16_t(0x29B1));
    }

    void resetRestoresCorrectDialect()
    {
        setTxCrcDialect(CrcDialect::Legacy);
        resetCrcDialect();
        QCOMPARE(txCrcDialect(), CrcDialect::Correct);
        QVERIFY(!crcDialectLocked());
    }

    void corruptFrameIsRejectedAndDoesNotLatch()
    {
        const QByteArray d("hello");
        const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
        const uint16_t good = crc16_ccitt(p, size_t(d.size()));
        const uint16_t legacy = crc16_ccitt_legacy(p, size_t(d.size()));
        uint16_t bad = uint16_t(good + 1);
        if (bad == legacy)
            bad = uint16_t(bad + 1);
        QVERIFY(!crc16ValidateRx(p, size_t(d.size()), bad));
        QVERIFY(!crcDialectLocked());
    }

    // Both tables agree on some inputs. A frame that validates under both says
    // nothing about the peer's dialect and must not latch it.
    void ambiguousFrameDoesNotLatch()
    {
        QByteArray d;
        bool found = false;
        for (int n = 1; n <= 4 && !found; ++n) {
            for (int v = 0; v < 256 && !found; ++v) {
                d = QByteArray(n, char(v));
                const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
                found = crc16_ccitt(p, size_t(d.size())) == crc16_ccitt_legacy(p, size_t(d.size()));
            }
        }
        QVERIFY2(found, "expected at least one input both tables agree on");
        const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
        QVERIFY(crc16ValidateRx(p, size_t(d.size()), crc16_ccitt(p, size_t(d.size()))));
        QVERIFY(!crcDialectLocked());
        QCOMPARE(txCrcDialect(), CrcDialect::Correct);
    }

    void legacyOnlyFrameLatchesLegacy()
    {
        QByteArray d;
        bool found = false;
        for (int n = 1; n <= 64 && !found; ++n) {
            d = QByteArray(n, char(0xA5));
            const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
            found = crc16_ccitt(p, size_t(d.size())) != crc16_ccitt_legacy(p, size_t(d.size()));
        }
        QVERIFY2(found, "expected an input the two tables disagree on");
        const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
        QVERIFY(crc16ValidateRx(p, size_t(d.size()), crc16_ccitt_legacy(p, size_t(d.size()))));
        QVERIFY(crcDialectLocked());
        QCOMPARE(txCrcDialect(), CrcDialect::Legacy);
    }

    void correctOnlyFrameLatchesCorrect()
    {
        QByteArray d;
        bool found = false;
        for (int n = 1; n <= 64 && !found; ++n) {
            d = QByteArray(n, char(0xA5));
            const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
            found = crc16_ccitt(p, size_t(d.size())) != crc16_ccitt_legacy(p, size_t(d.size()));
        }
        QVERIFY(found);
        setTxCrcDialect(CrcDialect::Legacy);
        const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
        QVERIFY(crc16ValidateRx(p, size_t(d.size()), crc16_ccitt(p, size_t(d.size()))));
        QVERIFY(crcDialectLocked());
        QCOMPARE(txCrcDialect(), CrcDialect::Correct);
    }

    void latchHoldsAfterFirstUnambiguousFrame()
    {
        QByteArray d;
        for (int n = 1; n <= 64; ++n) {
            d = QByteArray(n, char(0xA5));
            const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
            if (crc16_ccitt(p, size_t(d.size())) != crc16_ccitt_legacy(p, size_t(d.size())))
                break;
        }
        const auto *p = reinterpret_cast<const uint8_t *>(d.constData());
        QVERIFY(crc16ValidateRx(p, size_t(d.size()), crc16_ccitt_legacy(p, size_t(d.size()))));
        QCOMPARE(txCrcDialect(), CrcDialect::Legacy);
        // A later correct-dialect frame is still accepted but must not flip TX.
        QVERIFY(crc16ValidateRx(p, size_t(d.size()), crc16_ccitt(p, size_t(d.size()))));
        QCOMPARE(txCrcDialect(), CrcDialect::Legacy);
    }
};

QTEST_GUILESS_MAIN(TestCrc16)
#include "tst_crc16.moc"
