#include "perftimer.h"

#include "SDL_timer.h"
#include "glew.h"

#include <QDebug>

struct TimerQuery
{
	GLuint query;
	static bool queryActive;
	bool thisQueryActive;

	TimerQuery()
	    : thisQueryActive(false)
	{
		glGenQueries(1, &query);
	}

	void begin()
	{
		if (queryActive)
			return;

		if (thisQueryActive)
			return;

		glBeginQuery(GL_TIME_ELAPSED, query);
		queryActive = true;
		thisQueryActive = true;
	}

	void end()
	{
		if (!thisQueryActive)
			return;

		glEndQuery(GL_TIME_ELAPSED);
		queryActive = false;
		thisQueryActive = false;
	}

	bool getResult(GLuint64 *result)
	{
		if (thisQueryActive)
			return false;

		GLint isReady = GL_FALSE;
		glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &isReady);

		if (isReady != GL_TRUE)
			return false;

		glGetQueryObjectui64v(query, GL_QUERY_RESULT, result);

		if (glGetError() == GL_INVALID_OPERATION)
			return false;

		return true;
	}

	GLuint64 getResultSync()
	{
		if (thisQueryActive)
			return 0;

		GLuint64 result;
		GLint isReady = GL_FALSE;

		while (isReady == GL_FALSE)
			glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &isReady);

		glGetQueryObjectui64v(query, GL_QUERY_RESULT, &result);

		return result;
	}

	~TimerQuery()
	{
		if (thisQueryActive)
			end();

		glDeleteQueries(1, &query);
	}
};

bool TimerQuery::queryActive = false;

struct GPUTimerGLQuery : public PerfTimer
{
	TimerQuery queries[2];
	const int iter;

	uint8_t ind;
	uint64_t acc;
	int32_t counter;
	bool first;

	GPUTimerGLQuery(int iter)
	    : iter(iter),
	      ind(0),
	      acc(0),
	      counter(0),
	      first(true)
	{}

	void startTiming()
	{
		queries[ind].begin();
	}

	void endTiming()
	{
		queries[ind].end();

		swapInd();

		if (first)
		{
			first = false;
			return;
		}

		GLuint64 result;
		if (!queries[ind].getResult(&result))
			return;

		acc += result;

		if (++counter < iter)
			return;

		qDebug() << "                                  "
		            "Avg. GPU time:" << ((double) acc / (iter * 1000 * 1000)) << "ms";

		acc = counter = 0;
	}

	void swapInd()
	{
		ind = ind ? 0 : 1;
	}
};

struct GPUTimerDummy : public PerfTimer
{
	void startTiming() {}
	void endTiming() {}
};

struct CPUTimer : public PerfTimer
{
	const int iter;

	uint64_t acc;
	int32_t counter;
	Uint64 ticks;
	Uint64 perfFreq;

	CPUTimer(int iter)
	    : iter(iter),
	      acc(0),
	      counter(0),
	      ticks(0)
	{
		perfFreq = SDL_GetPerformanceFrequency();
	}

	void startTiming()
	{
		ticks = SDL_GetPerformanceCounter();
	}

	void endTiming()
	{
		acc += SDL_GetPerformanceCounter() - ticks;

		if (++counter < iter)
			return;

		qDebug() << "Avg. CPU time:" << ((double) acc / (iter * (perfFreq / 1000))) << "ms";
		acc = counter = 0;
	}
};

PerfTimer *createCPUTimer(int iter)
{
	return new CPUTimer(iter);
}

PerfTimer *createGPUTimer(int iter)
{
	if (GLEW_EXT_timer_query)
	{
		return new GPUTimerGLQuery(iter);
	}
	else
	{
		qDebug() << "GL_EXT_timer_query not present: cannot measure GPU performance";
		return new GPUTimerDummy();
	}
}
