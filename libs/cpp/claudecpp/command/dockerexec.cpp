#include "dockerexec.h"

DockerExec::DockerExec(const QString& container)
    : m_container(container)
{
}

DockerExec& DockerExec::interactive()
{
    m_interactive = true;
    return *this;
}

DockerExec& DockerExec::workdir(const QString& path)
{
    m_workdir = path;
    return *this;
}

DockerExec& DockerExec::env(const QString& key, const QString& value)
{
    m_env.append(qMakePair(key, value));
    return *this;
}

DockerExec& DockerExec::user(const QString& name)
{
    m_user = name;
    return *this;
}

DockerExec& DockerExec::run(const Command& command)
{
    m_innerProgram = command.program();
    m_innerArguments = command.arguments();
    return *this;
}

DockerExec& DockerExec::run(const QString& program, const QStringList& arguments)
{
    m_innerProgram = program;
    m_innerArguments = arguments;
    return *this;
}

QString DockerExec::program() const { return QStringLiteral("docker"); }

QStringList DockerExec::arguments() const
{
    QStringList args;
    args << QStringLiteral("exec");

    if (m_interactive)
    {
        args << QStringLiteral("--interactive");
    }
    if (m_workdir)
    {
        args << QStringLiteral("--workdir") << *m_workdir;
    }
    if (m_user)
    {
        args << QStringLiteral("--user") << *m_user;
    }
    for (const auto& variable : m_env)
    {
        args << QStringLiteral("--env") << variable.first + QLatin1Char('=') + variable.second;
    }

    args << m_container;

    if (!m_innerProgram.isEmpty())
    {
        args << m_innerProgram << m_innerArguments;
    }

    return args;
}
