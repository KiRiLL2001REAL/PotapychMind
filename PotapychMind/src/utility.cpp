#include "utility.h"

#include <direct.h>
#include <iostream>

const std::string Utility::getWorkingDirectory()
{
	char* buffer;
	if ((buffer = _getcwd(NULL, 0)) == NULL)
	{
		perror("_getcwd error");
		return "";
	}
	auto workingDirectory = std::string(buffer);
	free(buffer);
	return workingDirectory;
}

const std::wstring Utility::to_wstring(const std::string& str)
{
	return std::wstring(str.begin(), str.end());
}
