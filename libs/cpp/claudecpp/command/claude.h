#pragma once
#include <QString>
#include <QStringList>
#include <optional>

#include "command.h"

enum class OutputFormat
{
    Text,
    Json,
    StreamJson
};

class Claude : public Command
{
public:
    Claude& print();
    Claude& verbose();
    Claude& model(const QString& name);
    Claude& outputFormat(OutputFormat format);
    Claude& systemPrompt(const QString& prompt);
    Claude& appendSystemPrompt(const QString& prompt);
    Claude& settingSources(const QStringList& sources);
    Claude& jsonSchema(const QString& schema);
    Claude& disableSlashCommands();
    Claude& strictMcpConfig();
    Claude& noSessionPersistence();
    Claude& skipPermissions();
    Claude& tools(const QStringList& names);

    QString program() const override;
    QStringList arguments() const override;

private:
    bool m_print = false;
    bool m_verbose = false;
    bool m_disableSlashCommands = false;
    bool m_strictMcpConfig = false;
    bool m_noSessionPersistence = false;
    bool m_skipPermissions = false;
    std::optional<QString> m_model;
    std::optional<OutputFormat> m_outputFormat;
    std::optional<QString> m_systemPrompt;
    std::optional<QString> m_appendSystemPrompt;
    std::optional<QStringList> m_settingSources;
    std::optional<QString> m_jsonSchema;
    std::optional<QStringList> m_tools;
};
