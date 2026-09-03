#pragma once
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

#include "command.h"

struct Mount
{
    QString source;
    QString target;
    bool readOnly = false;

    QString toArgument() const;
};

class DockerRun : public Command
{
public:
    explicit DockerRun(const QString& image);

    DockerRun& name(const QString& container);
    DockerRun& detach();
    DockerRun& removeOnExit();
    DockerRun& interactive();
    DockerRun& env(const QString& key, const QString& value);
    DockerRun& passEnv(const QString& key);
    DockerRun& mount(const QString& source, const QString& target, bool readOnly = false);
    DockerRun& workdir(const QString& path);
    DockerRun& user(const QString& name);
    DockerRun& memory(const QString& limit);
    DockerRun& cpus(const QString& limit);
    DockerRun& network(const QString& mode);
    DockerRun& run(const Command& command);
    DockerRun& run(const QString& program, const QStringList& arguments = QStringList());

    QString program() const override;
    QStringList arguments() const override;

private:
    QString m_image;
    bool m_detach = false;
    bool m_removeOnExit = false;
    bool m_interactive = false;
    std::optional<QString> m_name;
    std::optional<QString> m_workdir;
    std::optional<QString> m_user;
    std::optional<QString> m_memory;
    std::optional<QString> m_cpus;
    std::optional<QString> m_network;
    QVector<QPair<QString, QString>> m_env;
    QStringList m_passedEnv;
    QVector<Mount> m_mounts;
    QString m_innerProgram;
    QStringList m_innerArguments;
};
