#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>

#include "claudecpp/process/commandrunner.h"
#include "guildcore/workspace/workspace.h"

class Guild : public QObject
{
    Q_OBJECT

public:
    explicit Guild(const Workspace& workspace, QObject* parent = nullptr);

    QStringList agents() const;
    bool isBusy() const;

    void up(const QString& agent);
    void run(const QString& agent, const QString& prompt);
    void cancel();

signals:
    void messageReceived(const QJsonObject& message);
    void invalidLineReceived(const QString& line);
    void finished(int exitCode);
    void failed(const QString& reason);

private:
    Workspace m_workspace;
    QString m_image;
    CommandRunner m_runner;
};
