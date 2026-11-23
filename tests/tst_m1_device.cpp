/*
 * M1Device transfer-state tests. No device is attached: frames are injected
 * through the (private) onFrameReceived slot, which is reachable via the meta
 * object. These guard the stuck-state bugs in the upload / mkdir queues.
 */
#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include <QFile>
#include "device/m1_device.h"
#include "protocol/rpc_protocol.h"

Q_DECLARE_METATYPE(rpc::Frame)

class TestM1Device : public QObject {
    Q_OBJECT
    QTemporaryDir m_cfg;
    QTemporaryDir m_files;

    static void inject(M1Device &dev, uint8_t cmd, const QByteArray &payload = {})
    {
        rpc::Frame f;
        f.cmd = cmd;
        f.payload = payload;
        QMetaObject::invokeMethod(&dev, "onFrameReceived", Qt::DirectConnection,
                                  Q_ARG(rpc::Frame, f));
    }
    static void nack(M1Device &dev, uint8_t code) { inject(dev, rpc::CMD_NACK, QByteArray(1, char(code))); }

    QString makeFile(const QString &name, const QByteArray &content)
    {
        const QString path = m_files.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        return path;
    }

private slots:
    void initTestCase()
    {
        qRegisterMetaType<rpc::Frame>("rpc::Frame");
        QCoreApplication::setOrganizationName("qmonstatek-test");
        QCoreApplication::setApplicationName("tst_m1_device");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_cfg.path());
    }

    // An unreadable/empty file used to leave the queue index set, so every later
    // upload failed with "Upload already in progress" until reconnect.
    void emptyFileInQueueDoesNotWedgeUploads()
    {
        M1Device dev;
        QSignalSpy err(&dev, &M1Device::fileOperationError);
        const QString empty = makeFile("empty.bin", QByteArray());
        dev.uploadFiles({QUrl::fromLocalFile(empty).toString()}, "0:/");
        QCOMPARE(err.count(), 1);
        QVERIFY(err.at(0).at(0).toString().contains("empty", Qt::CaseInsensitive));
        err.clear();
        dev.uploadFiles({QUrl::fromLocalFile(empty).toString()}, "0:/");
        QCOMPARE(err.count(), 1);
        QVERIFY2(!err.at(0).at(0).toString().contains("already in progress"),
                 qPrintable(err.at(0).at(0).toString()));
    }

    void missingFileInQueueDoesNotWedgeUploads()
    {
        M1Device dev;
        QSignalSpy err(&dev, &M1Device::fileOperationError);
        const QString gone = m_files.filePath("does-not-exist.bin");
        dev.uploadFiles({QUrl::fromLocalFile(gone).toString()}, "0:/");
        dev.uploadFiles({QUrl::fromLocalFile(gone).toString()}, "0:/");
        QCOMPARE(err.count(), 2);
        QVERIFY(!err.at(1).at(0).toString().contains("already in progress"));
    }

    // A non-"exists" mkdir NACK aborts the folder upload.
    void mkdirNackAbortsFolderUpload()
    {
        M1Device dev;
        QDir().mkpath(m_files.filePath("up/sub"));
        makeFile("up/sub/a.txt", "data");
        QSignalSpy err(&dev, &M1Device::fileOperationError);
        QSignalSpy progress(&dev, &M1Device::multiUploadProgress);
        dev.uploadFolder(QUrl::fromLocalFile(m_files.filePath("up")).toString(), "0:/");
        QCOMPARE(err.count(), 0);                       // waiting for the first mkdir reply
        nack(dev, rpc::ERR_SD_NOT_READY);
        QCOMPARE(err.count(), 1);
        QVERIFY(err.at(0).at(0).toString().startsWith("Upload failed"));
        // A later unrelated ACK must not resume the stale queue.
        const int before = progress.count();
        inject(dev, rpc::CMD_ACK);
        QCOMPARE(progress.count(), before);
    }

    // The queue must be reusable after an abort.
    void folderUploadCanRestartAfterAbort()
    {
        M1Device dev;
        QDir().mkpath(m_files.filePath("up2"));
        makeFile("up2/a.txt", "data");
        QSignalSpy err(&dev, &M1Device::fileOperationError);
        dev.uploadFolder(QUrl::fromLocalFile(m_files.filePath("up2")).toString(), "0:/");
        nack(dev, rpc::ERR_SD_NOT_READY);
        err.clear();
        dev.uploadFolder(QUrl::fromLocalFile(m_files.filePath("up2")).toString(), "0:/");
        QVERIFY(err.isEmpty());
    }

    // An already-existing destination folder is fine: the upload carries on.
    void mkdirExistsNackContinuesUpload()
    {
        M1Device dev;
        QDir().mkpath(m_files.filePath("up3"));
        makeFile("up3/a.txt", "data");
        QSignalSpy err(&dev, &M1Device::fileOperationError);
        QSignalSpy progress(&dev, &M1Device::multiUploadProgress);
        dev.uploadFolder(QUrl::fromLocalFile(m_files.filePath("up3")).toString(), "0:/");
        const int before = progress.count();
        nack(dev, rpc::ERR_FILE_EXISTS);
        QCOMPARE(err.count(), 0);
        QVERIFY2(progress.count() > before, "upload should move on to the files");
    }

    // A failed open used to be reported twice (the port's own error signal and
    // open()'s message). It must be exactly once, with the failing port named.
    void failedOpenIsReportedOnce()
    {
        M1Device dev;
        QSignalSpy err(&dev, &M1Device::errorOccurred);
        QVERIFY(!dev.connectToDevice("qmonstatek-no-such-port"));
        QCOMPARE(err.count(), 1);
        QVERIFY(err.at(0).at(0).toString().contains("qmonstatek-no-such-port"));
    }

    void autoConnectPersistsAndEmits()
    {
        QSettings().clear();
        {
            M1Device dev;
            QVERIFY(dev.autoConnect());
            QSignalSpy spy(&dev, &M1Device::autoConnectChanged);
            dev.setAutoConnect(false);
            dev.setAutoConnect(false);
            QCOMPARE(spy.count(), 1);
        }
        M1Device again;
        QVERIFY(!again.autoConnect());
    }
};

QTEST_GUILESS_MAIN(TestM1Device)
#include "tst_m1_device.moc"
