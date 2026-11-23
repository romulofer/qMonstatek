#include <QtTest>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QFile>
#include "updater/github_checker.h"

class TestGithubChecker : public QObject {
    Q_OBJECT

private slots:
    void parseFourComponentWithC3()
    {
        int a, b, c, d, e;
        QVERIFY(GithubChecker::parseVersionTag("v0.8.0.4-C3.2", a, b, c, d, e));
        QCOMPARE(a, 0); QCOMPARE(b, 8); QCOMPARE(c, 0); QCOMPARE(d, 4); QCOMPARE(e, 2);
    }

    void parseFourComponentWithoutC3()
    {
        int a, b, c, d, e;
        QVERIFY(GithubChecker::parseVersionTag("v1.2.3.4", a, b, c, d, e));
        QCOMPARE(e, 0);
        QCOMPARE(d, 4);
    }

    void parseC3Only()
    {
        int a, b, c, d, e;
        QVERIFY(GithubChecker::parseVersionTag("C3.4", a, b, c, d, e));
        QCOMPARE(a, 0); QCOMPARE(b, 8); QCOMPARE(e, 4);
    }

    void parseThreeComponent()
    {
        int a, b, c, d, e;
        QVERIFY(GithubChecker::parseVersionTag("v2.10.3", a, b, c, d, e));
        QCOMPARE(a, 2); QCOMPARE(b, 10); QCOMPARE(c, 3); QCOMPARE(d, 0);
    }

    void parseRejectsGarbage()
    {
        int a, b, c, d, e;
        QVERIFY(!GithubChecker::parseVersionTag("latest", a, b, c, d, e));
        QVERIFY(!GithubChecker::parseVersionTag("", a, b, c, d, e));
    }

    void md5VerifyMatchAndMismatch()
    {
        QTemporaryDir dir;
        const QString bin = dir.filePath("fw.bin");
        const QString md5 = dir.filePath("fw.md5");
        QFile b(bin); QVERIFY(b.open(QIODevice::WriteOnly)); b.write("hello"); b.close();
        const QByteArray hex = QCryptographicHash::hash("hello", QCryptographicHash::Md5).toHex();
        QFile m(md5); QVERIFY(m.open(QIODevice::WriteOnly)); m.write(hex + "  fw.bin\n"); m.close();
        GithubChecker gc;
        QVERIFY(gc.verifyFileMd5(bin, md5)["match"].toBool());
        QFile m2(md5); QVERIFY(m2.open(QIODevice::WriteOnly | QIODevice::Truncate));
        m2.write("00000000000000000000000000000000\n"); m2.close();
        QVERIFY(!gc.verifyFileMd5(bin, md5)["match"].toBool());
        QVERIFY(gc.verifyFileMd5(bin, dir.filePath("missing.md5")).contains("error"));
    }

    void fileSizeReportsMissing()
    {
        GithubChecker gc;
        QCOMPARE(gc.fileSize("/definitely/not/here"), qint64(-1));
    }

    void digestEmptyOrUnknownAlgorithmPasses()
    {
        QVERIFY(GithubChecker::digestMatches("data", QString()));
        QVERIFY(GithubChecker::digestMatches("data", "md5:abc"));
    }

    void digestSha256Matches()
    {
        const QByteArray data("firmware bytes");
        const QString hex = QString::fromLatin1(
            QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
        QVERIFY(GithubChecker::digestMatches(data, "sha256:" + hex));
        QVERIFY(GithubChecker::digestMatches(data, "SHA256:" + hex.toUpper()));
    }

    void digestSha256MismatchFails()
    {
        const QByteArray data("firmware bytes");
        QVERIFY(!GithubChecker::digestMatches("tampered bytes!",
            "sha256:" + QString::fromLatin1(
                QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex())));
        QVERIFY(!GithubChecker::digestMatches(data, "sha256:deadbeef"));
    }
};

QTEST_GUILESS_MAIN(TestGithubChecker)
#include "tst_github_checker.moc"
