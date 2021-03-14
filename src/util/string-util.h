#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <string>

// Copies the given text with additional newlines every X characters.
// Saves the output into the given std::string object.
// Newlines are only inserted on spaces (' ') or tabs ('\t').
void
copyWithNewlines(const char *input, std::string &output, const unsigned limit)
{
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
}

void
copyWithNewlines(const std::string& input, std::string& output, const unsigned limit)
{
	copyWithNewlines(input.c_str(), output, limit);
}

#endif // STRING_UTIL_H
