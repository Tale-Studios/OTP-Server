#pragma once
#include <memory>

#include "core/global.h"
#include "core/Role.h"
#include "util/Datagram.h"
#include "util/DatagramIterator.h"
#include "net/NetworkClient.h"
#include "net/NetworkAcceptor.h"
#include "net/NetworkConnector.h"

// BungeeCord is a role in the daemon that sends and receives packets from a TCP socket
// between another BungeeCord on a different OTP cluster.
class BungeeCord : public Role, public NetworkHandler
{
  public:
    BungeeCord(RoleConfig roleconfig);
    ~BungeeCord();

    void initialize()
    {
        // Stub method for NetworkClient.
    }

    // receive_datagram is called when we have received a datagram
    // from the other end.
    void receive_datagram(DatagramHandle dg);

    // receive_disconnect is called when the other end closes
    // the connection or the connection is lost.
    void receive_disconnect(const uvw::ErrorEvent& evt);

    // handle_datagram receives messages from our own cluster
    // to relay across the cord.
    void handle_datagram(DatagramHandle in_dg, DatagramIterator &dgi);

  private:
    // init_cord causes the BungeeCord to either bind or connect to
    // a port to establish a network.
    void init_cord();

    // Server connection:
    void handle_connection(const std::shared_ptr<uvw::TcpHandle> &socket,
                           const uvw::Addr &remote,
                           const uvw::Addr &local,
                           const bool haproxy_mode);
    void handle_error(const uvw::ErrorEvent& evt);

    // Client connection:
    void on_connect(const std::shared_ptr<uvw::TcpHandle> &socket);
    void on_connect_error(const uvw::ErrorEvent& evt);

    // Whether we are binding to a port or connecting to a server.
    bool m_connecting;

    // Whether we are connected or not:
    bool m_connected;

    std::unique_ptr<LogCategory> m_log;
    std::unique_ptr<NetworkAcceptor> m_net_acceptor;
    std::shared_ptr<NetworkConnector> m_connector;
    std::shared_ptr<NetworkClient> m_client;
};
