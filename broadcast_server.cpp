#include <websocketpp/connection.hpp>
#include <websocketpp/endpoint.hpp>

#include "broadcast_server.h"

broadcast_server::broadcast_server()
{
    // Initialize Asio Transport
    m_server.init_asio();

    // Register handler callbacks
    m_server.set_open_handler(bind(&broadcast_server::on_open, this, ::_1));
    m_server.set_close_handler(bind(&broadcast_server::on_close, this, ::_1));
    m_server.set_message_handler(bind(&broadcast_server::on_message, this, ::_1, ::_2));

    set_log_levels();
}

void broadcast_server::set_log_levels()
{
    // Disable outputing frame information to the console
    m_server.clear_access_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload);
}

void broadcast_server::run(uint16_t port,
    std::function<void(unsigned int, std::string)> client_msg_callback_fn,
    std::function<void(unsigned int)> client_connect_callback_fn,
    std::function<void(unsigned int)> client_disconnect_callback_fn) {
    // listen on specified port
    m_server.listen(port);

    // Start the server accept loop
    m_server.start_accept();

    client_msg_callback = client_msg_callback_fn;
    client_connect_callback = client_connect_callback_fn;
    client_disconnect_callback = client_disconnect_callback_fn;

    // Start the ASIO io_service run loop
    try {
        std::cout << "Starting up server at port " << port << "..." << std::endl;
        m_server.run();
    }
    catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
}

void broadcast_server::on_open(connection_hdl hdl) {
    {
        lock_guard<mutex> guard(m_action_lock);
        //std::cout << "on_open" << std::endl;
        m_actions.push(action(SUBSCRIBE, hdl));
    }
    m_action_cond.notify_one();
}

void broadcast_server::on_close(connection_hdl hdl) {
    {
        lock_guard<mutex> guard(m_action_lock);
        //std::cout << "on_close" << std::endl;
        m_actions.push(action(UNSUBSCRIBE, hdl));
    }
    m_action_cond.notify_one();
}

void broadcast_server::on_message(connection_hdl hdl, server::message_ptr msg) {
    // queue message up for sending by processing thread
    {
        lock_guard<mutex> guard(m_action_lock);
        //std::cout << "on_message" << std::endl;

        m_actions.push(action(MESSAGE, hdl, msg));
    }
    m_action_cond.notify_one();
}

void broadcast_server::post_data(std::string msg)
{
    lock_guard<mutex> guard(m_connection_lock);
    con_list::iterator it;

    for (it = m_connections.begin(); it != m_connections.end(); ++it)
    {
        auto con = m_server.get_con_from_hdl(*it);
        con->send(msg, websocketpp::frame::opcode::text);
    }
}

std::string broadcast_server::get_connection_address(const connection_hdl& hdl)
{
    auto ep = m_server.get_con_from_hdl(hdl)->get_raw_socket().remote_endpoint();
    auto address = ep.address().to_string();

    return address;
}

int broadcast_server::get_connection_port(const connection_hdl& hdl)
{
    auto ep = m_server.get_con_from_hdl(hdl)->get_raw_socket().remote_endpoint();
    auto port = ep.port();

    return port;
}

unsigned int broadcast_server::get_connection_id(const connection_hdl& hdl)
{
    auto ret = *(int*)hdl.lock().get();

    return ret;
}

void broadcast_server::process_messages() {
    while (1) {
        unique_lock<mutex> lock(m_action_lock);

        while (m_actions.empty()) {
            m_action_cond.wait(lock);
        }

        action a = m_actions.front();
        m_actions.pop();

        lock.unlock();

        auto client_id = get_connection_id(a.hdl);
        
        if (a.type == SUBSCRIBE) {
            lock_guard<mutex> guard(m_connection_lock);
            m_connections.insert(a.hdl);
            client_connect_callback(client_id);
        }
        else if (a.type == UNSUBSCRIBE) {
            lock_guard<mutex> guard(m_connection_lock);
            m_connections.erase(a.hdl);
            client_disconnect_callback(client_id);
        }
        else if (a.type == MESSAGE) {
            lock_guard<mutex> guard(m_connection_lock);
            auto payload = a.msg.get()->get_payload();
            client_msg_callback(client_id, payload);
        }
        else {
            // undefined.
        }
    }
}