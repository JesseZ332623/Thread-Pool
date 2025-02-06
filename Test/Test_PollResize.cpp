/**
 * @brief 线程池的测试用例 [2]: 测试线程池的扩容和收缩。 
*/

#include "./include/defs.hpp"
#include "./include/funcExecuteTimeCompute.hpp"
#include "./include/randomGenerator.hpp"
#include "../ThreadPool/ThreadPool.hpp"

// 线程池实例化在全局。
ThreadPool THREADPOOL{7};

int threadTask(std::vector<int> & __numList, RandomGenerator<size_t> & __randGen, size_t __amount);
void ThreadPoolResetTest(size_t __testTimes);


int main(int argc, char const *argv[])
{
    ThreadPoolResetTest(10000);

    putchar('\n');
    
    return EXIT_SUCCESS;
}

int threadTask(std::vector<int> & __numList, RandomGenerator<size_t> & __randGen, size_t __amount)
{
    for (size_t index = 0; index < __amount; ++index) {
        __numList[index] = __randGen.rand();
    }

    return std::accumulate(__numList.begin(), __numList.end(), int{});
}

void ThreadPoolResetTest(size_t __testTimes)
{
    RandomGenerator<size_t>         randomGen{1, std::thread::hardware_concurrency()};
    std::vector<int>                numberList(1000);
    std::vector<std::future<int>>   results;

    int success = 0, tasks = 0;

    for (size_t index = 0; index < __testTimes; ++index) 
    {
        size_t poolSize = randomGen.rand();

        print(
            NOTIFY_STYLE, "ThreadPoolResetTest No.[{}] resize({})\n",
            index + 1, poolSize
        );

        if (poolSize == std::thread::hardware_concurrency() / 2)
        {
            results.emplace_back(
                THREADPOOL.submit(threadTask, 
                std::ref(numberList), std::ref(randomGen), numberList.size())
            );
            ++tasks;
        }

        THREADPOOL.resize(poolSize);
    }

    for (std::future<int> & f : results) 
    {
        int res = f.get();
        print("{} ", res);
        ++success;
    }

    fmt::text_style resultStyle = (tasks == success) ? SUCCESS_STYLE : ERROR_STYLE;

    print(
        resultStyle, "\n\nSubmit tasks: {}, Success: {}, Failed: {}\n", 
        tasks, success, tasks - success
    );
}