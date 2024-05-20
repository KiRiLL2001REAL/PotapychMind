#include "utility.h"

#include <direct.h>
#include <iostream>
#include <windows.h>
#include <exception>

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

// https://stackoverflow.com/questions/14184709/is-this-code-safe-using-wstring-with-multibytetowidechar
const std::wstring Utility::to_wstring(const std::string& str)
{
	std::wstring out = L"";

#pragma warning ( push )
#pragma warning ( disable: 4267 )
	if (str.length() > 0)
	{
		// Calculate target buffer size (not including the zero terminator).
		int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			str.c_str(), str.size(), NULL, 0);
		if (len == 0)
			throw std::runtime_error("Invalid character sequence.");

		out.resize(len);
		// No error checking. We already know, that the conversion will succeed.
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			str.c_str(), str.size(), out.data(), out.size());
	}
#pragma warning ( pop )

	return out;
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
