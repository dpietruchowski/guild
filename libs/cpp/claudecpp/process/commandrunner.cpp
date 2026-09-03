#include "commandrunner.h"

CommandRunner::CommandRunner(QObject* parent)
    : QObject(parent)
    , m_environment(QProcessEnvironment::systemEnvironment())
{
    connect(&m_process, &QProcess::started, this, &CommandRunner::started);
    connect(&m_process, &QProcess::readyReadStandardOutput, this,
            &CommandRunner::readStandardOutput);
    connect(&m_process, &QProcess::readyReadStandardError, this, &CommandRunner::readStandardError);
    connect(&m_process, &QProcess::finished, this, &CommandRunner::handleFinished);
    connect(&m_process, &QProcess::errorOccurred, this, &CommandRunner::handleError);
}

void CommandRunner::setWorkingDirectory(const QString& path)
{
    m_process.setWorkingDirectory(path);
}

void CommandRunner::setEnvironmentVariable(const QString& key, const QString& value)
{
    m_environment.insert(key, value);
}

void CommandRunner::start(const Command& command)
{
    m_parser.reset();
    m_standardError.clear();
    m_cancelled = false;
    m_process.setProcessEnvironment(m_environment);
    m_process.start(command.program(), command.arguments());
}

void CommandRunner::writeInput(const QString& text) { m_process.write(text.toUtf8()); }

void CommandRunner::closeInput() { m_process.closeWriteChannel(); }

void CommandRunner::cancel()
{
    m_cancelled = true;
    m_process.terminate();
}

bool CommandRunner::isRunning() const { return m_process.state() != QProcess::NotRunning; }

QString CommandRunner::standardError() const { return m_standardError; }

void CommandRunner::readStandardOutput()
{
    emitResult(m_parser.feed(m_process.readAllStandardOutput()));
}

void CommandRunner::readStandardError()
{
    m_standardError += QString::fromUtf8(m_process.readAllStandardError());
}

void CommandRunner::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    readStandardOutput();
    emitResult(m_parser.feed("\n"));
    m_parser.reset();

    if (m_cancelled)
    {
        emit failed(QStringLiteral("cancelled"));
        return;
    }
    if (status == QProcess::CrashExit)
    {
        emit failed(QStringLiteral("process crashed"));
        return;
    }

    emit finished(exitCode);
}

void CommandRunner::handleError(QProcess::ProcessError error)
{
    if (error == QProcess::Crashed)
    {
        return;
    }

    emit failed(m_process.errorString());
}

void CommandRunner::emitResult(const ParseResult& result)
{
    for (const QJsonObject& message : result.events)
    {
        emit messageReceived(message);
    }
    for (const QString& line : result.invalidLines)
    {
        emit invalidLineReceived(line);
    }
}
