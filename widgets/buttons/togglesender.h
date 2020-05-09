#ifndef ToggleSender_H
#define ToggleSender_H

#include <QObject>
#include "buttonbase.h"

class ToggleSender : public ButtonBase
{
    Q_OBJECT

public:
    ToggleSender(QString text, QString senderId, int state, QObject *parent = nullptr);

signals:
    void toggleSender(QString senderId, bool state);

protected:
    void releaseAction() override;
    void setAction() override;

private:
    QString _senderId;

};

#endif // ToggleSender_H
