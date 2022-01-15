#include "debugwriter.h"

std::stringstream Debug::queueBuf = std::stringstream();
bool Debug::queue = false;

Debug::Debug()
{
    getBuf() << std::boolalpha;
}

Debug::~Debug()
{
#ifdef __ANDROID__
	__android_log_write(ANDROID_LOG_DEBUG, "mkxp", buf.str().c_str());
#else
	if (queue)
		getBuf() << std::endl;
	else
		std::cerr << buf.str() << std::endl;
#endif
}

std::stringstream &Debug::getBuf()
{
	return queue ? queueBuf : buf;
}

void Debug::startQueueing()
{
	queue = true;
	queueBuf.clear();
	queueBuf << std::boolalpha;
}

void Debug::stopQueueing()
{
	queue = false;
	const std::string str = queueBuf.str();

	if (str.length() > 0) {
		std::cerr << str << std::endl;
	}

	queueBuf.clear();
}
