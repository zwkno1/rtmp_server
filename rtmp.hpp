#ifndef RTMP_H
#define RTMP_H

#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <stdint.h>
#include <QDebug>

#pragma pack(push,1)

using boost::asio::ip::tcp;

template<class _Ty, class _In>
_Ty get_value(_In in)
{
    return *reinterpret_cast<_Ty *>(in);
}

template<class _Iter>
void data_dump(_Iter begin, _Iter end)
{
    std::cout << std::showbase << std::uppercase;
    while(begin != end)
    {
        std::cout << std::setw(2) << std::hex << (int)*begin << " ";
        ++begin;
    }
    std::cout << std::endl;
}

struct handshake_data
{
    uint32_t timestamp = 0;
    uint32_t timestamp1 = 0;
    char random[1528] = {0};
};


class rtmp_session : public boost::enable_shared_from_this<rtmp_session>
{
public:
    rtmp_session(boost::asio::io_service & service)
            :service_(service),
              socket_(service)
    {
    }

    ~rtmp_session()
    {
        qDebug() << "connection disconnect";
    }

    tcp::socket & socket() { return socket_; }

    void start()
    {
        auto self = shared_from_this();
        boost::asio::async_read(socket_, boost::asio::buffer(recv_buf_), boost::asio::transfer_exactly(c0_size + c1_size), [self](const boost::system::error_code err, size_t bytes_transferred){
            self->handshake_c0_c1(err, bytes_transferred);
        });
    }

    void handshake_c0_c1(const boost::system::error_code & err, size_t bytes_transferred)
    {
        auto self = shared_from_this();
        if(!err)
        {
            qDebug() << __FUNCTION__ << ":";
            data_dump(&recv_buf_[0], &recv_buf_[bytes_transferred]);
            client_version_ = recv_buf_[0];
            client_data_ = get_value<handshake_data>(&recv_buf_[1]);

            qDebug() << "client version: " << int(client_version_);
            qDebug() << "client timestamp: " << client_data_.timestamp;
            qDebug() << "client zero: " << client_data_.timestamp1;

            //s0
            send_buf_[0] = server_version_;
            //s1
            server_data_.timestamp = time(NULL);
            std::memcpy(&send_buf_[1], &server_data_, sizeof(server_data_));
            //s2
            client_data_.timestamp1 = server_data_.timestamp;
            std::memcpy(&send_buf_[1 + sizeof(server_data_)], &client_data_, sizeof(client_data_));

            boost::asio::async_write(socket_, boost::asio::buffer(send_buf_), boost::asio::transfer_exactly(s0_size + s1_size + s2_size), [self](const boost::system::error_code err, size_t bytes_transferred){
                self->handshake_s0_s1_s2(err, bytes_transferred);
            });
        }
    }

    void handshake_s0_s1_s2(const boost::system::error_code & err, size_t bytes_transferred)
    {
        auto self = shared_from_this();
        if(!err)
        {
            qDebug() << __FUNCTION__ << ":";
            data_dump(&send_buf_[0], &send_buf_[bytes_transferred]);
            boost::asio::async_read(socket_, boost::asio::buffer(recv_buf_), boost::asio::transfer_exactly(c2_size), [self](const boost::system::error_code err, size_t bytes_transferred){
                self->handshake_c2(err, bytes_transferred);
            });
        }
    }

    void handshake_c2(const boost::system::error_code & err, size_t bytes_transferred)
    {
        auto self = shared_from_this();
        if(!err)
        {
            qDebug() << __FUNCTION__ << ":";
            data_dump(&recv_buf_[0], &recv_buf_[bytes_transferred]);
            handshake_data *data = reinterpret_cast<handshake_data *>(&recv_buf_);
            if(data->timestamp != server_data_.timestamp)
            {
                qDebug() << "timestamp error";
                return;
            }
            qDebug() << "client timestamp1: " << data->timestamp1;
            if(std::memcmp(data->random, server_data_.random, sizeof(data->random)) != 0)
            {
                qDebug() << "random data error";
                return;
            }
            qDebug() << "handshake ok!" ;

            boost::asio::async_read(socket_, boost::asio::buffer(recv_buf_), boost::asio::transfer_exactly(10000), [self](const boost::system::error_code err, size_t bytes_transferred){
                self->handshake_wait(err, bytes_transferred);
            });
        }
    }

    void handshake_wait(const boost::system::error_code & err, size_t bytes_transferred)
    {
        auto self = shared_from_this();
        boost::asio::async_read(socket_, boost::asio::buffer(recv_buf_), boost::asio::transfer_exactly(10000), [self](const boost::system::error_code err, size_t bytes_transferred){
            self->handshake_wait(err, bytes_transferred);
        });
    }

private:
    char server_version_ = '\x03';
    char client_version_ = '\x03';
    handshake_data client_data_;
    handshake_data server_data_;
private:
    const static size_t c0_size = 1;
    const static size_t s0_size = 1;
    const static size_t c1_size = 1536;
    const static size_t s1_size = 1536;
    const static size_t c2_size = 1536;
    const static size_t s2_size = 1536;

private:
    boost::asio::io_service & service_;
    tcp::socket socket_;
    boost::array<unsigned char, 4096> recv_buf_;
    boost::array<unsigned char, 4096> send_buf_;
};

class rtmp_server
{
public:
    rtmp_server(boost::asio::io_service &service, short server_port)
        : service_(service)
        , acceptor_(service, tcp::endpoint(tcp::v4(), server_port))
    {
        start();
    }

    void start()
    {
        boost::shared_ptr<rtmp_session> new_session(new rtmp_session(service_));
        acceptor_.async_accept(new_session->socket(), [new_session, this](const boost::system::error_code & err){
            this->handle_accept(new_session, err);
        });
    }

    void handle_accept(boost::shared_ptr<rtmp_session> new_session,
        const boost::system::error_code& error)
    {
        qDebug() << "new connection";
        new_session->start();
        start();
    }

private:
    tcp::acceptor acceptor_;
    boost::asio::io_service & service_;
};

#pragma pack(pop)
#endif // RTMP_H
