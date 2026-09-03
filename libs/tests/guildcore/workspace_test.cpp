#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <gtest/gtest.h>

#include "guildcore/workspace/workspace.h"
#include "qtprinters.h"

class GuildHomeGuard
{
public:
    GuildHomeGuard()
        : m_previous(qgetenv("GUILD_HOME"))
        , m_wasSet(qEnvironmentVariableIsSet("GUILD_HOME"))
    {
    }

    ~GuildHomeGuard()
    {
        if (m_wasSet)
        {
            qputenv("GUILD_HOME", m_previous);
        }
        else
        {
            qunsetenv("GUILD_HOME");
        }
    }

private:
    QByteArray m_previous;
    bool m_wasSet;
};

class WorkspaceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const QString name
            = QString::fromLatin1(::testing::UnitTest::GetInstance()->current_test_info()->name());
        root = QDir(QStringLiteral(GUILD_TEST_SCRATCH_DIR))
                   .filePath(QStringLiteral("workspace-") + name);
        QDir(root).removeRecursively();
        QDir().mkpath(root);
    }

    void TearDown() override { QDir(root).removeRecursively(); }

    void makeDirectory(const QString& relative)
    {
        QDir().mkpath(root + QLatin1Char('/') + relative);
    }

    void makeFile(const QString& relative)
    {
        QFile file(root + QLatin1Char('/') + relative);
        file.open(QIODevice::WriteOnly);
        file.close();
    }

    QString root;
};

TEST_F(WorkspaceTest, FromEnvironment_UsesGuildHomeWhenItIsSet)
{
    const GuildHomeGuard guard;
    qputenv("GUILD_HOME", root.toUtf8());

    EXPECT_EQ(Workspace::fromEnvironment().path(), root);
}

TEST_F(WorkspaceTest, FromEnvironment_FallsBackToDotGuildInTheHomeDirectory)
{
    const GuildHomeGuard guard;
    qunsetenv("GUILD_HOME");

    EXPECT_EQ(Workspace::fromEnvironment().path(), QDir::homePath() + QStringLiteral("/.guild"));
}

TEST_F(WorkspaceTest, SharedPools_SitNextToTheAgents)
{
    const Workspace workspace(root);

    EXPECT_EQ(workspace.agentsPath(), root + QStringLiteral("/agents"));
    EXPECT_EQ(workspace.sharedPromptsPath(), root + QStringLiteral("/CLAUDE.d"));
    EXPECT_EQ(workspace.sharedSkillsPath(), root + QStringLiteral("/.claude/skills"));
}

TEST_F(WorkspaceTest, AgentNames_AreTheDirectoriesUnderAgents)
{
    makeDirectory(QStringLiteral("agents/john"));
    makeDirectory(QStringLiteral("agents/kate"));

    EXPECT_EQ(Workspace(root).agentNames(), QStringList({ "john", "kate" }));
}

TEST_F(WorkspaceTest, AgentNames_IgnoreFilesAndHiddenDirectories)
{
    makeDirectory(QStringLiteral("agents/john"));
    makeDirectory(QStringLiteral("agents/.cache"));
    makeFile(QStringLiteral("agents/README.md"));

    EXPECT_EQ(Workspace(root).agentNames(), QStringList({ "john" }));
}

TEST_F(WorkspaceTest, AgentNames_AreEmptyWhenTheWorkspaceDoesNotExist)
{
    const Workspace workspace(root + QStringLiteral("/missing"));

    EXPECT_FALSE(workspace.exists());
    EXPECT_TRUE(workspace.agentNames().isEmpty());
}

TEST_F(WorkspaceTest, ReadingTheWorkspace_CreatesNothing)
{
    const Workspace workspace(root + QStringLiteral("/missing"));

    workspace.agentNames();
    workspace.hasAgent(QStringLiteral("john"));

    EXPECT_FALSE(QFileInfo::exists(workspace.path()));
}

TEST_F(WorkspaceTest, Agent_ExposesTheFilesOfItsOwnDirectory)
{
    makeDirectory(QStringLiteral("agents/john"));

    const AgentDirectory john = Workspace(root).agent(QStringLiteral("john"));
    const QString expected = root + QStringLiteral("/agents/john");

    EXPECT_TRUE(john.isValid());
    EXPECT_EQ(john.name(), QStringLiteral("john"));
    EXPECT_EQ(john.path(), expected);
    EXPECT_EQ(john.configFile(), expected + QStringLiteral("/agent.toml"));
    EXPECT_EQ(john.promptFile(), expected + QStringLiteral("/CLAUDE.md"));
    EXPECT_EQ(john.memoryPath(), expected + QStringLiteral("/memory"));
    EXPECT_EQ(john.transcriptPath(), expected + QStringLiteral("/transcript"));
}

TEST_F(WorkspaceTest, UnknownAgent_IsNotValidButStillNamesItsPlace)
{
    const Workspace workspace(root);
    const AgentDirectory kate = workspace.agent(QStringLiteral("kate"));

    EXPECT_FALSE(workspace.hasAgent(QStringLiteral("kate")));
    EXPECT_FALSE(kate.isValid());
    EXPECT_EQ(kate.path(), root + QStringLiteral("/agents/kate"));
}

TEST_F(WorkspaceTest, AgentName_WithASeparator_ResolvesToNothing)
{
    makeDirectory(QStringLiteral("agents/john"));

    const AgentDirectory escaped = Workspace(root).agent(QStringLiteral("../../john"));

    EXPECT_FALSE(escaped.isValid());
    EXPECT_TRUE(escaped.path().isEmpty());
    EXPECT_TRUE(escaped.configFile().isEmpty());
}

TEST_F(WorkspaceTest, AgentName_ThatIsHidden_ResolvesToNothing)
{
    makeDirectory(QStringLiteral("agents/.cache"));

    EXPECT_FALSE(Workspace(root).agent(QStringLiteral(".cache")).isValid());
    EXPECT_FALSE(Workspace(root).agent(QStringLiteral("..")).isValid());
}

TEST_F(WorkspaceTest, DefaultConstructedAgentDirectory_IsInvalid)
{
    const AgentDirectory nobody;

    EXPECT_FALSE(nobody.isValid());
    EXPECT_TRUE(nobody.name().isEmpty());
    EXPECT_TRUE(nobody.path().isEmpty());
    EXPECT_TRUE(nobody.promptFile().isEmpty());
}
