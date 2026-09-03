#include <gtest/gtest.h>

#include "claudecpp/process/streamjsonparser.h"
#include "qtprinters.h"

class StreamJsonParserTest : public ::testing::Test
{
protected:
    StreamJsonParser parser;
};

TEST_F(StreamJsonParserTest, CompleteLine_YieldsOneEvent)
{
    const ParseResult result = parser.feed(R"({"type":"system","subtype":"init"})"
                                           "\n");

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events.at(0).value("type").toString(), QStringLiteral("system"));
    EXPECT_TRUE(result.invalidLines.isEmpty());
}

TEST_F(StreamJsonParserTest, IncompleteLine_IsHeldBack)
{
    const ParseResult result = parser.feed(R"({"type":"sys)");

    EXPECT_TRUE(result.events.isEmpty());
    EXPECT_TRUE(result.invalidLines.isEmpty());
    EXPECT_FALSE(parser.pending().isEmpty());
}

TEST_F(StreamJsonParserTest, LineSplitAcrossChunks_IsReassembled)
{
    EXPECT_TRUE(parser.feed(R"({"type":"assi)").events.isEmpty());
    EXPECT_TRUE(parser.feed(R"(stant","index)").events.isEmpty());

    const ParseResult result = parser.feed("\":3}\n");

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_EQ(result.events.at(0).value("type").toString(), QStringLiteral("assistant"));
    EXPECT_EQ(result.events.at(0).value("index").toInt(), 3);
    EXPECT_TRUE(parser.pending().isEmpty());
}

TEST_F(StreamJsonParserTest, ManyLinesInOneChunk_YieldEventsInOrder)
{
    const ParseResult result = parser.feed("{\"index\":1}\n{\"index\":2}\n{\"index\":3}\n");

    ASSERT_EQ(result.events.size(), 3);
    EXPECT_EQ(result.events.at(0).value("index").toInt(), 1);
    EXPECT_EQ(result.events.at(1).value("index").toInt(), 2);
    EXPECT_EQ(result.events.at(2).value("index").toInt(), 3);
}

TEST_F(StreamJsonParserTest, BlankLines_AreSkipped)
{
    const ParseResult result = parser.feed("\n\n{\"index\":1}\n\n");

    EXPECT_EQ(result.events.size(), 1);
    EXPECT_TRUE(result.invalidLines.isEmpty());
}

TEST_F(StreamJsonParserTest, CarriageReturns_AreTrimmed)
{
    const ParseResult result = parser.feed("{\"index\":1}\r\n");

    ASSERT_EQ(result.events.size(), 1);
    EXPECT_TRUE(result.invalidLines.isEmpty());
}

TEST_F(StreamJsonParserTest, NonJsonLine_IsReportedNotSwallowed)
{
    const ParseResult result = parser.feed("Not logged in - please run /login\n{\"index\":1}\n");

    EXPECT_EQ(result.events.size(), 1);
    ASSERT_EQ(result.invalidLines.size(), 1);
    EXPECT_EQ(result.invalidLines.at(0), QStringLiteral("Not logged in - please run /login"));
}

TEST_F(StreamJsonParserTest, JsonArrayLine_IsInvalidBecauseEventsAreObjects)
{
    const ParseResult result = parser.feed("[1,2,3]\n");

    EXPECT_TRUE(result.events.isEmpty());
    EXPECT_EQ(result.invalidLines.size(), 1);
}

TEST_F(StreamJsonParserTest, ByteAtATime_ProducesTheSameEvents)
{
    const QByteArray stream = "{\"index\":1}\n{\"index\":2}\n";

    QVector<QJsonObject> events;
    for (int i = 0; i < stream.size(); ++i)
    {
        events += parser.feed(stream.mid(i, 1)).events;
    }

    ASSERT_EQ(events.size(), 2);
    EXPECT_EQ(events.at(0).value("index").toInt(), 1);
    EXPECT_EQ(events.at(1).value("index").toInt(), 2);
}

TEST_F(StreamJsonParserTest, Reset_DropsThePartialLine)
{
    parser.feed(R"({"type":"assi)");
    parser.reset();

    EXPECT_TRUE(parser.pending().isEmpty());
    EXPECT_TRUE(parser.feed("{\"index\":1}\n").invalidLines.isEmpty());
}
