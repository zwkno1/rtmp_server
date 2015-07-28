#include <QCoreApplication>
#include "rtmp.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    boost::asio::io_service service;
    rtmp_server server(service, 40000);
    server.start();
    service.run();

    return a.exec();
}
