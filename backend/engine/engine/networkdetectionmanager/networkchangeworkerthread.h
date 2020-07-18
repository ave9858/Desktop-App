#ifndef NETWORKCHANGEWORKERTHREAD_H
#define NETWORKCHANGEWORKERTHREAD_H

#include <QObject>
#include <QThread>

class NetworkChangeWorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit NetworkChangeWorkerThread(QObject *parent = nullptr);
    virtual ~NetworkChangeWorkerThread();

    void earlyExit();

signals:
    void networkChanged();

public slots:
    void run() override;

private:
    void *hExitEvent_;

};

#endif // NETWORKCHANGEWORKERTHREAD_H
