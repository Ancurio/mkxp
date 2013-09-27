#ifndef PERFTIMER_H
#define PERFTIMER_H

struct PerfTimer
{
	virtual ~PerfTimer() {}
	virtual void startTiming() = 0;
	virtual void endTiming() = 0;
};

/* Create timers that run on either CPU or GPU.
 * After 'iter' pairs of startTiming()/endTiming(),
 * they will calculate the average measurement and
 * print it to the console */
PerfTimer *createGPUTimer(int iter);
PerfTimer *createCPUTimer(int iter);

#endif // PERFTIMER_H
