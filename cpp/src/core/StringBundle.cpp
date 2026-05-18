#include "core/StringBundle.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QStringConverter>
#include <QTextStream>

namespace {
QString repoRoot() {
#ifdef LABELIMG_REPO_ROOT
    return QString::fromUtf8(LABELIMG_REPO_ROOT);
#else
    return QDir::currentPath();
#endif
}
}

StringBundle::StringBundle(const QString &language) {
    load(language.isEmpty() ? systemLanguage() : normalizeLanguage(language));
}

QString StringBundle::normalizeLanguage(const QString &localeName) {
    if (localeName.isEmpty()) {
        return "en";
    }

    QString normalized = localeName;
    normalized = normalized.section('.', 0, 0).section('@', 0, 0).replace('_', '-').toLower();

    if (normalized == "zh-cn") return "zh-CN";
    if (normalized == "zh-tw" || normalized == "zh-hk" || normalized == "zh-mo" || normalized == "zh-hant") return "zh-TW";
    if (normalized.startsWith("zh")) return "zh-CN";
    if (normalized == "ja" || normalized.startsWith("ja-")) return "ja-JP";
    if (normalized == "en" || normalized.startsWith("en-")) return "en";
    return "en";
}

QString StringBundle::systemLanguage() {
    return normalizeLanguage(QLocale::system().name());
}

QStringList StringBundle::supportedLanguages() {
    return {"en", "zh-CN", "zh-TW", "ja-JP"};
}

QString StringBundle::language() const {
    return m_language;
}

QString StringBundle::get(const QString &key) const {
    return m_messages.value(key, key);
}

void StringBundle::load(const QString &language) {
    m_language = normalizeLanguage(language);
    const QString dir = QDir(repoRoot()).filePath("resources/strings");
    loadFile(QDir(dir).filePath("strings.properties"));
    if (m_language != "en") {
        loadFile(QDir(dir).filePath(QString("strings-%1.properties").arg(m_language)));
    }
}

void StringBundle::loadFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    while (!stream.atEnd()) {
        QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }
        int separator = line.indexOf('=');
        if (separator < 0) {
            continue;
        }
        QString key = line.left(separator).trimmed();
        QString value = line.mid(separator + 1).trimmed();
        if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"')) {
            value = value.mid(1, value.size() - 2);
        }
        m_messages.insert(key, value);
    }
}
