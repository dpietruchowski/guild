#pragma once
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>

#include "claudecpp/command/command.h"
#include "streamjsonparser.h"

class CommandRunner : public QObject
{
    Q_OBJECT

public:
    explicit CommandRunner(QObject* parent = nullptr);

    void setWorkingDirectory(const QString& path);
    void setEnvironmentVariable(const QString& key, const QString& value);

    void start(const Command& command);
    void writeInput(const QString& text);
    void closeInput();
    void cancel();

    bool isRunning() const;
    QString standardError() const;

signals:
    void started();
    void messageReceived(const QJsonObject& message);
    void invalidLineReceived(const QString& line);
    void finished(int exitCode);
    void failed(const QString& reason);

private:
    void readStandardOutput();
    void readStandardError();
    void handleFinished(int exitCode, QProcess::ExitStatus status);
    void handleError(QProcess::ProcessError error);
    void emitResult(const ParseResult& result);

    QProcess m_process;
    StreamJsonParser m_parser;
    QProcessEnvironment m_environment;
    QString m_standardError;
    bool m_cancelled = false;
};
