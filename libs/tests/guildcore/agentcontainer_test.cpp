#include <QDir>
#include <gtest/gtest.h>

#include "claudecpp/command/claude.h"
#include "guildcore/agent/agentcontainer.h"
#include "qtprinters.h"

class AgentContainerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const QString name
            = QString::fromLatin1(::testing::UnitTest::GetInstance()->current_test_info()->name());
        root = QDir(QStringLiteral(GUILD_TEST_SCRATCH_DIR))
                   .filePath(QStringLiteral("container-") + name);
        QDir(root).removeRecursively();
        QDir().mkpath(root + QStringLiteral("/agents/john"));
    }

    void TearDown() override { QDir(root).removeRecursively(); }

    void makeSharedSkills() { QDir().mkpath(root + QStringLiteral("/.claude/skills")); }

    AgentContainer container() const
    {
        const Workspace workspace(root);
        return AgentContainer(workspace, workspace.agent(QStringLiteral("john")),
                              QStringLiteral("guild-agent:latest"));
    }

    QString root;
};

TEST_F(AgentContainerTest, Name_IsDerivedFromTheAgent)
{
    EXPECT_EQ(container().name(), QStringLiteral("guild-john"));
}

TEST_F(AgentContainerTest, Create_MountsTheAgentDirectoryAndSleeps)
{
    EXPECT_EQ(
        container().createCommand().arguments(),
        QStringList({ "run", "--detach", "--name", "guild-john", "--env", "HOME=/home/agent",
                      "--env", "CLAUDE_CODE_OAUTH_TOKEN", "--volume", root + "/agents/john:/work",
                      "--workdir", "/work", "guild-agent:latest", "sleep", "infinity" }));
}

TEST_F(AgentContainerTest, Create_MountsTheSharedPoolReadOnlyWhenItExists)
{
    makeSharedSkills();

    EXPECT_TRUE(container().createCommand().arguments().contains(
        root + "/.claude/skills:/guild/.claude/skills:ro"));
}

TEST_F(AgentContainerTest, Create_SkipsTheSharedPoolWhenThereIsNone)
{
    const QStringList arguments = container().createCommand().arguments();

    EXPECT_EQ(arguments.count(QStringLiteral("--volume")), 1);
}

TEST_F(AgentContainerTest, Create_NeverPutsTheTokenValueInArgv)
{
    for (const QString& argument : container().createCommand().arguments())
    {
        EXPECT_FALSE(argument.contains(QStringLiteral("CLAUDE_CODE_OAUTH_TOKEN=")));
    }
}

TEST_F(AgentContainerTest, Exec_WrapsTheCommandInTheAgentsContainer)
{
    Claude claude;
    claude.print().outputFormat(OutputFormat::StreamJson).verbose();

    EXPECT_EQ(container().execCommand(claude).arguments(),
              QStringList({ "exec", "--interactive", "--workdir", "/work", "guild-john", "claude",
                            "--print", "--verbose", "--output-format", "stream-json" }));
}

TEST_F(AgentContainerTest, SharedDirectory_IsWhatAddDirShouldName)
{
    makeSharedSkills();

    const AgentContainer agent = container();

    EXPECT_TRUE(agent.createCommand().arguments().contains(
        root + "/.claude/skills:" + agent.sharedDirectory() + "/.claude/skills:ro"));
}

TEST_F(AgentContainerTest, ImageFromEnvironment_FallsBackToTheDefault)
{
    const QByteArray previous = qgetenv("GUILD_IMAGE");
    const bool wasSet = qEnvironmentVariableIsSet("GUILD_IMAGE");

    qunsetenv("GUILD_IMAGE");
    EXPECT_EQ(AgentContainer::imageFromEnvironment(), QStringLiteral("guild-agent:latest"));

    qputenv("GUILD_IMAGE", "guild-agent:dev");
    EXPECT_EQ(AgentContainer::imageFromEnvironment(), QStringLiteral("guild-agent:dev"));

    if (wasSet)
    {
        qputenv("GUILD_IMAGE", previous);
    }
    else
    {
        qunsetenv("GUILD_IMAGE");
    }
}
