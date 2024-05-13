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
	// ������ ������� �������� �����������
	if (lhs.empty() && rhs.empty())
		return true;
	// ������� ������ ����� �� ���������
	if (lhs.type() != rhs.type())
		return false;
	// ������� ������ ������������ �� ���������
	if (lhs.cols != rhs.cols || lhs.rows != rhs.rows || lhs.dims != rhs.dims)
		return false;
	// �������, ����������� �� ������ ������ �� ���������
	if (lhs.data != rhs.data)
		return false;
	// ������������ ��������� (��������� - ������ �������), ����� ���������� ����� � � ��������� � 0
	return cv::sum(lhs != rhs) == cv::Scalar(0, 0, 0, 0);
}
