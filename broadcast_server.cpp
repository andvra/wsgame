#include "broadcast_server.h"

broadcast_server::broadcast_server()
{
    // Initialize Asio Transport
    m_server.init_asio();

    // Register handler callbacks
    m_server.set_open_handler(bind(&broadcast_server::on_open, this, ::_1));
    m_server.set_close_handler(bind(&broadcast_server::on_close, this, ::_1));
    m_server.set_message_handler(bind(&broadcast_server::on_message, this, ::_1, ::_2));
}

void broadcast_server::run(uint16_t port) {
    // listen on specified port
    m_server.listen(port);

    // Start the server accept loop
    m_server.start_accept();

    // Start the ASIO io_service run loop
    try {
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

void broadcast_server::process_messages() {
    while (1) {
        unique_lock<mutex> lock(m_action_lock);

        while (m_actions.empty()) {
            m_action_cond.wait(lock);
        }

        action a = m_actions.front();
        m_actions.pop();

        lock.unlock();

        if (a.type == SUBSCRIBE) {
            lock_guard<mutex> guard(m_connection_lock);
            m_connections.insert(a.hdl);
        }
        else if (a.type == UNSUBSCRIBE) {
            lock_guard<mutex> guard(m_connection_lock);
            m_connections.erase(a.hdl);
        }
        else if (a.type == MESSAGE) {
            lock_guard<mutex> guard(m_connection_lock);

            con_list::iterator it;
            for (it = m_connections.begin(); it != m_connections.end(); ++it) {
                m_server.send(*it, a.msg);
            }
        }
        else {
            // undefined.
        }
    }
}