/**************************************************************************
*   文件名	：client.cpp
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
#include <chrono>
#include <iostream>
using namespace std;
long long getNowMs()
{
    return clock() / (CLOCKS_PER_SEC / 1000); //兼容跨平台
}
using namespace chrono;
system_clock::time_point start;
client::client(const QString& ip, quint16 port)
{
    tcp = new QTcpSocket();
    connect(tcp, &QTcpSocket::connected, this, [=]() { qDebug() << "connected!"; });
    connect(tcp, &QTcpSocket::disconnected, this, []() { qDebug() << "disconnected!"; });
    connect(tcp, &QTcpSocket::readyRead, this, [this]() {
        while (tcp->bytesAvailable() > 0)
        {
            QByteArray datagram;
            datagram.resize(tcp->bytesAvailable());
            tcp->read(datagram.data(), datagram.size());
            auto end      = system_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            std::cout << "recv: time(us):" << duration.count() << "data: " << datagram.data() << "\n";
        }
    });
    tcp->connectToHost(ip, port);
}
void client::send()
{
    QString str;
    for (int i = 0; i < 20; ++i)
    {
        str.append("#");
    }
    start = system_clock::now();
    //    qDebug() << str;
    tcp->write(str.toLatin1(), str.length());
}

static const int packsize = 1024;
udpclient::udpclient(const QString& ip, quint16 port)
{
    m_udp = new QUdpSocket();
    connect(m_udp, &QUdpSocket::connected, this, [=]() { qDebug() << "connected!"; });
    connect(m_udp, &QUdpSocket::disconnected, this, []() { qDebug() << "disconnected!"; });
    connect(m_udp, &QUdpSocket::readyRead, this, [this]() {
        qDebug() << "recv";
        static char revdata[packsize * 8];
        while (m_udp->hasPendingDatagrams())
        {
            int sz = m_udp->readDatagram(revdata, packsize * 8);
            if (sz > 0)
            {
                auto end      = system_clock::now();
                auto duration = duration_cast<microseconds>(end - start);
                std::cout << "recv: time(us):" << duration.count() << "data: " << revdata << "\n";
            }
        }
    });
    m_addr = QHostAddress(ip);
    m_port = port;
    qDebug() << m_addr << m_port;
    m_udp->bind(33335, QUdpSocket::ShareAddress);
}

int  s_count = 0;
void udpclient::send()
{
    s_count++;
    auto str = QString("aaaaaaa %1").arg(s_count);
    m_udp->writeDatagram(str.toLatin1(), m_addr, m_port);
    start = system_clock::now();
}

int test_Loop()
{
    QUdpSocket* p = new QUdpSocket;
    char        revdata[packsize * 8];
    if (p->bind(QHostAddress::LocalHost, 33335))
    {
        for (int i = 0; i < 100; ++i)
        {
            auto str = QString("aaaaaaa %1");
            p->writeDatagram(str.toLatin1(), QHostAddress("127.0.0.1"), 33334);
            start = system_clock::now();
            while (!(p->hasPendingDatagrams()))
                ;
            p->waitForReadyRead();
            int sz = p->readDatagram(revdata, packsize * 8);
            if (sz > 0)
            {
                auto end      = system_clock::now();
                auto duration = duration_cast<microseconds>(end - start);
                std::cout << "recv: time(us):" << duration.count() << "data: " << revdata << "\n";
                QThread::msleep(1000);
            }
        }
        p->close();
    }
    p->deleteLater();
    return 1;
}
