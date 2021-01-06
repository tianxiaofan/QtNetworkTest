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
#include <thread>
using namespace std;
using namespace chrono;
using asio::ip::udp;
enum
{
    max_length = 1024
};

double getNowMs()
{
    return (double)clock() / ((double)CLOCKS_PER_SEC / 1000.0); //兼容跨平台
}

using asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
    chat_client(asio::io_context& io_context, const tcp::resolver::results_type& endpoints)
        : io_context_(io_context), socket_(io_context)
    {
        do_connect(endpoints);
    }

    void write(const chat_message& msg)
    {
        asio::post(io_context_, [this, msg]() {
            bool write_in_progress = !write_msgs_.empty();
            write_msgs_.push_back(msg);
            if (!write_in_progress)
            {
                do_write();
            }
        });
    }

    void close()
    {
        asio::post(io_context_, [this]() { socket_.close(); });
    }

private:
    void do_connect(const tcp::resolver::results_type& endpoints)
    {
        asio::async_connect(socket_, endpoints, [this](std::error_code ec, tcp::endpoint) {
            if (!ec)
            {
                do_read_header();
            }
        });
    }

    void do_read_header()
    {
        asio::async_read(socket_, asio::buffer(read_msg_.data(), chat_message::header_length),
                         [this](std::error_code ec, std::size_t /*length*/) {
                             if (!ec && read_msg_.decode_header())
                             {
                                 do_read_body();
                             }
                             else
                             {
                                 socket_.close();
                             }
                         });
    }

    void do_read_body()
    {
        asio::async_read(socket_, asio::buffer(read_msg_.body(), read_msg_.body_length()),
                         [this](std::error_code ec, std::size_t /*length*/) {
                             if (!ec)
                             {
                                 std::cout.write(read_msg_.body(), read_msg_.body_length());
                                 auto end      = system_clock::now();
                                 auto duration = duration_cast<microseconds>(end - start);
                                 std::cout << "---- us " << duration.count() << "---\n";
                                 do_read_header();
                             }
                             else
                             {
                                 socket_.close();
                             }
                         });
    }

    void do_write()
    {
        asio::async_write(socket_, asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()),
                          [this](std::error_code ec, std::size_t /*length*/) {
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
                                  socket_.close();
                              }
                          });
        start = system_clock::now();
    }

private:
    asio::io_context&  io_context_;
    tcp::socket        socket_;
    chat_message       read_msg_;
    chat_message_queue write_msgs_;
    system_clock::time_point start;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 3)
        {
            std::cerr << "Usage: AsioClient <udp/tcp> <ip> <port>\n";
            return 1;
        }

        if (std::string(argv[1]) == "tcp")
        {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            auto          endpoints = resolver.resolve(argv[2], argv[3]);
            chat_client   c(io_context, endpoints);

            std::thread t([&io_context]() { io_context.run(); });

            //        char line[chat_message::max_body_length + 1];
            //        while (std::cin.getline(line, chat_message::max_body_length + 1))
            //        {
            //            chat_message msg;
            //            msg.body_length(std::strlen(line));
            //            std::memcpy(msg.body(), line, msg.body_length());
            //            msg.encode_header();
            //            c.write(msg);
            //        }

            const char* str = "dddddddddddddddddddd";
            while (1)
            {
                chat_message msg;
                msg.body_length(std::strlen(str));
                std::memcpy(msg.body(), str, msg.body_length());
                msg.encode_header();
                c.write(msg);
                std::this_thread::sleep_for(1000ms);
            }

            c.close();
            t.join();
        }
        if (std::string(argv[1]) == "udp")
        {
            //            asio::io_context io_context;
            //            sender           s(io_context, asio::ip::make_address(argv[2]));
            //            io_context.run();

            asio::io_context io_context;

            udp::socket s(io_context, udp::endpoint(udp::v4(), 0));

            udp::resolver               resolver(io_context);
            udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), argv[2], argv[3]);

            while (1)
            {
                const char* request        = "AVCCCcccccc";
                size_t      request_length = std::strlen(request);
                auto        start          = system_clock::now();
                s.send_to(asio::buffer(request, request_length), *endpoints.begin());

                char          reply[max_length];
                udp::endpoint sender_endpoint;
                size_t        reply_length = s.receive_from(asio::buffer(reply, max_length), sender_endpoint);
                auto          end          = system_clock::now();
                auto          duration     = duration_cast<microseconds>(end - start);
                std::cout << "Reply is: us (" << duration.count() << "us)";
                std::cout.write(reply, reply_length);
                std::cout << "\n";
                std::this_thread::sleep_for(1000ms);
            }
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
