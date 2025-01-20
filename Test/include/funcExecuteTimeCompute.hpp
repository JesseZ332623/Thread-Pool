#ifndef __FUNCTION_EXECUTETIME_COMPUTE__H__
#define __FUNCTION_EXECUTETIME_COMPUTE__H__

#include <chrono>

template <typename Function>
std::chrono::milliseconds functionExecuteTime(Function __func);

template <typename Function>
std::chrono::milliseconds functionExecuteTime(Function __func)
{
    using namespace std::chrono;

    time_point beginTime = system_clock::now();

    __func();

    return duration_cast<milliseconds>(system_clock::now() - beginTime);
}

#endif // __FUNCTION_EXECUTETIME_COMPUTE__H__