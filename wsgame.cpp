#include <chrono>
#include "nlohmann/json.hpp"
#include "broadcast_server.h"
#include "game.h"

auto game_instance = game();

void post_game_status(broadcast_server& server);

void game_loop(broadcast_server& server)
{
	auto t_last = std::chrono::high_resolution_clock::now();
	while (1)
	{
		auto t_now = std::chrono::high_resolution_clock::now();
		auto t_delta_us = std::chrono::duration_cast<std::chrono::microseconds>(t_now - t_last);
		auto t_delta_s = static_cast<float>(t_delta_us.count() / 1000000.0f);
		t_last = t_now;
		game_instance.move(t_delta_s);
		post_game_status(server);
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

nlohmann::json get_bezier_pts()
{
	auto bezier_pts = nlohmann::json::array();
	for (auto& pt : game_instance.bezier_points())
		bezier_pts.push_back(nlohmann::json::object({ {"x", pt->x}, {"y", pt->y} }));

	return bezier_pts;
}

nlohmann::json get_player_pts()
{
	auto player_pts = nlohmann::json::array();
	for (auto& player : game_instance.players())
		player_pts.push_back(nlohmann::json::object({ {"x", player->pos.x}, {"y", player->pos.y} }));

	return player_pts;
}

void post_game_status(broadcast_server& server)
{
	nlohmann::json json = {
	{"id", 0},
	{"bezier_pts", get_bezier_pts()},
	{"player_pts", get_player_pts()}
	};
	auto s = json.dump();
	server.post_data(s);
}

void process_client_connect(unsigned int connection_id)
{
	std::cout << "Client with ID " << connection_id << " connected" << std::endl;
	game_instance.add_player(connection_id, game::game_point(100 + connection_id % 500, 100 + (connection_id / 1000) % 500));
}

void process_client_disconnect(unsigned int connection_id)
{
	std::cout << "Client with ID " << connection_id << " disconnected" << std::endl;
}

void add_player_movement(game::player* player, game::player::Movements movement, bool activate)
{
	if (activate)
		player->add_movement(movement);
	else
		player->remove_movement(movement);
}

void process_client_message(unsigned int connection_id, std::string msg)
{
	try
	{
		auto json = nlohmann::json::parse(msg);
		auto el_type = json.find("type");
		auto el_left = json.find("left");
		auto el_right = json.find("right");
		auto el_up = json.find("up");
		auto el_down = json.find("down");
		auto player = game_instance.get_player(connection_id);
		if ((el_type != json.end())
			&& (el_left != json.end())
			&& (el_right != json.end())
			&& (el_up != json.end())
			&& (el_down != json.end())
			&& (player != nullptr))
		{
			auto msg_type = el_type.value();
			if (msg_type == "keys")
			{
				auto msg_left = el_left.value();
				auto msg_right = el_right.value();
				auto msg_up = el_up.value();
				auto msg_down = el_down.value();
				add_player_movement(player, game::player::Movements::Left, msg_left);
				add_player_movement(player, game::player::Movements::Right, msg_right);
				add_player_movement(player, game::player::Movements::Up, msg_up);
				add_player_movement(player, game::player::Movements::Down, msg_down);
			}
		}
	}
	catch (const std::exception& exc)
	{
		// Catch anything thrown within try block that derives from std::exception
		std::cerr << exc.what();
	}
	catch (...)
	{
		// Catch anything else. We don't know what the issue is, though
		std::cout << "Couldn't parse client message" << std::endl;
	}
}

int main() {
	const int port = 9002;

	try {
		broadcast_server server_instance;

		thread t1(bind(&broadcast_server::process_messages, &server_instance));
		thread t2(game_loop, std::ref(server_instance));

		server_instance.run(port, &process_client_message, &process_client_connect, &process_client_disconnect);

		t1.join();
		t2.join();
	}
	catch (websocketpp::exception const& e) {
		std::cout << e.what() << std::endl;
	}
}