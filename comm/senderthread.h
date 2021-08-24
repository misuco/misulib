#ifndef SENDERTHREAD_H
#define SENDERTHREAD_H

#include <QObject>
#include <QThread>

class SenderThread : public QThread
{
    Q_OBJECT

public:
    explicit SenderThread();
    void run();

};

#endif // SENDERTHREAD_H
