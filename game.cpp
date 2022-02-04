#include "game.h"

game::game()
{
	points = std::vector<game_point>{
		game_point(0,0),
		game_point(100,100),
		game_point(100,200),
		game_point(300,200),
		game_point(123,321)
	};
}