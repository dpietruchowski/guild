#include "dockerrun.h"

QString Mount::toArgument() const
{
    QString argument = source + QLatin1Char(':') + target;
    if (readOnly)
    {
        argument += QLatin1String(":ro");
    }
    return argument;
}

DockerRun::DockerRun(const QString& image)
    : m_image(image)
{
}

DockerRun& DockerRun::name(const QString& container)
{
    m_name = container;
    return *this;
}

DockerRun& DockerRun::detach()
{
    m_detach = true;
    return *this;
}

DockerRun& DockerRun::removeOnExit()
{
    m_removeOnExit = true;
    return *this;
}

DockerRun& DockerRun::interactive()
{
    m_interactive = true;
    return *this;
}

DockerRun& DockerRun::env(const QString& key, const QString& value)
{
    m_env.append(qMakePair(key, value));
    return *this;
}

DockerRun& DockerRun::mount(const QString& source, const QString& target, bool readOnly)
{
    m_mounts.append(Mount { source, target, readOnly });
    return *this;
}

DockerRun& DockerRun::workdir(const QString& path)
{
    m_workdir = path;
    return *this;
}

DockerRun& DockerRun::user(const QString& name)
{
    m_user = name;
    return *this;
}

DockerRun& DockerRun::memory(const QString& limit)
{
    m_memory = limit;
    return *this;
}

DockerRun& DockerRun::cpus(const QString& limit)
{
    m_cpus = limit;
    return *this;
}

DockerRun& DockerRun::network(const QString& mode)
{
    m_network = mode;
    return *this;
}

DockerRun& DockerRun::run(const Command& command)
{
    m_innerProgram = command.program();
    m_innerArguments = command.arguments();
    return *this;
}

DockerRun& DockerRun::run(const QString& program, const QStringList& arguments)
{
    m_innerProgram = program;
    m_innerArguments = arguments;
    return *this;
}

QString DockerRun::program() const { return QStringLiteral("docker"); }

QStringList DockerRun::arguments() const
{
    QStringList args;
    args << QStringLiteral("run");

    if (m_detach)
    {
        args << QStringLiteral("--detach");
    }
    if (m_removeOnExit)
    {
        args << QStringLiteral("--rm");
    }
    if (m_interactive)
    {
        args << QStringLiteral("--interactive");
    }
    if (m_name)
    {
        args << QStringLiteral("--name") << *m_name;
    }
    for (const auto& variable : m_env)
    {
        args << QStringLiteral("--env") << variable.first + QLatin1Char('=') + variable.second;
    }
    for (const auto& mount : m_mounts)
    {
        args << QStringLiteral("--volume") << mount.toArgument();
    }
    if (m_workdir)
    {
        args << QStringLiteral("--workdir") << *m_workdir;
    }
    if (m_user)
    {
        args << QStringLiteral("--user") << *m_user;
    }
    if (m_memory)
    {
        args << QStringLiteral("--memory") << *m_memory;
    }
    if (m_cpus)
    {
        args << QStringLiteral("--cpus") << *m_cpus;
    }
    if (m_network)
    {
        args << QStringLiteral("--network") << *m_network;
    }

    args << m_image;

    if (!m_innerProgram.isEmpty())
    {
        args << m_innerProgram << m_innerArguments;
    }

    return args;
}
