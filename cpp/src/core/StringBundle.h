#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class StringBundle {
public:
    explicit StringBundle(const QString &language = QString());

    static QString normalizeLanguage(const QString &localeName);
    static QString systemLanguage();
    static QStringList supportedLanguages();

    QString language() const;
    QString get(const QString &key) const;

private:
    void load(const QString &language);
    void loadFile(const QString &path);

    QString m_language;
    QHash<QString, QString> m_messages;
};
