/**************************************************************************
*   文件名	：client.h
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
#pragma once

#include <QDebug>
#include <QObject>
#include <QTcpSocket>
#include <QThread>
#include <QUdpSocket>

class client : public QObject
{
    Q_OBJECT
public:
    client(const QString& ip, quint16 port);
    void send();

private:
    QTcpSocket* tcp;
};

class udpclient : public QObject
{
    Q_OBJECT
public:
    udpclient(const QString& ip, quint16 port);
    void send();

private:
    QUdpSocket*  m_udp;
    QHostAddress m_addr;
    quint16      m_port;
};

int test_Loop();
