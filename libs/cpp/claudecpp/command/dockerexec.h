#pragma once
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

#include "command.h"

class DockerExec : public Command
{
public:
    explicit DockerExec(const QString& container);

    DockerExec& interactive();
    DockerExec& workdir(const QString& path);
    DockerExec& env(const QString& key, const QString& value);
    DockerExec& user(const QString& name);
    DockerExec& run(const Command& command);
    DockerExec& run(const QString& program, const QStringList& arguments = QStringList());

    QString program() const override;
    QStringList arguments() const override;

private:
    QString m_container;
    bool m_interactive = false;
    std::optional<QString> m_workdir;
    std::optional<QString> m_user;
    QVector<QPair<QString, QString>> m_env;
    QString m_innerProgram;
    QStringList m_innerArguments;
};
