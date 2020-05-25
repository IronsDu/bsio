#pragma once

#include <memory>
#include <functional>

#include <bsio/IoContextThreadPool.hpp>
#include <bsio/Functor.hpp>
#include <bsio/SharedSocket.hpp>
#include <asio/basic_socket_acceptor.hpp>

namespace bsio { namespace net {

    class TcpAcceptor : public asio::noncopyable, 
                        public std::enable_shared_from_this<TcpAcceptor>
    {
    public:
        using Ptr = std::shared_ptr<TcpAcceptor>;

        TcpAcceptor(
            asio::io_context& listenContext,
            IoContextThreadPool::Ptr ioContextThreadPool,
            asio::ip::tcp::endpoint endpoint)
            :
            mIoContextThreadPool(std::move(ioContextThreadPool)),
            mAcceptor(std::make_shared<asio::ip::tcp::acceptor>(listenContext, std::move(endpoint)))
        {
        }

        virtual ~TcpAcceptor()
        {
            close();
        }

        void    startAccept(SocketEstablishHandler callback)
        {
            doAccept(std::move(callback));
        }
        
        void    close() const
        {
            mAcceptor->close();
        }

    private:
        void    doAccept(SocketEstablishHandler callback)
        {
            if (!mAcceptor->is_open())
            {
                return;
            }

            auto& ioContext = mIoContextThreadPool->pickIoContext();
            auto sharedSocket = SharedSocket::Make(asio::ip::tcp::socket(ioContext), ioContext);

            const auto self = shared_from_this();
            mAcceptor->async_accept(
                sharedSocket->socket(),
                [self, callback = std::move(callback), sharedSocket, this](std::error_code ec) {
                    if (!ec)
                    {
                        sharedSocket->context().post([=]() {
                                callback(std::move(sharedSocket->socket()));
                            });
                    }
                    doAccept(std::move(callback));
                });
        }

    private:
        IoContextThreadPool::Ptr                    mIoContextThreadPool;
        std::shared_ptr<asio::ip::tcp::acceptor>    mAcceptor;
    };

} }