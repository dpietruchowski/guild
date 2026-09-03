#pragma once
#include <QString>

#include "claudecpp/command/dockerexec.h"
#include "claudecpp/command/dockerrun.h"
#include "guildcore/workspace/agentdirectory.h"
#include "guildcore/workspace/workspace.h"

class AgentContainer
{
public:
    AgentContainer(const Workspace& workspace, const AgentDirectory& agent, const QString& image);

    static QString imageFromEnvironment();

    QString name() const;
    QString workdir() const;
    QString sharedDirectory() const;

    DockerRun createCommand() const;
    DockerExec execCommand(const Command& command) const;

private:
    Workspace m_workspace;
    AgentDirectory m_agent;
    QString m_image;
};
