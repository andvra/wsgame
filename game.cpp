#include "game.h"

game::game()
{
	_bezier_points = std::vector<game_point*>{
		new game_point(0,0),
		new game_point(100,100),
		new game_point(100,200),
		new game_point(300,200),
		new game_point(123,321)
	};
}

game::~game()
{
	for (auto p : _bezier_points)
		delete p;
	for (auto p : _players)
		delete p;
}

game::player* game::get_player(unsigned int id, unsigned int* idx)
{
	auto p = std::find_if(_players.begin(), _players.end(), [&id](player* p) {return p->id == id; });

	if (p == _players.end())
		return nullptr;
	else
	{
		if (idx != nullptr)
			*idx = std::distance(_players.begin(), p);
		return *p;
	}

}
void game::add_player(unsigned int id, game_point pos)
{
	if (get_player(id) == nullptr)
		_players.push_back(new player(id, pos));
}

void game::remove_player(unsigned int id)
{
	unsigned int idx;
	auto player = get_player(id, &idx);

	if (player != nullptr)
	{
		delete player;
		_players.erase(_players.begin() + idx);
	}
}

const std::vector<game::game_point*>& game::bezier_points() const
{
	return _bezier_points;
}

const std::vector<game::player*>& game::players() const
{
	return _players;
}

void game::player::add_movement(Movements m)
{
	auto it = std::find(movements.begin(), movements.end(), m);
	if (it == movements.end())
		movements.push_back(m);
}

void game::player::remove_movement(Movements m)
{
	auto it = std::find(movements.begin(), movements.end(), m);
	if (it != movements.end())
		movements.erase(it);
}

bool game::player::has_movement(Movements m)
{
	auto it = std::find(movements.begin(), movements.end(), m);

	return it != movements.end();
}

void game::move(float delta_t)
{
	for (auto p : _players)
		p->move(delta_t);
}

void game::player::move(float delta_t)
{
	const float speed_factor = 50;

	if (has_movement(game::player::Movements::Left))
		pos.x -= delta_t * speed_factor;
	if (has_movement(game::player::Movements::Right))
		pos.x += delta_t * speed_factor;
	if (has_movement(game::player::Movements::Up))
		pos.y -= delta_t * speed_factor;
	if (has_movement(game::player::Movements::Down))
		pos.y += delta_t * speed_factor;
}