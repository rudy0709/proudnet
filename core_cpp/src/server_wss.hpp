﻿// org folder의 동명 파일로부터 branch 후 edit된 파일입니다.
// org folder의 파일을 업데이트하면 여기로 merge하는거 잊지 마세요!

#ifndef SERVER_WSS_HPP
#define	SERVER_WSS_HPP

#ifdef SUPPORTS_WEBGL

#include "server_ws.hpp"
#include <asio/ssl.hpp>
#include <openssl/ssl.h>

namespace Proud {
    typedef asio::ssl::stream<asio::ip::tcp::socket> WSS;    
        
    template<>
    class SocketServer<WSS> : public SocketServerBase<WSS> {
        std::string session_id_context;
        bool set_session_id_context=false;
    public:
        DEPRECATED SocketServer(unsigned short port, size_t thread_pool_size, const std::string& cert_file, const std::string& private_key_file, 
                size_t timeout_request=5, size_t timeout_idle=0, 
                const std::string& verify_file=std::string()) : SocketServer(cert_file, private_key_file, verify_file) {
            config.port=port;
            config.thread_pool_size=thread_pool_size;
            config.timeout_request=timeout_request;
            config.timeout_idle=timeout_idle;
        }
        
        SocketServer(const std::string& cert_file, const std::string& private_key_file,
                     const std::string& verify_file=std::string()) : SocketServerBase<WSS>(443), context(asio::ssl::context::tlsv12) {
            context.use_certificate_chain_file(cert_file);
            context.use_private_key_file(private_key_file, asio::ssl::context::pem);
            
            if(verify_file.size()>0) {
                context.load_verify_file(verify_file);
                context.set_verify_mode(asio::ssl::verify_peer | asio::ssl::verify_fail_if_no_peer_cert |
                                        asio::ssl::verify_client_once);
                set_session_id_context=true;
            }
        }
        
        void start() {
            if(set_session_id_context) {
                // Creating session_id_context from address:port but reversed due to small SSL_MAX_SSL_SESSION_ID_LENGTH
                session_id_context=std::to_string(config.port)+':';
                session_id_context.append(config.address.rbegin(), config.address.rend());
                SSL_CTX_set_session_id_context(context.native_handle(), reinterpret_cast<const unsigned char*>(session_id_context.data()),
                                               std::min<size_t>(session_id_context.size(), SSL_MAX_SSL_SESSION_ID_LENGTH));
            }
            SocketServerBase::start();
        }

    protected:
        asio::ssl::context context;
        
        void accept() {
            //Create new socket for this connection (stored in Connection::socket)
            //Shared_ptr is used to pass temporary objects to the asynchronous functions
            std::shared_ptr<Connection> connection(new Connection(new WSS(*io_service, context)));
            
            acceptor->async_accept(connection->socket->lowest_layer(), [this, connection](const std::error_code& ec) {
                //Immediately start accepting a new connection (if io_service hasn't been stopped)
                if (ec != asio::error::operation_aborted)
                    accept();

                if(!ec) {
                    asio::ip::tcp::no_delay option(true);
                    connection->socket->lowest_layer().set_option(option);
                    
                    //Set timeout on the following asio::ssl::stream::async_handshake
                    auto timer=get_timeout_timer(connection, config.timeout_request);
                    connection->socket->async_handshake(asio::ssl::stream_base::server, 
                            [this, connection, timer](const std::error_code& ec) {
                        if(timer)
                            timer->cancel();
                        if(!ec)
                            read_handshake(connection);
                    });
                }
            });
        }
    };

	typedef SocketServer<WSS> WssServer;
}

#endif /* SUPPORTS_WEBGL */

#endif	/* SERVER_WSS_HPP */

