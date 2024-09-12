#pragma once

#include <iostream>
#include <cstring>

#define GET_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG(message) std::cout << "[" << GET_FILENAME << ":" << __LINE__ << "] " << message << std::endl