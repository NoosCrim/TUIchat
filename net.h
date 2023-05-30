#pragma once
#include <C:\include\asio.hpp>
#include <iomanip>
#include <iostream>
#include <vector>
#include "threadsafe.h"

using tcp = asio::ip::tcp;

namespace net
{
    template<typename msgT>
    struct message
    {
        struct message_header
        {
            msgT msgType;
            uint32_t msgBodySize;
        };
        message_header header;
        
        size_t Size() const { return sizeof(header) + body.size(); }
        std::vector<uint8_t> body;
        template<typename dataT>
        message<msgT>& operator<< (const dataT& newData);

        template<typename dataT> 
        message<msgT>& operator>> (dataT& out);

    };
    template<typename msgT>
    class connection {
    public:
        threadsafe_queue<message<msgT>> received;
        connection(asio::io_context& _context);
        bool Connect(std::string const& ip_address, uint16_t port);
        void Listen(uint16_t port, std::string IP = "");
        void Send(const message<msgT>& msg);
        void StartReceive();
        void Disconect();
    protected:
        asio::io_context& context;
        message<msgT> tempInMsg;
        threadsafe_queue<message<msgT>> toSend;
        tcp::socket socket;
        tcp::acceptor acceptor;
        tcp::endpoint endpoint;

        void SendHeader();
        void SendBody();
        void ReceiveHeader();
        void ReceiveBody();
        void StoreReceived();
    };
    
}
template<typename msgT>
template<typename dataT> net::message<msgT>& net::message<msgT>::operator<< (const dataT& newData)
{
    static_assert(std::is_trivially_copyable<dataT>::value, "\nData is too compliated to put into message!\n");
    size_t prevDataSize = body.size();
    body.resize(prevDataSize + sizeof(dataT));
    header.msgSize = body.size() + sizeof(header);
    std::memcpy(body.data() + prevDataSize, &newData, sizeof(dataT));
    return *this;
}

template<typename msgT>
template<typename dataT> net::message<msgT>& net::message<msgT>::operator>>(dataT& out)
{
    static_assert(std::is_trivially_copyable<dataT>::value, "\nVariable is too compliated to read into from the message!\n");
    size_t newSize = body.size() - sizeof(dataT);
    std::memcpy(&out, body.data() + newSize, sizeof(dataT));
    body.resize(newSize);
    header.msgSize = body.size() + sizeof(header);
    return *this;
}

template<typename T>
net::connection<T>::connection(asio::io_context& _context): context(_context), socket(_context), acceptor(_context)
{

}
template <typename T>
void net::connection<T>::Listen(uint16_t port, std::string IP)
{
    asio::error_code ec;
    acceptor.open(tcp::v4());
    acceptor.set_option(tcp::acceptor::reuse_address(true));
    acceptor.bind(tcp::endpoint{ {}, port });
    acceptor.listen();

    std::cout << "\nListening for connection..." << std::endl;
    do
    {
        acceptor.accept(socket, endpoint, ec);
        if (ec)
            std::cout << ec.message() << std::endl;
    } while (IP != "" && endpoint.address() != asio::ip::address::from_string(IP));

    StartReceive();
    std::cout << "\nConnection has been found!" << std::endl;
}
template <typename T>
bool net::connection<T>::Connect(std::string const& ip_address, uint16_t port)
{
    endpoint = tcp::endpoint(asio::ip::address::from_string(ip_address), port);
    asio::error_code ec;
    std::cout << "\nLooking for connection..." << std::endl;
    socket.connect(endpoint, ec);
    if (!ec)
    {
        StartReceive();
        std::cout << "\nConnected!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "\nConnecting failed: " << ec.message() << std::endl;
        socket.close();
        return false;
    }
}
template<typename T>
void net::connection<T>::Send(const message<T>& msg)
{
    bool isSending = !toSend.empty();
    toSend.push(msg);
    if (!isSending)
        SendHeader();
}
template<typename T>
void net::connection<T>::SendHeader()
{
    socket.async_send(asio::buffer(&(toSend.front().header), sizeof(toSend.front().header)),
        [this](const asio::error_code& ec, size_t bytes_sent)
        {
            if (!ec)
            {
                if (toSend.front().body.size() > 0)
                    SendBody();
                else
                {
                    std::cout << "\nMessage body is empty, it will not be sent" << std::endl;
                    toSend.pop();
                    if (!toSend.empty())
                        SendHeader();
                }
            }
            else
            {
                std::cout << "\nHeader has not been sent: " << ec.message() << "\nMessage body will not be sent." << std::endl;
                toSend.pop();
                socket.close();
            }
        });
}
template<typename T>
void net::connection<T>::SendBody()
{
    socket.async_send(asio::buffer(toSend.front().body.data(), toSend.front().header.msgBodySize),
        [this](const asio::error_code& ec, size_t bytes_sent)
        {
            if (ec)
                std::cout << "\nMessage body has not been sent: " << ec.message() << std::endl;
            //else
            //    std::cout << "Message body has been sent. Size: " << toSend.front().header.msgBodySize << '\n';
            toSend.pop();
            if (!toSend.empty())
                SendHeader();   
        });
}
template<typename T>
void net::connection<T>::StartReceive()
{
    ReceiveHeader();
}
template<typename T>
void net::connection<T>::ReceiveHeader()
{
    socket.async_receive(asio::buffer(&(tempInMsg.header), sizeof(tempInMsg.header)),
        [this](const asio::error_code& ec, size_t bytes_received)
        {
            if (!ec)
            {
                //std::cout << "Got header\n MsgBodySize: " << tempInMsg.header.msgBodySize << '\n';
                tempInMsg.body.resize(tempInMsg.header.msgBodySize);
                if (tempInMsg.header.msgBodySize > 0)
                    ReceiveBody();
                else
                    StoreReceived();
            }
            else
            {
                std::cout << "\nMessage header has not been received: " << ec.message() << std::endl;
                socket.close();
            }
        });
}
template<typename T>
void net::connection<T>::ReceiveBody()
{
    socket.async_receive(asio::buffer(tempInMsg.body.data(), tempInMsg.header.msgBodySize),
        [this](const asio::error_code& ec, size_t bytes_received)
        {
            if (!ec)
            {
                //std::cout << "Got body\n";
                StoreReceived();
            }
            else
                std::cout << "\nMessage body has not been received: " << ec.message() << std::endl;
            ReceiveHeader();
        });
}
template<typename T>
void net::connection<T>::StoreReceived()
{
    //std::cout << "Stored message\n Body size: " << tempInMsg.body.size() << "\n  Acording to header: " << tempInMsg.header.msgBodySize << '\n';
    received.push(tempInMsg);
}
template<typename T>
void net::connection<T>::Disconect()
{
    socket.close();
}