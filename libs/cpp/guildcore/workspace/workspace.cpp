#include "workspace.h"

#include <QDir>
#include <QFileInfo>

Workspace::Workspace(const QString& path)
    : m_path(QDir::cleanPath(path))
{
}

Workspace Workspace::fromEnvironment()
{
    const QString home = qEnvironmentVariable("GUILD_HOME");
    if (!home.isEmpty())
    {
        return Workspace(home);
    }
    return Workspace(QDir::homePath() + QStringLiteral("/.guild"));
}

bool Workspace::exists() const { return QFileInfo(m_path).isDir(); }

QString Workspace::path() const { return m_path; }

QString Workspace::agentsPath() const { return m_path + QStringLiteral("/agents"); }

QString Workspace::sharedPromptsPath() const { return m_path + QStringLiteral("/CLAUDE.d"); }

QString Workspace::sharedSkillsPath() const { return m_path + QStringLiteral("/skills"); }

QStringList Workspace::agentNames() const
{
    return QDir(agentsPath()).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
}

bool Workspace::hasAgent(const QString& name) const { return agent(name).isValid(); }

AgentDirectory Workspace::agent(const QString& name) const
{
    return AgentDirectory(name, agentsPath() + QLatin1Char('/') + name);
}
