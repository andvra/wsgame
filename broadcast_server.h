#pragma once

#include <set>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/common/thread.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::connection_hdl;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

using websocketpp::lib::thread;
using websocketpp::lib::mutex;
using websocketpp::lib::lock_guard;
using websocketpp::lib::unique_lock;
using websocketpp::lib::condition_variable;

enum action_type {
    SUBSCRIBE,
    UNSUBSCRIBE,
    MESSAGE
};

struct action {
    action(action_type t, connection_hdl h) : type(t), hdl(h) {}
    action(action_type t, connection_hdl h, server::message_ptr m)
        : type(t), hdl(h), msg(m) {}

    action_type type;
    websocketpp::connection_hdl hdl;
    server::message_ptr msg;
};

class broadcast_server {
public:
    broadcast_server();
    void run(uint16_t port,
        std::function<void(unsigned int, std::string)> client_message_callback,
        std::function<void(unsigned int)> client_connect_callback,
        std::function<void(unsigned int)> client_disconnect_callback);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, server::message_ptr msg);
    void process_messages();
    void post_data(std::string msg);
private:
    void set_log_levels();
    typedef std::set<connection_hdl, std::owner_less<connection_hdl>> con_list;
    unsigned int get_connection_id(const connection_hdl& hdl);
    std::string get_connection_address(const connection_hdl& hdl);
    int get_connection_port(const connection_hdl& hdl);
    server m_server;
    con_list m_connections;
    std::queue<action> m_actions;

    mutex m_action_lock;
    mutex m_connection_lock;
    condition_variable m_action_cond;
    std::function<void(int, std::string)> client_msg_callback;
    std::function<void(int)> client_connect_callback;
    std::function<void(int)> client_disconnect_callback;
};