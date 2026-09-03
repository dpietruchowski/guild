#include "agentcontainer.h"

#include <QFileInfo>

static const QString WORKDIR = QStringLiteral("/work");
static const QString SHARED_DIRECTORY = QStringLiteral("/guild");
static const QString CONTAINER_HOME = QStringLiteral("/home/agent");
static const QString TOKEN_VARIABLE = QStringLiteral("CLAUDE_CODE_OAUTH_TOKEN");

AgentContainer::AgentContainer(const Workspace& workspace, const AgentDirectory& agent,
                               const QString& image)
    : m_workspace(workspace)
    , m_agent(agent)
    , m_image(image)
{
}

QString AgentContainer::imageFromEnvironment()
{
    const QString image = qEnvironmentVariable("GUILD_IMAGE");
    if (!image.isEmpty())
    {
        return image;
    }
    return QStringLiteral("guild-agent:latest");
}

QString AgentContainer::name() const { return QStringLiteral("guild-") + m_agent.name(); }

QString AgentContainer::workdir() const { return WORKDIR; }

QString AgentContainer::sharedDirectory() const { return SHARED_DIRECTORY; }

bool AgentContainer::hasSharedPool() const
{
    return QFileInfo(m_workspace.sharedSkillsPath()).isDir();
}

DockerRun AgentContainer::createCommand() const
{
    DockerRun run(m_image);
    run.name(name())
        .detach()
        .env(QStringLiteral("HOME"), CONTAINER_HOME)
        .passEnv(TOKEN_VARIABLE)
        .mount(m_agent.path(), WORKDIR)
        .mount(m_agent.homePath(), CONTAINER_HOME);

    if (hasSharedPool())
    {
        run.mount(m_workspace.sharedSkillsPath(),
                  SHARED_DIRECTORY + QStringLiteral("/.claude/skills"), true);
    }

    run.workdir(WORKDIR).run(QStringLiteral("sleep"), { QStringLiteral("infinity") });
    return run;
}

DockerExec AgentContainer::execCommand(const Command& command) const
{
    DockerExec exec(name());
    exec.interactive().workdir(WORKDIR).run(command);
    return exec;
}
