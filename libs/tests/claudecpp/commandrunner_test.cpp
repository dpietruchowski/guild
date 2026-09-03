#include <QSignalSpy>
#include <gtest/gtest.h>

#include "claudecpp/process/commandrunner.h"
#include "qtprinters.h"

class ShellCommand : public Command
{
public:
    ShellCommand(const QString& program, const QStringList& arguments)
        : m_program(program)
        , m_arguments(arguments)
    {
    }

    QString program() const override { return m_program; }
    QStringList arguments() const override { return m_arguments; }

private:
    QString m_program;
    QStringList m_arguments;
};

class CommandRunnerTest : public ::testing::Test
{
protected:
    CommandRunner runner;
};

TEST_F(CommandRunnerTest, StdinIsEchoed_YieldsOneMessagePerLine)
{
    QSignalSpy messages(&runner, &CommandRunner::messageReceived);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("cat", {}));
    runner.writeInput("{\"index\":1}\n{\"index\":2}\n");
    runner.closeInput();

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(finished.at(0).at(0).toInt(), 0);
    ASSERT_EQ(messages.count(), 2);
    EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("index").toInt(), 1);
    EXPECT_EQ(messages.at(1).at(0).toJsonObject().value("index").toInt(), 2);
}

TEST_F(CommandRunnerTest, Started_IsEmittedBeforeOutput)
{
    QSignalSpy started(&runner, &CommandRunner::started);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("cat", {}));
    runner.closeInput();

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(started.count(), 1);
}

TEST_F(CommandRunnerTest, LastLineWithoutNewline_IsStillAMessage)
{
    QSignalSpy messages(&runner, &CommandRunner::messageReceived);
    QSignalSpy invalid(&runner, &CommandRunner::invalidLineReceived);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("printf", { "%s", "{\"index\":7}" }));

    ASSERT_TRUE(finished.wait());
    ASSERT_EQ(messages.count(), 1);
    EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("index").toInt(), 7);
    EXPECT_EQ(invalid.count(), 0);
}

TEST_F(CommandRunnerTest, NonJsonOutput_IsReportedSeparately)
{
    QSignalSpy messages(&runner, &CommandRunner::messageReceived);
    QSignalSpy invalid(&runner, &CommandRunner::invalidLineReceived);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("printf", { "%s\\n", "Not logged in" }));

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(messages.count(), 0);
    ASSERT_EQ(invalid.count(), 1);
    EXPECT_EQ(invalid.at(0).at(0).toString(), QStringLiteral("Not logged in"));
}

TEST_F(CommandRunnerTest, ExitCode_IsPassedThrough)
{
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("sh", { "-c", "exit 3" }));

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(finished.at(0).at(0).toInt(), 3);
}

TEST_F(CommandRunnerTest, StandardError_IsCollected)
{
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.start(ShellCommand("sh", { "-c", "echo boom 1>&2" }));

    ASSERT_TRUE(finished.wait());
    EXPECT_EQ(runner.standardError().trimmed(), QStringLiteral("boom"));
}

TEST_F(CommandRunnerTest, MissingProgram_Fails)
{
    QSignalSpy failed(&runner, &CommandRunner::failed);

    runner.start(ShellCommand("guild-no-such-binary", {}));

    ASSERT_TRUE(failed.wait());
    EXPECT_FALSE(failed.at(0).at(0).toString().isEmpty());
}

TEST_F(CommandRunnerTest, Cancel_FailsWithoutLookingLikeACrash)
{
    QSignalSpy started(&runner, &CommandRunner::started);
    QSignalSpy failed(&runner, &CommandRunner::failed);

    runner.start(ShellCommand("sleep", { "30" }));
    ASSERT_TRUE(started.wait());

    runner.cancel();

    ASSERT_TRUE(failed.wait());
    EXPECT_EQ(failed.at(0).at(0).toString(), QStringLiteral("cancelled"));
    EXPECT_FALSE(runner.isRunning());
}

TEST_F(CommandRunnerTest, Environment_ReachesTheProcess)
{
    QSignalSpy messages(&runner, &CommandRunner::messageReceived);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.setEnvironmentVariable("GUILD_AGENT", "john");
    runner.start(ShellCommand("sh", { "-c", "printf '{\"agent\":\"%s\"}\\n' \"$GUILD_AGENT\"" }));

    ASSERT_TRUE(finished.wait());
    ASSERT_EQ(messages.count(), 1);
    EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("agent").toString(),
              QStringLiteral("john"));
}

TEST_F(CommandRunnerTest, WorkingDirectory_ReachesTheProcess)
{
    QSignalSpy messages(&runner, &CommandRunner::messageReceived);
    QSignalSpy finished(&runner, &CommandRunner::finished);

    runner.setWorkingDirectory("/");
    runner.start(ShellCommand("sh", { "-c", "printf '{\"cwd\":\"%s\"}\\n' \"$PWD\"" }));

    ASSERT_TRUE(finished.wait());
    ASSERT_EQ(messages.count(), 1);
    EXPECT_EQ(messages.at(0).at(0).toJsonObject().value("cwd").toString(), QStringLiteral("/"));
}
