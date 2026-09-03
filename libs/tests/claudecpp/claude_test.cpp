#include <gtest/gtest.h>

#include "claudecpp/command/claude.h"
#include "qtprinters.h"

class ClaudeTest : public ::testing::Test
{
};

TEST_F(ClaudeTest, Default_HasProgramAndNoArguments)
{
    Claude claude;

    EXPECT_EQ(claude.program(), QStringLiteral("claude"));
    EXPECT_EQ(claude.arguments(), QStringList());
}

TEST_F(ClaudeTest, Flags_AreEmittedInFixedOrder)
{
    Claude claude;
    claude.verbose().outputFormat(OutputFormat::StreamJson).print().model("sonnet");

    EXPECT_EQ(claude.arguments(),
              QStringList({ "--print", "--verbose", "--model", "sonnet", "--output-format",
                            "stream-json" }));
}

TEST_F(ClaudeTest, OutputFormat_MapsToCliNames)
{
    EXPECT_EQ(Claude().outputFormat(OutputFormat::Text).arguments().last(), QStringLiteral("text"));
    EXPECT_EQ(Claude().outputFormat(OutputFormat::Json).arguments().last(), QStringLiteral("json"));
    EXPECT_EQ(Claude().outputFormat(OutputFormat::StreamJson).arguments().last(),
              QStringLiteral("stream-json"));
}

TEST_F(ClaudeTest, SettingSources_JoinedWithComma)
{
    Claude claude;
    claude.settingSources({ "user", "project" });

    EXPECT_EQ(claude.arguments(), QStringList({ "--setting-sources", "user,project" }));
}

TEST_F(ClaudeTest, SettingSources_EmptyListIsNotTheSameAsUnset)
{
    EXPECT_EQ(Claude().arguments(), QStringList());
    EXPECT_EQ(Claude().settingSources({}).arguments(), QStringList({ "--setting-sources", "" }));
}

TEST_F(ClaudeTest, Session_FlagsAreEmittedBeforeTheVariadicTools)
{
    Claude claude;
    claude.tools({ "Read" })
        .forkSession()
        .continueSession()
        .resume("auth-refactor")
        .sessionId("550e8400-e29b-41d4-a716-446655440000");

    EXPECT_EQ(claude.arguments(),
              QStringList({ "--session-id", "550e8400-e29b-41d4-a716-446655440000", "--resume",
                            "auth-refactor", "--continue", "--fork-session", "--tools", "Read" }));
}

TEST_F(ClaudeTest, AddDir_IsRepeatableAndKeepsItsOrder)
{
    Claude claude;
    claude.addDir("/guild").addDir("/project");

    EXPECT_EQ(claude.arguments(), QStringList({ "--add-dir", "/guild", "--add-dir", "/project" }));
}

TEST_F(ClaudeTest, AddDir_ComesBeforeTheVariadicTools)
{
    Claude claude;
    claude.tools({ "Read" }).addDir("/guild");

    EXPECT_EQ(claude.arguments(), QStringList({ "--add-dir", "/guild", "--tools", "Read" }));
}

TEST_F(ClaudeTest, Tools_AreEmittedAsSeparateArguments)
{
    Claude claude;
    claude.tools({ "Read", "Bash" });

    EXPECT_EQ(claude.arguments(), QStringList({ "--tools", "Read", "Bash" }));
}

TEST_F(ClaudeTest, Tools_AlwaysComeLastBecauseTheFlagIsVariadic)
{
    Claude claude;
    claude.tools({ "Read", "Bash" }).print().noSessionPersistence();

    EXPECT_EQ(claude.arguments(),
              QStringList({ "--print", "--no-session-persistence", "--tools", "Read", "Bash" }));
}

TEST_F(ClaudeTest, Tools_EmptyListDisablesEveryTool)
{
    EXPECT_EQ(Claude().tools({}).arguments(), QStringList({ "--tools", "" }));
}

TEST_F(ClaudeTest, SystemPrompt_IsPassedAsOneArgumentWithSpaces)
{
    Claude claude;
    claude.systemPrompt("You are John, a C++ reviewer.");

    EXPECT_EQ(claude.arguments(),
              QStringList({ "--system-prompt", "You are John, a C++ reviewer." }));
}

TEST_F(ClaudeTest, IsolatedInvocation_MatchesTheMeasuredRecipe)
{
    Claude claude;
    claude.print()
        .outputFormat(OutputFormat::StreamJson)
        .verbose()
        .settingSources({})
        .systemPrompt("prompt")
        .disableSlashCommands()
        .strictMcpConfig()
        .noSessionPersistence()
        .tools({ "Read", "Write", "Edit", "Bash" });

    EXPECT_EQ(
        claude.arguments(),
        QStringList({ "--print", "--verbose", "--output-format", "stream-json", "--system-prompt",
                      "prompt", "--setting-sources", "", "--disable-slash-commands",
                      "--strict-mcp-config", "--no-session-persistence", "--tools", "Read", "Write",
                      "Edit", "Bash" }));
}
