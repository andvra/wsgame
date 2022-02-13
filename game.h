#pragma once

#include <vector>

class game
{
public:
	class game_point
	{
	public:
		game_point(float x, float y) : x(x), y(y) {}
		float x, y;
	};
	class player
	{
	public:
		enum class Movements { Left, Right, Up, Down };
		player(unsigned int id, game_point pos) : id(id), pos(pos) {};
		game_point pos;
		unsigned int id;
		void add_movement(Movements m);
		void remove_movement(Movements m);
		bool has_movement(Movements m);
		void move(float delta_t);
	private:
		std::vector<Movements> movements;
	};
	game();
	~game();
	void move(float delta_t);
	void add_player(unsigned int id, game_point pos);
	void remove_player(unsigned int id);
	const std::vector<game_point*>& bezier_points() const;
	const std::vector<player*>& players() const;
	game::player* get_player(unsigned int id, unsigned int* idx = nullptr);
private:
	std::vector<game_point*> _bezier_points;
	std::vector<player*> _players;
};