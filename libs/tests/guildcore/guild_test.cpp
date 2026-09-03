#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <gtest/gtest.h>

#include "guildcore/guild.h"
#include "qtprinters.h"

class GuildTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const QString name
            = QString::fromLatin1(::testing::UnitTest::GetInstance()->current_test_info()->name());
        root = QDir(QStringLiteral(GUILD_TEST_SCRATCH_DIR))
                   .filePath(QStringLiteral("guild-") + name);
        QDir(root).removeRecursively();
        QDir().mkpath(root + QStringLiteral("/agents/john"));
        QDir().mkpath(root + QStringLiteral("/bin"));

        writeFakeDocker();

        previousPath = qgetenv("PATH");
        qputenv("PATH", (root + QStringLiteral("/bin:")).toUtf8() + previousPath);
    }

    void TearDown() override
    {
        qputenv("PATH", previousPath);
        QDir(root).removeRecursively();
    }

    void writeFakeDocker()
    {
        const QString script = QStringLiteral("#!/bin/sh\n"
                                              ": > %1/argv.txt\n"
                                              "for arg in \"$@\"\n"
                                              "do\n"
                                              "    printf '%s\\n' \"$arg\" >> %1/argv.txt\n"
                                              "done\n"
                                              "cat > %1/stdin.txt\n"
                                              "printf '{\"type\":\"assistant\",\"index\":1}\\n'\n")
                                   .arg(root);

        const QString path = root + QStringLiteral("/bin/docker");
        QFile file(path);
        file.open(QIODevice::WriteOnly);
        file.write(script.toUtf8());
        file.close();
        file.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                            | QFile::ReadGroup | QFile::ExeGroup | QFile::ReadOther
                            | QFile::ExeOther);
    }

    QStringList recordedArguments() const
    {
        return readFile(QStringLiteral("argv.txt")).split('\n');
    }

    QString recordedInput() const { return readFile(QStringLiteral("stdin.txt")); }

    QString readFile(const QString& name) const
    {
        QFile file(root + QLatin1Char('/') + name);
        if (!file.open(QIODevice::ReadOnly))
        {
            return QString();
        }
        return QString::fromUtf8(file.readAll()).trimmed();
    }

    void makeSharedSkills() { QDir().mkpath(root + QStringLiteral("/.claude/skills")); }

    QString root;
    QByteArray previousPath;
};

TEST_F(GuildTest, Agents_ComeFromTheWorkspace)
{
    QDir().mkpath(root + QStringLiteral("/agents/kate"));

    Guild guild((Workspace(root)));

    EXPECT_EQ(guild.agents(), QStringList({ "john", "kate" }));
}

TEST_F(GuildTest, Run_ExecsClaudeInTheAgentsContainer)
{
    Guild guild((Workspace(root)));
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("hello"));

    ASSERT_TRUE(finished.wait());
    const QStringList arguments = recordedArguments();
    EXPECT_EQ(arguments.first(), QStringLiteral("exec"));
    EXPECT_TRUE(arguments.contains(QStringLiteral("guild-john")));
    EXPECT_TRUE(arguments.contains(QStringLiteral("claude")));
    EXPECT_TRUE(arguments.contains(QStringLiteral("--output-format")));
    EXPECT_TRUE(arguments.contains(QStringLiteral("stream-json")));
}

TEST_F(GuildTest, Run_SendsThePromptOnStdin)
{
    Guild guild((Workspace(root)));
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("write a haiku about ninja"));

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(recordedInput(), QStringLiteral("write a haiku about ninja"));
    EXPECT_FALSE(recordedArguments().contains(QStringLiteral("write a haiku about ninja")));
}

TEST_F(GuildTest, Run_ForwardsTheParsedEvents)
{
    Guild guild((Workspace(root)));
    QSignalSpy messages(&guild, &Guild::messageReceived);
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("hello"));

    ASSERT_TRUE(finished.wait());
    ASSERT_EQ(messages.count(), 1);
    EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("type").toString(),
              QStringLiteral("assistant"));
}

TEST_F(GuildTest, Run_WithASharedPool_NamesItWithAddDir)
{
    makeSharedSkills();

    Guild guild((Workspace(root)));
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("hello"));

    ASSERT_TRUE(finished.wait());
    const QStringList arguments = recordedArguments();
    const int flag = arguments.indexOf(QStringLiteral("--add-dir"));
    ASSERT_NE(flag, -1);
    EXPECT_EQ(arguments.at(flag + 1), QStringLiteral("/guild"));
}

TEST_F(GuildTest, Run_WithoutASharedPool_PassesNoAddDir)
{
    Guild guild((Workspace(root)));
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("hello"));

    ASSERT_TRUE(finished.wait());
    EXPECT_FALSE(recordedArguments().contains(QStringLiteral("--add-dir")));
}

TEST_F(GuildTest, Up_CreatesTheDetachedContainer)
{
    Guild guild((Workspace(root)));
    QSignalSpy finished(&guild, &Guild::finished);

    guild.up(QStringLiteral("john"));

    ASSERT_TRUE(finished.wait());
    const QStringList arguments = recordedArguments();
    EXPECT_EQ(arguments.first(), QStringLiteral("run"));
    EXPECT_TRUE(arguments.contains(QStringLiteral("--detach")));
    EXPECT_TRUE(arguments.contains(QStringLiteral("guild-john")));
    EXPECT_TRUE(arguments.contains(root + "/agents/john:/work"));
}

TEST_F(GuildTest, UnknownAgent_FailsWithoutRunningAnything)
{
    Guild guild((Workspace(root)));
    QSignalSpy failed(&guild, &Guild::failed);

    guild.run(QStringLiteral("kate"), QStringLiteral("hello"));

    ASSERT_EQ(failed.count(), 1);
    EXPECT_EQ(failed.at(0).at(0).toString(), QStringLiteral("unknown agent: kate"));
    EXPECT_FALSE(QFile::exists(root + QStringLiteral("/argv.txt")));
}

TEST_F(GuildTest, AgentNameThatEscapes_IsUnknown)
{
    Guild guild((Workspace(root)));
    QSignalSpy failed(&guild, &Guild::failed);

    guild.run(QStringLiteral("../../john"), QStringLiteral("hello"));

    ASSERT_EQ(failed.count(), 1);
    EXPECT_FALSE(QFile::exists(root + QStringLiteral("/argv.txt")));
}

TEST_F(GuildTest, SecondRunWhileTheFirstIsAlive_IsRefused)
{
    Guild guild((Workspace(root)));
    QSignalSpy failed(&guild, &Guild::failed);
    QSignalSpy finished(&guild, &Guild::finished);

    guild.run(QStringLiteral("john"), QStringLiteral("hello"));
    guild.run(QStringLiteral("john"), QStringLiteral("hello again"));

    ASSERT_EQ(failed.count(), 1);
    EXPECT_EQ(failed.at(0).at(0).toString(), QStringLiteral("already running"));
    EXPECT_TRUE(finished.wait());
}
