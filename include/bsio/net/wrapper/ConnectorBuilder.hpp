#pragma once

#include <bsio/net/TcpSession.hpp>
#include <bsio/net/wrapper/internal/Option.hpp>
#include <bsio/net/wrapper/internal/TcpSessionBuilder.hpp>
#include <functional>
#include <memory>
#include <optional>

namespace bsio::net::wrapper {

template<typename Derived>
class BaseTcpSessionConnectorBuilder : public internal::BaseSessionOptionBuilder<Derived>
{
public:
    virtual ~BaseTcpSessionConnectorBuilder() = default;

    Derived& WithConnector(TcpConnector connector) noexcept
    {
        mConnector = std::move(connector);
        return static_cast<Derived&>(*this);
    }

    Derived& WithEndpoint(asio::ip::tcp::endpoint endpoint) noexcept
    {
        mSocketOption.endpoint = std::move(endpoint);
        return static_cast<Derived&>(*this);
    }

    Derived& WithTimeout(std::chrono::nanoseconds timeout) noexcept
    {
        mSocketOption.timeout = timeout;
        return static_cast<Derived&>(*this);
    }

    Derived& WithFailedHandler(SocketFailedConnectHandler handler) noexcept
    {
        mSocketOption.failedHandler = std::move(handler);
        return static_cast<Derived&>(*this);
    }

    Derived& AddSocketProcessingHandler(SocketProcessingHandler handler) noexcept
    {
        mSocketOption.socketProcessingHandlers.emplace_back(std::move(handler));
        return static_cast<Derived&>(*this);
    }

    Derived& WithRecvBufferSize(size_t size) noexcept
    {
        mReceiveBufferSize = size;
        return static_cast<Derived&>(*this);
    }

    void asyncConnect()
    {
        using Base = internal::BaseSessionOptionBuilder<Derived>;

        if (!mConnector)
        {
            throw std::runtime_error("connector is empty");
        }
        if (Base::Option().establishHandlers.empty())
        {
            throw std::runtime_error("establishHandlers is empty");
        }

        mConnector->asyncConnect(
                mSocketOption.endpoint,
                mSocketOption.timeout,
                [option = Base::Option(),
                 receiveBufferSize = mReceiveBufferSize](asio::ip::tcp::socket socket) {
                    const auto session = TcpSession::Make(
                            std::move(socket),
                            receiveBufferSize,
                            option.dataHandler,
                            option.closedHandler,
                            option.eofHandler);
                    for (const auto& callback : option.establishHandlers)
                    {
                        callback(session);
                    }
                    session->startRecv();
                },
                mSocketOption.failedHandler,
                mSocketOption.socketProcessingHandlers);
        Base::clear();
    }

private:
    std::optional<TcpConnector> mConnector;
    internal::SocketConnectOption mSocketOption;
    size_t mReceiveBufferSize = {0};
};

class TcpSessionConnectorBuilder : public BaseTcpSessionConnectorBuilder<TcpSessionConnectorBuilder>
{
};

}// namespace bsio::net::wrapper
