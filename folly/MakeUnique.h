/*************************************************************************
    > File Name: MakeUnique.h
    > Author: ma6174
    > Mail: ma6174@163.com
    > Created Time: Sun Jan 21 19:10:17 2018
 ************************************************************************/

#pragma once
#include<memory>
namespace std {
template<typename T>
std::unique_ptr<T> make_unique(const std::size_t size) {
    return std::unique_ptr<T>(new typename std::remove_extent<T>::type[size]());
}
template <class T, class ... Args>
std::unique_ptr<T> make_unique(Args&& ... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

}

