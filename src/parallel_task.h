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

//a simple parallel task system where a task is its own worker thread

#ifndef _PARALLEL_TASK_H_
#define _PARALLEL_TASK_H_

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#include <cassert>
#include <iostream>

//#define VERBOSE

namespace Parallel
{

int Dispatch(void * data);

class Task
{
	private:
		bool quit;

		SDL_sem * sem_frame_start;
		SDL_sem * sem_frame_end;

		bool executing;

		SDL_Thread * thread;

	public:
		Task() : quit(false), sem_frame_start(NULL), sem_frame_end(NULL), executing(false), thread(NULL)
		{}

		void Init()
		{
			//don't allow double init
			assert (!sem_frame_start);
			assert (!sem_frame_end);
			assert (!thread);

			quit = false;
			sem_frame_start = SDL_CreateSemaphore(0);
			sem_frame_end = SDL_CreateSemaphore(0);
			thread = SDL_CreateThread(Dispatch, NULL, this);
		}

		~Task()
		{
			Deinit();
		}

		void Deinit()
		{
			//don't allow double deinit, or deinit with no init
			if (!(sem_frame_start && sem_frame_end && thread))
				return;

			quit = true;
			//at this point our thread is waiting for the frame to start, so have it do one last iteration so it can see the quit flag
#ifdef VERBOSE
			std::cout << "telling child to exit" << std::endl;
#endif
			SDL_SemPost(sem_frame_start);
#ifdef VERBOSE
			std::cout << "waiting for thread to exit" << std::endl;
#endif
			//wait for the thread to exit
			SDL_WaitThread(thread, NULL);
#ifdef VERBOSE
			std::cout << "thread has exited" << std::endl;
#endif
			thread = NULL;

			SDL_DestroySemaphore(sem_frame_start);
			SDL_DestroySemaphore(sem_frame_end);
#ifdef VERBOSE
			std::cout << "destroyed semaphores" << std::endl;
#endif

			sem_frame_start = NULL;
			sem_frame_end = NULL;
		}

		void Run()
		{
#ifdef VERBOSE
			std::cout << "child thread run" << std::endl;
#endif
			this->Setup();

#ifdef VERBOSE
			std::cout << "posting end semaphore to allow the main loop to run through the first iteration" << std::endl;
#endif
			SDL_SemPost(sem_frame_end);

			while (!quit)
			{
#ifdef VERBOSE
				std::cout << "waiting for start semaphore" << std::endl;
#endif
				SDL_SemWait(sem_frame_start);
#ifdef VERBOSE
				std::cout << "done waiting for start semaphore" << std::endl;
#endif

				executing = true;
				if (!quit) //avoid doing the last tick if we don't have to
					this->Execute();
				executing = false;

#ifdef VERBOSE
				std::cout << "posting end semaphore" << std::endl;
#endif
				SDL_SemPost(sem_frame_end);
			}
#ifdef VERBOSE
			std::cout << "child thread exit" << std::endl;
#endif
		}

		void Start()
		{
#ifdef VERBOSE
			std::cout << "posting start semaphore" << std::endl;
#endif
			SDL_SemPost(sem_frame_start);
		}

		void End()
		{
#ifdef VERBOSE
			std::cout << "waiting for end semaphore" << std::endl;
#endif
			SDL_SemWait(sem_frame_end);
#ifdef VERBOSE
			std::cout << "done waiting for end semaphore" << std::endl;
#endif
		}

		virtual void Execute() = 0;
		virtual void Setup() {}

		bool GetExecuting() const
		{
			return executing;
		}
};

}

#endif
