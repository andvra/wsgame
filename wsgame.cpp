#include "nlohmann/json.hpp"
#include "broadcast_server.h"
#include "game.h"

void post_game_status(broadcast_server& server, game& game)
{
	unsigned int cnt = 0;
	while (1)
	{
		auto jsonp = nlohmann::json::array();
		for (auto& pt : game.points)
			jsonp.push_back(nlohmann::json::object({ {"x", pt.x}, {"y", pt.y} }));
		game.points[0].x = (3*cnt++) % 300;
		nlohmann::json json = {
		{"id", cnt},
		{"pts", jsonp},
		{"msg", "Here's a message for you"}
		};
		auto s = json.dump();
		server.post_data(s);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void process_client_message(std::string msg)
{
	std::cout << "Got message from client: " << msg << std::endl;
	try
	{
		auto json = nlohmann::json::parse(msg);
		std::cout << "Message in JSON is " << json.at("msg") << std::endl;
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

	auto game_instance = game();

	try {
		broadcast_server server_instance;

		// Start a thread to run the processing loop
		thread t1(bind(&broadcast_server::process_messages, &server_instance));
		thread t2(post_game_status, std::ref(server_instance), std::ref(game_instance));

		// Run the asio loop with the main thread
		server_instance.run(9002, &process_client_message);

		t1.join();
		t2.join();
	}
	catch (websocketpp::exception const& e) {
		std::cout << e.what() << std::endl;
	}
}