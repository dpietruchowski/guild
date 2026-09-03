#include "agentdirectory.h"

#include <QFileInfo>
#include <QUuid>

static const QUuid SESSION_NAMESPACE("{7c8f2d94-3b1e-4a6f-9c4d-8e2a1b0f5d73}");

static bool isUsableName(const QString& name)
{
    if (name.isEmpty() || name.startsWith(QLatin1Char('.')))
    {
        return false;
    }
    return !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'));
}

AgentDirectory::AgentDirectory(const QString& name, const QString& path)
{
    if (!isUsableName(name))
    {
        return;
    }

    m_name = name;
    m_path = path;
}

bool AgentDirectory::isValid() const { return !m_path.isEmpty() && QFileInfo(m_path).isDir(); }

QString AgentDirectory::name() const { return m_name; }

QString AgentDirectory::path() const { return m_path; }

QString AgentDirectory::configFile() const { return child(QStringLiteral("agent.toml")); }

QString AgentDirectory::promptFile() const { return child(QStringLiteral("CLAUDE.md")); }

QString AgentDirectory::memoryPath() const { return child(QStringLiteral("memory")); }

QString AgentDirectory::transcriptPath() const { return child(QStringLiteral("transcript")); }

QString AgentDirectory::homePath() const { return child(QStringLiteral("home")); }

QString AgentDirectory::sessionId() const
{
    if (m_name.isEmpty())
    {
        return QString();
    }
    return QUuid::createUuidV5(SESSION_NAMESPACE, m_name.toUtf8()).toString(QUuid::WithoutBraces);
}

QString AgentDirectory::child(const QString& relative) const
{
    if (m_path.isEmpty())
    {
        return QString();
    }
    return m_path + QLatin1Char('/') + relative;
}
