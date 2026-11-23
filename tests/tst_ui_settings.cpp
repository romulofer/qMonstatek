#include <QtTest>
#include <QTemporaryDir>
#include <QSettings>
#include "ui_settings.h"

class TestUiSettings : public QObject {
    Q_OBJECT
    QTemporaryDir m_dir;

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName("qmonstatek-test");
        QCoreApplication::setApplicationName("tst_ui_settings");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
    }
    void init() { QSettings().clear(); }

    void defaults()
    {
        UiSettings s;
        QCOMPARE(s.theme(), QString("dark"));
        QCOMPARE(s.accent(), QString("green"));
        QCOMPARE(s.caseColor(), QString("white"));
        QCOMPARE(s.defaultFps(), 10);
        QVERIFY(!s.guidedEspPending());
    }

    void setterEmitsOnlyOnChange()
    {
        UiSettings s;
        QSignalSpy spy(&s, &UiSettings::themeChanged);
        s.setTheme("light");
        s.setTheme("light");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(s.theme(), QString("light"));
    }

    void persistsAcrossInstances()
    {
        {
            UiSettings a;
            a.setAccent("magenta");
            a.setGuidedEspPending(true);
        }
        UiSettings b;
        QCOMPARE(b.accent(), QString("magenta"));
        QVERIFY(b.guidedEspPending());
    }

    void defaultFpsIsClamped()
    {
        UiSettings s;
        s.setDefaultFps(99);
        QCOMPARE(s.defaultFps(), 15);
        s.setDefaultFps(-4);
        QCOMPARE(s.defaultFps(), 1);
        s.setDefaultFps(7);
        QCOMPARE(s.defaultFps(), 7);
    }

    void dialogFolderPerKey()
    {
        UiSettings s;
        QVERIFY(s.dialogFolder("fw").isEmpty());
        s.setDialogFolder("fw", QUrl("file:///tmp/a"));
        s.setDialogFolder("esp", QUrl("file:///tmp/b"));
        QCOMPARE(s.dialogFolder("fw"), QUrl("file:///tmp/a"));
        QCOMPARE(s.dialogFolder("esp"), QUrl("file:///tmp/b"));
        s.setDialogFolder("", QUrl("file:///tmp/c"));        // ignored
        s.setDialogFolder("fw", QUrl());                      // ignored
        QCOMPARE(s.dialogFolder("fw"), QUrl("file:///tmp/a"));
    }
};

QTEST_GUILESS_MAIN(TestUiSettings)
#include "tst_ui_settings.moc"
