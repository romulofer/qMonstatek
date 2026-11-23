#include "ui_settings.h"

#include <QSettings>

QUrl UiSettings::dialogFolder(const QString &key) const
{
    if (key.isEmpty())
        return QUrl();
    QSettings &s = m_settings;
    return s.value(QStringLiteral("dialogFolders/") + key).toUrl();
}

void UiSettings::setDialogFolder(const QString &key, const QUrl &folder)
{
    if (key.isEmpty() || folder.isEmpty())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("dialogFolders/") + key, folder);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
}

QString UiSettings::theme() const
{
    QSettings &s = m_settings;
    return s.value(QStringLiteral("appearance/theme"), QStringLiteral("dark")).toString();
}

void UiSettings::setTheme(const QString &t)
{
    if (t == theme())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("appearance/theme"), t);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
    emit themeChanged();
}

QString UiSettings::caseColor() const
{
    QSettings &s = m_settings;
    return s.value(QStringLiteral("appearance/caseColor"), QStringLiteral("white")).toString();
}

void UiSettings::setCaseColor(const QString &c)
{
    if (c == caseColor())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("appearance/caseColor"), c);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
    emit caseColorChanged();
}

QString UiSettings::accent() const
{
    QSettings &s = m_settings;
    return s.value(QStringLiteral("appearance/accent"), QStringLiteral("green")).toString();
}

void UiSettings::setAccent(const QString &a)
{
    if (a == accent())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("appearance/accent"), a);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
    emit accentChanged();
}

bool UiSettings::guidedEspPending() const
{
    QSettings &s = m_settings;
    return s.value(QStringLiteral("setup/guidedEspPending"), false).toBool();
}

void UiSettings::setGuidedEspPending(bool v)
{
    if (v == guidedEspPending())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("setup/guidedEspPending"), v);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
    emit guidedEspPendingChanged();
}

int UiSettings::defaultFps() const
{
    QSettings &s = m_settings;
    return qBound(1, s.value(QStringLiteral("screen/defaultFps"), 10).toInt(), 15);
}

void UiSettings::setDefaultFps(int fps)
{
    fps = qBound(1, fps, 15);
    if (fps == defaultFps())
        return;
    QSettings &s = m_settings;
    s.setValue(QStringLiteral("screen/defaultFps"), fps);
    s.sync();   // persist now; a long-lived QSettings would otherwise write lazily
    emit defaultFpsChanged();
}
