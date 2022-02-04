#pragma once

#include <vector>

class game
{
public:
	class game_point
	{
	public:
		game_point(float x, float y) :x(x), y(y) {}
		float x, y;
	};
	game();
	std::vector<game_point> points;
};