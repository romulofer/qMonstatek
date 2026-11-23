#include <QtTest>
#include <QThread>
#include "device/log_model.h"

class TestLogModel : public QObject {
    Q_OBJECT

private slots:
    void appendAndRead()
    {
        LogModel m;
        m.append("one");
        m.append("two");
        QCOMPARE(m.rowCount(), 2);
        QCOMPARE(m.data(m.index(1), LogModel::MessageRole).toString(), QString("two"));
        QCOMPARE(m.fullText(), QString("one\ntwo"));
    }

    void clearEmptiesModel()
    {
        LogModel m;
        m.append("x");
        m.clear();
        QCOMPARE(m.rowCount(), 0);
        QVERIFY(m.fullText().isEmpty());
    }

    void capsAtMaxLines()
    {
        LogModel m;
        for (int i = 0; i < 5200; ++i)
            m.append(QString::number(i));
        QCOMPARE(m.rowCount(), 5000);
        QCOMPARE(m.data(m.index(0), LogModel::MessageRole).toString(), QString("200"));
        QCOMPARE(m.data(m.index(4999), LogModel::MessageRole).toString(), QString("5199"));
    }

    void globalPointerClearedOnDestruction()
    {
        {
            LogModel m;
            setGlobalLogModel(&m);
            QCOMPARE(globalLogModel(), &m);
        }
        QVERIFY(globalLogModel() == nullptr);
    }

    // The Qt message handler logs from worker threads. append() must be applied
    // on the model's own thread, never directly from the caller's.
    void appendFromWorkerThreadIsQueued()
    {
        LogModel m;
        QSignalSpy spy(&m, &LogModel::lineAppended);
        QThread t;
        QObject worker;
        worker.moveToThread(&t);
        t.start();
        QMetaObject::invokeMethod(&worker, [&m]() { m.append("from worker"); },
                                  Qt::BlockingQueuedConnection);
        // Nothing applied yet: it is queued to this (the model's) thread.
        QCOMPARE(m.rowCount(), 0);
        QTRY_COMPARE(m.rowCount(), 1);
        QCOMPARE(spy.count(), 1);
        t.quit();
        t.wait();
    }
};

QTEST_GUILESS_MAIN(TestLogModel)
#include "tst_log_model.moc"
