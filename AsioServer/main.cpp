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
#include <QCoreApplication>
#if 0
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    return a.exec();
}
#endif

#include "asio.hpp"
#include "chat_message.hpp"
#include <chrono>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
using namespace std;
using namespace chrono;
using asio::ip::tcp;
using asio::ip::udp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
    virtual ~chat_participant() {}
    virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
    void join(chat_participant_ptr participant)
    {
        participants_.insert(participant);
        for (auto msg : recent_msgs_)
            participant->deliver(msg);
    }

    void leave(chat_participant_ptr participant) { participants_.erase(participant); }

    void deliver(const chat_message& msg)
    {
        recent_msgs_.push_back(msg);
        while (recent_msgs_.size() > max_recent_msgs)
            recent_msgs_.pop_front();

        for (auto participant : participants_)
            participant->deliver(msg);
    }

private:
    std::set<chat_participant_ptr> participants_;
    enum
    {
        max_recent_msgs = 100
    };
    chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(tcp::socket socket, chat_room& room) : socket_(std::move(socket)), room_(room) {}

    void start()
    {
        room_.join(shared_from_this());
        do_read_header();
    }

    void deliver(const chat_message& msg)
    {
        bool write_in_progress = !write_msgs_.empty();
        write_msgs_.push_back(msg);
        if (!write_in_progress)
        {
            do_write();
        }
    }

private:
    void do_read_header()
    {
        auto self(shared_from_this());
        asio::async_read(socket_, asio::buffer(read_msg_.data(), chat_message::header_length),
                         [this, self](std::error_code ec, std::size_t /*length*/) {
                             if (!ec && read_msg_.decode_header())
                             {
                                 do_read_body();
                             }
                             else
                             {
                                 room_.leave(shared_from_this());
                             }
                         });
    }

    void do_read_body()
    {
        auto self(shared_from_this());
        asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()),
                         [this, self](std::error_code ec, std::size_t /*length*/) {
                             if (!ec)
                             {
                                 room_.deliver(read_msg_);
                                 do_read_header();
                             }
                             else
                             {
                                 room_.leave(shared_from_this());
                             }
                         });
    }

    void do_write()
    {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                          [this, self](std::error_code ec, std::size_t /*length*/) {
                              if (!ec)
                              {
                                  write_msgs_.pop_front();
                                  if (!write_msgs_.empty())
                                  {
                                      do_write();
                                  }
                              }
                              else
                              {
                                  room_.leave(shared_from_this());
                              }
                          });
    }

    tcp::socket        socket_;
    chat_room&         room_;
    chat_message       read_msg_;
    chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
    chat_server(asio::io_context& io_context, const tcp::endpoint& endpoint) : acceptor_(io_context, endpoint)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept([this](std::error_code ec, tcp::socket socket) {
            if (!ec)
            {
                std::make_shared<chat_session>(std::move(socket), room_)->start();
            }

            do_accept();
        });
    }

    tcp::acceptor acceptor_;
    chat_room     room_;
};

class server
{
public:
    server(asio::io_context& io_context, short port) : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
        do_receive();
    }

    void do_receive()
    {
        socket_.async_receive_from(
            asio::buffer(data_, max_length), sender_endpoint_, [this](std::error_code ec, std::size_t bytes_recvd) {
                if (!ec && bytes_recvd > 0)
                {
                    do_send(bytes_recvd);
                    auto st = system_clock::now().time_since_epoch();
                    std::cout << data_ << " size:" << bytes_recvd << " time: " << st.count() << std::endl;
                }
                else
                {
                    do_receive();
                }
            });
    }

    void do_send(std::size_t length)
    {
        socket_.async_send_to(asio::buffer(data_, length), sender_endpoint_,
                              [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/) {
                                  memset(data_, 0, sizeof(data_));
                                  do_receive();
                              });
    }

private:
    udp::socket   socket_;
    udp::endpoint sender_endpoint_;
    enum
    {
        max_length = 1024
    };
    char data_[max_length];
};

enum
{
    max_length = 1024
};
void blockingServer(asio::io_context& io_context, unsigned short port)
{
    udp::socket sock(io_context, udp::endpoint(udp::v4(), port));
    for (;;)
    {
        char          data[max_length];
        udp::endpoint sender_endpoint;
        size_t        length = sock.receive_from(asio::buffer(data, max_length), sender_endpoint);
        if (length > 0)
        {
            sock.send_to(asio::buffer(data, length), sender_endpoint);
            auto st = system_clock::now().time_since_epoch();
            std::cout << data << " size:" << length << " time: " << st.count() << std::endl;
        }
    }
}

//----------------------------------------------------------------------
int main(int argc, char* argv[])
{
    try
    {
        if (argc < 2)
        {
            std::cerr << "Usage: AsioServer <udp/tcp> <port>\n";
            return 1;
        }

        if (std::string(argv[1]) == "tcp")
        {
            asio::io_context io_context;

            std::list<chat_server> servers;
            for (int i = 2; i < argc; ++i)
            {
                tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
                servers.emplace_back(io_context, endpoint);
            }

            io_context.run();
        }
        if (std::string(argv[1]) == "udp")
        {
            asio::io_context io_context;
            //            server           s(io_context, std::atoi(argv[2]));
            //            io_context.run();

            blockingServer(io_context, std::atoi(argv[2]));
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
