#include "guild.h"

#include "claudecpp/command/claude.h"
#include "guildcore/agent/agentcontainer.h"

Guild::Guild(const Workspace& workspace, QObject* parent)
    : QObject(parent)
    , m_workspace(workspace)
    , m_image(AgentContainer::imageFromEnvironment())
{
    connect(&m_runner, &CommandRunner::messageReceived, this, &Guild::messageReceived);
    connect(&m_runner, &CommandRunner::invalidLineReceived, this, &Guild::invalidLineReceived);
    connect(&m_runner, &CommandRunner::finished, this, &Guild::finished);
    connect(&m_runner, &CommandRunner::failed, this, &Guild::failed);
}

QStringList Guild::agents() const { return m_workspace.agentNames(); }

bool Guild::isBusy() const { return m_runner.isRunning(); }

void Guild::up(const QString& agent)
{
    const AgentDirectory directory = m_workspace.agent(agent);
    if (!directory.isValid())
    {
        emit failed(QStringLiteral("unknown agent: ") + agent);
        return;
    }
    if (isBusy())
    {
        emit failed(QStringLiteral("already running"));
        return;
    }

    m_runner.start(AgentContainer(m_workspace, directory, m_image).createCommand());
    m_runner.closeInput();
}

void Guild::run(const QString& agent, const QString& prompt)
{
    const AgentDirectory directory = m_workspace.agent(agent);
    if (!directory.isValid())
    {
        emit failed(QStringLiteral("unknown agent: ") + agent);
        return;
    }
    if (isBusy())
    {
        emit failed(QStringLiteral("already running"));
        return;
    }

    const AgentContainer container(m_workspace, directory, m_image);

    Claude claude;
    claude.print()
        .verbose()
        .outputFormat(OutputFormat::StreamJson)
        .noSessionPersistence()
        .skipPermissions();

    if (container.hasSharedPool())
    {
        claude.addDir(container.sharedDirectory());
    }

    m_runner.start(container.execCommand(claude));
    m_runner.writeInput(prompt);
    m_runner.closeInput();
}

void Guild::cancel() { m_runner.cancel(); }
