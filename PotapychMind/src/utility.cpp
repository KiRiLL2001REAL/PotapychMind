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
