#pragma once

#include <string>
#include <opencv2/core.hpp>

class Utility
{
public:
	static const std::string getWorkingDirectory();
	static const std::wstring to_wstring(const std::string& str);
	// Может быть не нужна эта функция
	static bool isMatEqual(cv::Mat& lhs, cv::Mat& rhs);

	template <typename M, typename V>
	inline static void mapToVec(const  M& map, V& vec);
};

template<typename M, typename V>
inline void Utility::mapToVec(const M& map, V& vec)
{
	for (typename M::const_iterator it = map.begin(); it != map.end(); ++it)
		vec.push_back(it->second);
}
