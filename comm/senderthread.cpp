#include "senderthread.h"

SenderThread::SenderThread() : QThread()
{

}

void SenderThread::run()
{
    exec();
}
