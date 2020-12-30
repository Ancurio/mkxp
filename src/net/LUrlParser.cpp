/*
 * Lightweight URL & URI parser (RFC 1738, RFC 3986)
 * https://github.com/corporateshark/LUrlParser
 *
 * The MIT License (MIT)
 *
 * Copyright (C) 2015-2020 Sergey Kosarevsky (sk@linderdaum.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "LUrlParser.h"

#include <algorithm>
#include <cstring>
#include <stdlib.h>

namespace
{
	// check if the scheme name is valid
	bool isSchemeValid(const std::string& schemeName)
	{
		for (auto c : schemeName)
		{
			if (!isalpha(c) && c != '+' && c != '-' && c != '.') return false;
		}

		return true;
	}
}

bool LUrlParser::ParseURL::getPort(int* outPort) const
{
	if (!isValid()) { return false; }

	const int port = atoi(port_.c_str());

	if (port <= 0 || port > 65535) { return false; }

	if (outPort) { *outPort = port; }

	return true;
}

// based on RFC 1738 and RFC 3986
LUrlParser::ParseURL LUrlParser::ParseURL::parseURL(const std::string& URL)
{
	LUrlParser::ParseURL result;

	const char* currentString = URL.c_str();

	/*
	 *	<scheme>:<scheme-specific-part>
	 *	<scheme> := [a-z\+\-\.]+
	 *	For resiliency, programs interpreting URLs should treat upper case letters as equivalent to lower case in scheme names
	 */

	 // try to read scheme
	{
		const char* localString = strchr(currentString, ':');

		if (!localString)
		{
			return ParseURL(LUrlParserError_NoUrlCharacter);
		}

		// save the scheme name
		result.scheme_ = std::string(currentString, localString - currentString);

		if (!isSchemeValid(result.scheme_))
		{
			return ParseURL(LUrlParserError_InvalidSchemeName);
		}

		// scheme should be lowercase
		std::transform(result.scheme_.begin(), result.scheme_.end(), result.scheme_.begin(), ::tolower);

		// skip ':'
		currentString = localString + 1;
	}

	/*
	 *	//<user>:<password>@<host>:<port>/<url-path>
	 *	any ":", "@" and "/" must be normalized
	 */

	 // skip "//"
	if (*currentString++ != '/') return ParseURL(LUrlParserError_NoDoubleSlash);
	if (*currentString++ != '/') return ParseURL(LUrlParserError_NoDoubleSlash);

	// check if the user name and password are specified
	bool bHasUserName = false;

	const char* localString = currentString;

	while (*localString)
	{
		if (*localString == '@')
		{
			// user name and password are specified
			bHasUserName = true;
			break;
		}
		else if (*localString == '/')
		{
			// end of <host>:<port> specification
			bHasUserName = false;
			break;
		}

		localString++;
	}

	// user name and password
	localString = currentString;

	if (bHasUserName)
	{
		// read user name
		while (*localString && *localString != ':' && *localString != '@') localString++;

		result.userName_ = std::string(currentString, localString - currentString);

		// proceed with the current pointer
		currentString = localString;

		if (*currentString == ':')
		{
			// skip ':'
			currentString++;

			// read password
			localString = currentString;

			while (*localString && *localString != '@') localString++;

			result.password_ = std::string(currentString, localString - currentString);

			currentString = localString;
		}

		// skip '@'
		if (*currentString != '@')
		{
			return ParseURL(LUrlParserError_NoAtSign);
		}

		currentString++;
	}

	const bool bHasBracket = (*currentString == '[');

	// go ahead, read the host name
	localString = currentString;

	while (*localString)
	{
		if (bHasBracket && *localString == ']')
		{
			// end of IPv6 address
			localString++;
			break;
		}
		else if (!bHasBracket && (*localString == ':' || *localString == '/'))
		{
			// port number is specified
			break;
		}

		localString++;
	}

	result.host_ = std::string(currentString, localString - currentString);

	currentString = localString;

	// is port number specified?
	if (*currentString == ':')
	{
		currentString++;

		// read port number
		localString = currentString;

		while (*localString && *localString != '/') localString++;

		result.port_ = std::string(currentString, localString - currentString);

		currentString = localString;
	}

	// end of string
	if (!*currentString)
	{
		result.errorCode_ = LUrlParserError_Ok;

		return result;
	}

	// skip '/'
	if (*currentString != '/')
	{
		return ParseURL(LUrlParserError_NoSlash);
	}

	currentString++;

	// parse the path
	localString = currentString;

	while (*localString && *localString != '#' && *localString != '?') localString++;

	result.path_ = std::string(currentString, localString - currentString);

	currentString = localString;

	// check for query
	if (*currentString == '?')
	{
		// skip '?'
		currentString++;

		// read query
		localString = currentString;

		while (*localString&&* localString != '#') localString++;

		result.query_ = std::string(currentString, localString - currentString);

		currentString = localString;
	}

	// check for fragment
	if (*currentString == '#')
	{
		// skip '#'
		currentString++;

		// read fragment
		localString = currentString;

		while (*localString) localString++;

		result.fragment_ = std::string(currentString, localString - currentString);

		currentString = localString;
	}

	result.errorCode_ = LUrlParserError_Ok;

	return result;
}
