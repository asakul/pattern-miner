#ifndef LOG_H_
#define LOG_H_

#include "3rdparty/easylogging/easylogging++.h"
#include <iomanip>

#define STR(x) #x
#define PRINT(expr) STR(expr) << ": " << (expr)

void initLogging(const std::string& filename, bool debug);

#endif /* LOG_H_ */
