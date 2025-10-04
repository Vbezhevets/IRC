#ifndef UTILS_HPP
# define UTILS_HPP

#include <iostream>
#include <vector>

#define LOG_INFO std::cerr << "[INFO ] "
#define LOG_DEBUG std::cerr << "[DEBUG] "
#define LOG_ERR std::cerr << "[ERROR] "

std::vector<std::string> split(const std::string& str, char delimiter);

#endif // UTILS_HPP
