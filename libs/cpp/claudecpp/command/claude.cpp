#include "claude.h"

static QString outputFormatName(OutputFormat format)
{
    switch (format)
    {
        case OutputFormat::Text:
            return QStringLiteral("text");
        case OutputFormat::Json:
            return QStringLiteral("json");
        case OutputFormat::StreamJson:
            return QStringLiteral("stream-json");
    }
    return QString();
}

Claude& Claude::print()
{
    m_print = true;
    return *this;
}

Claude& Claude::verbose()
{
    m_verbose = true;
    return *this;
}

Claude& Claude::model(const QString& name)
{
    m_model = name;
    return *this;
}

Claude& Claude::outputFormat(OutputFormat format)
{
    m_outputFormat = format;
    return *this;
}

Claude& Claude::systemPrompt(const QString& prompt)
{
    m_systemPrompt = prompt;
    return *this;
}

Claude& Claude::appendSystemPrompt(const QString& prompt)
{
    m_appendSystemPrompt = prompt;
    return *this;
}

Claude& Claude::settingSources(const QStringList& sources)
{
    m_settingSources = sources;
    return *this;
}

Claude& Claude::jsonSchema(const QString& schema)
{
    m_jsonSchema = schema;
    return *this;
}

Claude& Claude::disableSlashCommands()
{
    m_disableSlashCommands = true;
    return *this;
}

Claude& Claude::strictMcpConfig()
{
    m_strictMcpConfig = true;
    return *this;
}

Claude& Claude::noSessionPersistence()
{
    m_noSessionPersistence = true;
    return *this;
}

Claude& Claude::skipPermissions()
{
    m_skipPermissions = true;
    return *this;
}

Claude& Claude::tools(const QStringList& names)
{
    m_tools = names;
    return *this;
}

QString Claude::program() const { return QStringLiteral("claude"); }

QStringList Claude::arguments() const
{
    QStringList args;

    if (m_print)
    {
        args << QStringLiteral("--print");
    }
    if (m_verbose)
    {
        args << QStringLiteral("--verbose");
    }
    if (m_model)
    {
        args << QStringLiteral("--model") << *m_model;
    }
    if (m_outputFormat)
    {
        args << QStringLiteral("--output-format") << outputFormatName(*m_outputFormat);
    }
    if (m_systemPrompt)
    {
        args << QStringLiteral("--system-prompt") << *m_systemPrompt;
    }
    if (m_appendSystemPrompt)
    {
        args << QStringLiteral("--append-system-prompt") << *m_appendSystemPrompt;
    }
    if (m_settingSources)
    {
        args << QStringLiteral("--setting-sources") << m_settingSources->join(QLatin1Char(','));
    }
    if (m_jsonSchema)
    {
        args << QStringLiteral("--json-schema") << *m_jsonSchema;
    }
    if (m_disableSlashCommands)
    {
        args << QStringLiteral("--disable-slash-commands");
    }
    if (m_strictMcpConfig)
    {
        args << QStringLiteral("--strict-mcp-config");
    }
    if (m_noSessionPersistence)
    {
        args << QStringLiteral("--no-session-persistence");
    }
    if (m_skipPermissions)
    {
        args << QStringLiteral("--dangerously-skip-permissions");
    }
    if (m_tools)
    {
        args << QStringLiteral("--tools");
        if (m_tools->isEmpty())
        {
            args << QString(QLatin1String(""));
        }
        else
        {
            args << *m_tools;
        }
    }

    return args;
}
