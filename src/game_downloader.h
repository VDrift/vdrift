/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _GAMEDOWNLOADER_H
#define _GAMEDOWNLOADER_H

#include <string>
#include <vector>

class Game;
class Http;

// a functor class to allow classes below the game class' hierarchy to download items using
// the full game-class functionality
class GameDownloader
{
public:
	GameDownloader(Game & gameref, const Http & httpref) : game(gameref), http(httpref) {}
	
	// the implementation for these is in game.cpp
	bool operator()(const std::string & file);
	bool operator()(const std::vector <std::string> & files);
	const Http & GetHttp() const {return http;}
	
private:
	Game & game;
	const Http & http;
};

#endif
