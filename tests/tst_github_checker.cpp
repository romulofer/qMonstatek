#include <QtTest>
#include <QCryptographicHash>
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
