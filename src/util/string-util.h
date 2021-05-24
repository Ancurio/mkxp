#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#ifndef __APPLE__

#include <string>

// Copies the given text with additional newlines every X characters.
// Saves the output into a new std::string object.
// Newlines are only inserted on spaces (' ') or tabs ('\t').
static std::string copyWithNewlines(const char *input, const unsigned limit)
{
    std::string output;
	unsigned noNewlineCount = 0;
	
	while (*input != '\0')
	{
		if ((*input == ' ' || *input == '\t') && noNewlineCount >= limit)
		{
			output += '\n';
			noNewlineCount = 0;
		}
		else 
		{
			output += *input;
			
			if (*input == '\n')
				noNewlineCount = 0;
			else
				noNewlineCount++;
		}

		input++;
	}
    return output;
}

/*
static std::string copyWithNewlines(const std::string& input, const unsigned limit)
{
	return copyWithNewlines(input.c_str(), limit);
}
 */

#endif

#endif // STRING_UTIL_H
