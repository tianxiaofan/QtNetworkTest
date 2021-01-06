/**************************************************************************
*   文件名	：%{Cpp:License:FileName}
*   =======================================================================
*   创 建 者	：田小帆
*   创建日期	：2021-1-5
*   邮   箱	：499131808@qq.com
*   Q Q		：499131808
*   功能描述	：
*   使用说明	：
*   ======================================================================
*   修改者	：
*   修改日期	：
*   修改内容	：
*   ======================================================================
*
***************************************************************************/
#include "client.h"
#include <QCoreApplication>
#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>

//通过宏来控制不同功能
#define udp1 0
#define udp2 1
#define tcp 0

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
#if tcp
    static client cl("127.0.0.1", 33334);
    static QTimer ti;

    QObject::connect(&ti, &QTimer::timeout, [=]() { cl.send(); });
    ti.start(1000);
#endif

#if udp1
    static udpclient cl("127.0.0.1", 33334);
    static QTimer    ti;

    QObject::connect(&ti, &QTimer::timeout, [=]() { cl.send(); });
    ti.start(1000);
#endif

#if udp2
    test_Loop();
#endif

    return a.exec();
}
