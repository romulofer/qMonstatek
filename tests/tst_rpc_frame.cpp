#include <QtTest>
#include "protocol/rpc_frame.h"
#include "protocol/rpc_protocol.h"
#include "protocol/crc16.h"

using namespace rpc;

class TestRpcFrame : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qRegisterMetaType<rpc::Frame>("rpc::Frame"); }
    void init() { resetCrcDialect(); }

    void roundTrip()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        const QByteArray payload("\x01\x02\x03payload", 10);
        codec.feed(FrameCodec::buildFrame(CMD_PING, 7, payload));
        QCOMPARE(spy.count(), 1);
        const Frame f = spy.takeFirst().at(0).value<Frame>();
        QCOMPARE(f.cmd, uint8_t(CMD_PING));
        QCOMPARE(f.seq, uint8_t(7));
        QCOMPARE(f.payload, payload);
    }

    void emptyPayload()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        codec.feed(FrameCodec::buildFrame(CMD_PING, 1));
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.takeFirst().at(0).value<Frame>().payload.isEmpty());
    }

    void byteAtATime()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        const QByteArray wire = FrameCodec::buildFrame(CMD_PING, 3, QByteArray(40, 'x'));
        for (char c : wire)
            codec.feed(QByteArray(1, c));
        QCOMPARE(spy.count(), 1);
    }

    void garbageBeforeSyncIsSkipped()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        QByteArray junk(5, char(0x00));
        codec.feed(junk + FrameCodec::buildFrame(CMD_PING, 2, QByteArray("ok")));
        QCOMPARE(spy.count(), 1);
    }

    void backToBackFrames()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        codec.feed(FrameCodec::buildFrame(CMD_PING, 1, "a") + FrameCodec::buildFrame(CMD_PING, 2, "b"));
        QCOMPARE(spy.count(), 2);
    }

    void corruptedCrcIsReportedAndDropped()
    {
        FrameCodec codec;
        QSignalSpy ok(&codec, &FrameCodec::frameReady);
        QSignalSpy bad(&codec, &FrameCodec::frameCrcError);
        QByteArray wire = FrameCodec::buildFrame(CMD_PING, 1, QByteArray("payload"));
        wire[wire.size() - 1] = char(wire[wire.size() - 1] ^ 0x5A);
        codec.feed(wire);
        QCOMPARE(ok.count(), 0);
        QCOMPARE(bad.count(), 1);
    }

    void oversizedLengthResyncs()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        // SYNC, cmd, seq, len = 0xFFFF (> MAX_PAYLOAD): must resync, not wait for 64 KB.
        QByteArray bad;
        bad.append(char(SYNC_BYTE)); bad.append(char(CMD_PING)); bad.append(char(1));
        bad.append(char(0xFF)); bad.append(char(0xFF));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("exceeds max"));
        codec.feed(bad + FrameCodec::buildFrame(CMD_PING, 9, QByteArray("after")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Frame>().seq, uint8_t(9));
    }

    void resetDropsPartialFrame()
    {
        FrameCodec codec;
        QSignalSpy spy(&codec, &FrameCodec::frameReady);
        const QByteArray wire = FrameCodec::buildFrame(CMD_PING, 1, QByteArray(10, 'x'));
        codec.feed(wire.left(6));
        codec.reset();
        codec.feed(FrameCodec::buildFrame(CMD_PING, 2, QByteArray("y")));
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).value<Frame>().seq, uint8_t(2));
    }

    void sequenceCounterWraps()
    {
        FrameCodec codec;
        uint8_t last = 0;
        for (int i = 0; i < 300; ++i)
            last = codec.nextSeq();
        QCOMPARE(last, uint8_t(299 & 0xFF));
    }
};

QTEST_GUILESS_MAIN(TestRpcFrame)
#include "tst_rpc_frame.moc"
