#include "agentdirectory.h"

#include <QFileInfo>

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

QString AgentDirectory::child(const QString& relative) const
{
    if (m_path.isEmpty())
    {
        return QString();
    }
    return m_path + QLatin1Char('/') + relative;
}
