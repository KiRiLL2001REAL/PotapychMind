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

bool Utility::isMatEqual(cv::Mat& lhs, cv::Mat& rhs)
{
	// Пустые матрицы являются одинаковыми
	if (lhs.empty() && rhs.empty())
		return true;
	// Матрицы разных типов НЕ одинаковы
	if (lhs.type() != rhs.type())
		return false;
	// Матрицы разных размерностей НЕ одинаковы
	if (lhs.cols != rhs.cols || lhs.rows != rhs.rows || lhs.dims != rhs.dims)
		return false;
	// Матрицы, указывающие на разные данные не одинаковы
	if (lhs.data != rhs.data)
		return false;
	// Поэлементное сравнение (результат - булева матрица), затем нахождение суммы и её сравнение с 0
	return cv::sum(lhs != rhs) == cv::Scalar(0, 0, 0, 0);
}
