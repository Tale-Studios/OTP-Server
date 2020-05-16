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
    m_net_acceptor(nullptr), m_connector(nullptr), m_client(nullptr),
    m_connecting(false), m_connected(false)
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

BungeeCord::~BungeeCord()
{
    if(m_connected) {
        m_client->disconnect();
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

        m_connecting = true;

        ConnectCallback callback = std::bind(&BungeeCord::on_connect, this, std::placeholders::_1);
        ConnectErrorCallback err_callback = std::bind(&BungeeCord::on_connect_error, this, std::placeholders::_1);

        m_client = std::make_shared<NetworkClient>(this);
        m_connector = std::make_shared<NetworkConnector>(g_loop);
        m_connector->connect(connect_addr.get_rval(m_roleconfig), 7192, callback, err_callback);
    }
}

void BungeeCord::handle_connection(const std::shared_ptr<uvw::TcpHandle> &socket,
                                   const uvw::Addr &remote,
                                   const uvw::Addr &local,
                                   const bool haproxy_mode)
{
    if(m_connected) {
        // We're already connected...
        m_log->error() << "Got a redundant incoming connection from "
                       << remote.ip << ":" << remote.port << "\n";
        return;
    }

    m_log->info() << "Got an incoming connection from "
                  << remote.ip << ":" << remote.port << "\n";

    m_connected = true;

    m_client = std::make_shared<NetworkClient>(this);
    m_client->initialize(socket);
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

    m_connected = true;

    // Initialize the NetworkClient.
    m_client->initialize(socket);

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

void BungeeCord::receive_datagram(DatagramHandle dg)
{
    DatagramIterator dgi(dg);
    dgi.seek_payload();
    channel_t sender = dgi.read_channel();
    uint16_t msgtype = dgi.read_uint16();
    switch(msgtype) {
    case BUNGEECORD_ROUTE_MESSAGE: {
        // Put together the datagram:
        DatagramPtr datagram = Datagram::create(dgi.read_string());

        // Route it:
        route_datagram(datagram);
        break;
    }
    case BUNGEECORD_ROUTE_MESSAGES: {
        vector<string> messages;

        // Iterate over sent messages
        while(dgi.get_remaining() > 0) {
            messages.push_back(dgi.read_string());
        }

        // Route each message
        for(auto message : messages) {
            DatagramPtr datagram = Datagram::create(message);
            route_datagram(datagram);
        }
        break;
    }
    default:
        m_log->warning() << "Received unknown message over the cord: msgtype=" << msgtype << std::endl;
    }
}

void BungeeCord::receive_disconnect(const uvw::ErrorEvent& evt)
{
    m_connected = false;

    m_log->fatal() << "Lost connection to other end: " << evt.what() << "\n";

    // If we're connecting, we just want to shut down:
    if(m_connecting) {
        terminate();
        return;
    }
}

void BungeeCord::handle_datagram(DatagramHandle in_dg, DatagramIterator &dgi)
{
    // If we aren't connected, we can't route a message to anything:
    if(!m_connected) {
        m_log->warning() << "Received route message when not connected.\n";
        return;
    }

    channel_t sender = dgi.read_channel();
    uint16_t msgtype = dgi.read_uint16();
    switch(msgtype) {
    case BUNGEECORD_ROUTE_MESSAGE:
    case BUNGEECORD_ROUTE_MESSAGES: {
        // Route the message:
        m_client->send_datagram(in_dg);
        break;
    }
    default:
        m_log->warning() << "Received unknown internal message: msgtype=" << msgtype << std::endl;
    }
}

RoleFactoryItem<BungeeCord> bc_fact("bungeecord");
