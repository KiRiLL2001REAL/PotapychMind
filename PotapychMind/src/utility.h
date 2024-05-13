#pragma once

#include <string>
#include <opencv2/core.hpp>

class Utility final
{
public:
	static const std::string getWorkingDirectory();
	static const std::wstring to_wstring(const std::string& str);
	static bool isMatEqual(cv::Mat& lhs, cv::Mat& rhs);
};

