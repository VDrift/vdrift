#include "game.h"
#include "logging.h"
#include "pathmanager.h"

#ifndef _WIN32
#include <fenv.h>
#include <signal.h>
void release_mouse(int a)
{
	std::cout << "SIGABRT detected, releasing the mouse" << std::endl;
	SDL_WM_GrabInput(SDL_GRAB_OFF);
}
#endif

#include <list>
using std::list;

#include <string>
using std::string;

#include <iostream>
using std::endl;

#include <fstream>
#include <sstream>

int main (int argc, char * argv[])
{
#ifndef _WIN32
// catch fpe exceptions.
//      feenableexcept(FE_INVALID|FE_DIVBYZERO|FE_OVERFLOW);
//      feenableexcept(FE_ALL_EXCEPT);
	//handle an ABORT so we can release the mouse
	struct sigaction act;
	act.sa_handler = release_mouse;
	sigaction(SIGABRT,&act, NULL);
#endif

	list <string> args(argv, argv + argc);

	//find the path of the log file
	PATHMANAGER paths;
	std::stringstream dummy;
	paths.Init(dummy, dummy);
	string logfilename = paths.GetLogFile();

	//open the log file
	std::ofstream logfile(logfilename.c_str());
	if (!logfile)
	{
		std::cerr << "Couldn't open log file: " << logfilename << std::endl;
		return EXIT_FAILURE;
	}

	//set up the logging arrangement
	logging::splitterstreambuf infosplitter(std::cout, logfile);
	std::ostream infosplitterstream(&infosplitter);
	logging::splitterstreambuf errorsplitter(std::cerr, logfile);
	std::ostream errorsplitterstream(&errorsplitter);
	logging::logstreambuf infolog("INFO: ", infosplitterstream);
	//logstreambuf infolog("INFO: ", logfile);
	logging::logstreambuf errorlog("ERROR: ", errorsplitterstream);

	//primary logging ostreams
	std::ostream info_output(&infolog);
	std::ostream error_output(&errorlog);

	//create the game object
	GAME game(info_output, error_output);

	//kick it all off
	game.Start(args);

	info_output << "Exiting" << std::endl;

	return EXIT_SUCCESS;
}
