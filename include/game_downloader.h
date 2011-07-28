#ifndef _GAMEDOWNLOADER_H
#define _GAMEDOWNLOADER_H

#include <string>
#include <vector>

class GAME;
class HTTP;

// a functor class to allow classes below the game class' hierarchy to download items using
// the full game-class functionality
class GAME_DOWNLOADER
{
public:
	GAME_DOWNLOADER(GAME & gameref, const HTTP & httpref) : game(gameref), http(httpref) {}
	
	// the implementation for these is in game.cpp
	bool operator()(const std::string & file);
	bool operator()(const std::vector <std::string> & files);
	const HTTP & GetHttp() const {return http;}
	
private:
	GAME & game;
	const HTTP & http;
};

#endif
