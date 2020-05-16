#include "core/global.h"
#include "core/msgtypes.h"
#include "core/RoleFactory.h"
#include "config/ConfigVariable.h"
#include "config/constraints.h"
#include "net/TcpAcceptor.h"
#include "net/NetworkConnector.h"
#include "BungeeCord.h"

static RoleConfigGroup bc_config("bungeecord");
static ConfigVariable<channel_t> control_channel("control", INVALID_CHANNEL, bc_config);
static ConfigVariable<std::string> bind_addr("bind", "unspecified", bc_config);
static ConfigVariable<std::string> connect_addr("connect", "unspecified", bc_config);
static ValidAddressConstraint valid_bind_addr(bind_addr);
static ValidAddressConstraint valid_connect_addr(connect_addr);

BungeeCord::BungeeCord(RoleConfig roleconfig) : Role(roleconfig),
    m_net_acceptor(nullptr), m_connector(nullptr)//, m_client(nullptr)
{
    // Get our channel we've been assigned to:
    channel_t channel = control_channel.get_rval(m_roleconfig);
    if(channel != INVALID_CHANNEL) {
        // Subscribe to our channel.
        subscribe_channel(channel);

        // Update our name.
        std::stringstream name;
        name << "BungeeCord(" << channel << ")";
        m_log = std::unique_ptr<LogCategory>(new LogCategory("bungeecord", name.str()));
        set_con_name(name.str());

        // Initialize our cord.
        init_cord();
    }
}

void BungeeCord::init_cord()
{
    // Bind to a port and instantiate a TcpAcceptor.
    if(bind_addr.get_rval(m_roleconfig) != "unspecified") {
        TcpAcceptorCallback callback = std::bind(&BungeeCord::handle_connection,
                                                 this, std::placeholders::_1,
                                                 std::placeholders::_2,
                                                 std::placeholders::_3,
                                                 std::placeholders::_4);

        AcceptorErrorCallback err_callback = std::bind(&BungeeCord::handle_error,
                                                       this, std::placeholders::_1);

        m_net_acceptor = std::unique_ptr<TcpAcceptor>(new TcpAcceptor(callback, err_callback));
        m_net_acceptor->bind(bind_addr.get_rval(m_roleconfig), 7192);
        m_net_acceptor->start();

        m_log->info() << "Opened server." << std::endl;
    }

    // Establish a connection.
    if(connect_addr.get_rval(m_roleconfig) != "unspecified") {
        m_log->info() << "Connecting to server..." << std::endl;

        ConnectCallback callback = std::bind(&BungeeCord::on_connect, this, std::placeholders::_1);
        ConnectErrorCallback err_callback = std::bind(&BungeeCord::on_connect_error, this, std::placeholders::_1);

        //m_client = std::make_shared<NetworkClient>();
        m_connector = std::make_shared<NetworkConnector>(g_loop);
        m_connector->connect(connect_addr.get_rval(m_roleconfig), 7192, callback, err_callback);
    }
}

void BungeeCord::handle_connection(const std::shared_ptr<uvw::TcpHandle> &socket,
                                   const uvw::Addr &remote,
                                   const uvw::Addr &local,
                                   const bool haproxy_mode)
{
    m_log->info() << "Got an incoming connection from "
                  << remote.ip << ":" << remote.port << "\n";
}

void BungeeCord::handle_error(const uvw::ErrorEvent& evt)
{
    if(evt.code() == UV_EADDRINUSE || evt.code() == UV_EADDRNOTAVAIL) {
        // We failed to bind. Fall over and die.
        m_log->fatal() << "Failed to bind to address: " << evt.what() << "\n";
        terminate();
    }
}

void BungeeCord::on_connect(const std::shared_ptr<uvw::TcpHandle> &socket)
{
    if(socket == nullptr) {
        // We failed to establish a connection. Fall over and die.
        m_log->fatal() << "Failed to establish connection\n";
        terminate();
    }

    m_log->info() << "Successfully connected to server\n";

    // Initialize the NetworkClient.
    //m_client->initialize(socket);

    // Destroy the NetworkConnector we used to connect to the server.
    m_connector->destroy();
    m_connector = nullptr;
}

void BungeeCord::on_connect_error(const uvw::ErrorEvent& evt)
{
    // We failed to connect. Fall over and die.
    m_log->fatal() << "Failed to connect to address: " << evt.what() << "\n";
    terminate();
}

void BungeeCord::handle_datagram(DatagramHandle, DatagramIterator &dgi)
{
    channel_t sender = dgi.read_channel();
    uint16_t msgtype = dgi.read_uint16();
    switch(msgtype) {
    default:
        m_log->warning() << "Received unknown message: msgtype=" << msgtype << std::endl;
    }
}

RoleFactoryItem<BungeeCord> bc_fact("bungeecord");
