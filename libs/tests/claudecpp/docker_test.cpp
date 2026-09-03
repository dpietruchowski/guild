#include <gtest/gtest.h>

#include "claudecpp/command/claude.h"
#include "claudecpp/command/dockerexec.h"
#include "claudecpp/command/dockerrun.h"
#include "qtprinters.h"

class DockerExecTest : public ::testing::Test
{
};

class DockerRunTest : public ::testing::Test
{
};

TEST_F(DockerExecTest, Bare_NamesTheContainer)
{
    DockerExec exec("guild-john");

    EXPECT_EQ(exec.program(), QStringLiteral("docker"));
    EXPECT_EQ(exec.arguments(), QStringList({ "exec", "guild-john" }));
}

TEST_F(DockerExecTest, Options_ComeBeforeTheContainer)
{
    DockerExec exec("guild-john");
    exec.interactive().workdir("/work").env("GUILD_TURN", "7");

    EXPECT_EQ(exec.arguments(),
              QStringList({ "exec", "--interactive", "--workdir", "/work", "--env", "GUILD_TURN=7",
                            "guild-john" }));
}

TEST_F(DockerExecTest, Run_AppendsTheWrappedCommandAfterTheContainer)
{
    Claude claude;
    claude.print().outputFormat(OutputFormat::StreamJson);

    DockerExec exec("guild-john");
    exec.interactive().run(claude);

    EXPECT_EQ(exec.arguments(),
              QStringList({ "exec", "--interactive", "guild-john", "claude", "--print",
                            "--output-format", "stream-json" }));
}

TEST_F(DockerExecTest, Run_CopiesTheWrappedCommand)
{
    DockerExec exec("guild-john");

    {
        Claude claude;
        claude.print();
        exec.run(claude);
    }

    EXPECT_EQ(exec.arguments(), QStringList({ "exec", "guild-john", "claude", "--print" }));
}

TEST_F(DockerRunTest, LongLivedAgentContainer)
{
    DockerRun run("guild-agent");
    run.name("guild-john")
        .detach()
        .env("CLAUDE_CODE_OAUTH_TOKEN", "sk-ant-oat01-secret")
        .mount("/home/damian/workspace/guild/agents/john", "/work")
        .mount("/home/damian/workspace/fillin-app", "/project")
        .workdir("/work")
        .run("sleep", { "infinity" });

    EXPECT_EQ(run.program(), QStringLiteral("docker"));
    EXPECT_EQ(run.arguments(),
              QStringList({ "run", "--detach", "--name", "guild-john", "--env",
                            "CLAUDE_CODE_OAUTH_TOKEN=sk-ant-oat01-secret", "--volume",
                            "/home/damian/workspace/guild/agents/john:/work", "--volume",
                            "/home/damian/workspace/fillin-app:/project", "--workdir", "/work",
                            "guild-agent", "sleep", "infinity" }));
}

TEST_F(DockerRunTest, PassEnv_SendsTheKeyWithoutItsValue)
{
    DockerRun run("guild-agent");
    run.passEnv("CLAUDE_CODE_OAUTH_TOKEN");

    EXPECT_EQ(run.arguments(),
              QStringList({ "run", "--env", "CLAUDE_CODE_OAUTH_TOKEN", "guild-agent" }));
}

TEST_F(DockerExecTest, PassEnv_SendsTheKeyWithoutItsValue)
{
    DockerExec exec("guild-john");
    exec.passEnv("CLAUDE_CODE_OAUTH_TOKEN");

    EXPECT_EQ(exec.arguments(),
              QStringList({ "exec", "--env", "CLAUDE_CODE_OAUTH_TOKEN", "guild-john" }));
}

TEST_F(DockerRunTest, ReadOnlyMount_GetsTheRoSuffix)
{
    DockerRun run("guild-agent");
    run.mount("/opt/skills", "/skills", true);

    EXPECT_EQ(run.arguments(),
              QStringList({ "run", "--volume", "/opt/skills:/skills:ro", "guild-agent" }));
}

TEST_F(DockerRunTest, Limits_AreEmittedBeforeTheImage)
{
    DockerRun run("guild-agent");
    run.memory("2g").cpus("1.5").network("none");

    EXPECT_EQ(run.arguments(),
              QStringList({ "run", "--memory", "2g", "--cpus", "1.5", "--network", "none",
                            "guild-agent" }));
}

TEST_F(DockerRunTest, OneShotRunOfClaude)
{
    Claude claude;
    claude.print().settingSources({}).tools({ "Read" });

    DockerRun run("guild-agent");
    run.removeOnExit().interactive().mount("/agents/john", "/work").workdir("/work").run(claude);

    EXPECT_EQ(run.arguments(),
              QStringList({ "run", "--rm", "--interactive", "--volume", "/agents/john:/work",
                            "--workdir", "/work", "guild-agent", "claude", "--print",
                            "--setting-sources", "", "--tools", "Read" }));
}
