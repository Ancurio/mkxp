#ifndef SDLUTIL_H
#define SDLUTIL_H

#include <SDL_atomic.h>
#include <SDL_thread.h>

#include <string>

struct AtomicFlag
{
	AtomicFlag()
	{
		clear();
	}

	void set()
	{
		SDL_AtomicSet(&atom, 1);
	}

	void clear()
	{
		SDL_AtomicSet(&atom, 0);
	}

	operator bool() const
	{
		return SDL_AtomicGet(&atom);
	}

private:
	mutable SDL_atomic_t atom;
};

template<class C, void (C::*func)()>
int __sdlThreadFun(void *obj)
{
	(static_cast<C*>(obj)->*func)();
	return 0;
}

template<class C, void (C::*func)()>
SDL_Thread *createSDLThread(C *obj, const std::string &name = std::string())
{
	return SDL_CreateThread(__sdlThreadFun<C, func>, name.c_str(), obj);
}

#endif // SDLUTIL_H
