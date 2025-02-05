/**
 * @brief 线程池的测试用例 [1]: 使用线程池进行并行求和。 
*/

#include <numeric>
#include <csignal>

#include "./include/defs.hpp"
#include "./include/funcExecuteTimeCompute.hpp"
#include "./include/randomGenerator.hpp"
#include "../ThreadPool/ThreadPool.hpp"

#undef __argc
#undef __argv

volatile sig_atomic_t DONE = false;

// 线程池实例化在全局，默认只放飞本 CPU 最大能支持的物理线程数。
ThreadPool THREADPOOL{std::thread::hardware_concurrency()};

/**
 * @brief 强制结束程序时要做的任务。 
*/
void exitHandle(int __signal)
{
    print(WARNING_STYLE, "{} Exit Forced, Thread Pool Stop.\n", CurrentTime());

    DONE = true;

    print(ERROR_STYLE, "[DONE]\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief 主函数参数检查
*/
int argumentCheck(const int __argc, char const * __argv[]) 
{
    if (__argc != 2) 
    {
        print(ERROR_STYLE, "Test_ParallelAccumulate <test-usage-amount>\n\n");
        print(ERROR_STYLE, "[DONE]\n");
        exit(EXIT_FAILURE);
    }

    try {
        return std::stoi(__argv[1]);
    }
    catch (const std::exception & except) 
    {
        print("{}\n", except.what());
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief 并行的计算一个容器的全体元素之和。
*/
template <typename Iterator, typename Type>
Type parallelAccumulate(Iterator first, Iterator last, Type init)
{
    std::size_t length = std::distance(first, last);    // 计算容器的元素数

    if (!length) { return init; }   // 对于空容器，直接返回初始值就行

    if (length <= 10000) { return std::accumulate<Iterator, Type>(first, last, init); }

    // 动态的计算块大小
    const std::size_t blockSize 
        = std::max<size_t>(4500, length / std::thread::hardware_concurrency());

    // 计算块的数量
    const std::size_t blockAmount 
        = (length + blockSize - 1) / blockSize;

    // 为每一个块分配一个期值，存储每一个块的计算结果
    std::vector<std::future<Type>> futures(blockAmount - 1);

    Iterator blockStart = first;

    for (std::size_t index = 0; index < (blockAmount - 1); ++index)
    {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);  // 标记当前块的起点和终点

        try 
        {
            // 提交任务
            futures[index] = THREADPOOL.submit(
                std::accumulate<Iterator, Type>, 
                blockStart, blockEnd, Type{}
            );
        }
        catch (const std::exception & except) 
        {
            print(
                ERROR_STYLE, 
                "{} {}", CurrentTime(), except.what()
            );
        }

        blockStart = blockEnd;  // 设置下一个块的起点
    }
    
    // 主线程参与计算最后一个块的和
    Type lastRes  = std::accumulate(blockStart, last, Type{});
    Type finalRes = init;
    
    // 汇总线程们的计算结果
    for (std::size_t index = 0; index < (blockAmount - 1); ++index) {
        finalRes += futures[index].get();
    }

    finalRes += lastRes;

    return finalRes;
}

int main(int argc, char const *argv[])
{
    system("cls");

    const int LoopAmount = argumentCheck(argc, argv);

    std::signal(SIGINT, exitHandle);

    std::vector<int64_t> numberList(150000000);

    RandomGenerator<int64_t> randomGen{-1919810, 1919810};

    int64_t concurrencyRes{};
    int64_t stlRes{};

    std::vector<uint64_t> concurrencyTime(LoopAmount);
    std::vector<uint64_t> stlTime(LoopAmount);

    auto fillData = [&](void) 
    {
        for (std::size_t index = 0; index < numberList.size(); ++index) {
            numberList[index] = randomGen.rand();
        }
    };

    auto accumulateConcurrency = [&]() 
    {
        concurrencyRes = parallelAccumulate(
            numberList.begin(), numberList.end(), int64_t{}
        );
    };

    auto stlAccumulate = [&]() 
    {
        stlRes = std::accumulate(
            numberList.begin(), numberList.end(), int64_t{}
        );
    };

    for (int times = 0; times < LoopAmount; ++times)
    {
        print(
            NOTIFY_STYLE, "{} Test No. [{}], Fill {} data, cost: [{}] ms.\n\n", 
            CurrentTime(), times + 1, numberList.size(), 
            functionExecuteTime(fillData).count()
        );

        concurrencyTime[times] = functionExecuteTime(accumulateConcurrency).count();
        stlTime[times]         = functionExecuteTime(stlAccumulate).count();
        
        print(
            SUCCESS_STYLE, 
            "parallelAccumulate() result = {} cost [{}] ms.\n"
            "std::accumulate()    result = {} cost [{}] ms.\n",
            concurrencyRes, concurrencyTime[times],
            stlRes, stlTime[times]
        );

        (concurrencyRes == stlRes) 
        ?   print(SUCCESS_STYLE, "Compare result: [TRUE]\n\n")
        :   print(
            ERROR_STYLE,   
            "Compare result: [FALSE]"
            "concurrencyRes = {}, stlRes = {}\n\n", 
            concurrencyRes, stlRes
        );

        if (DONE) { break; }
    }

    print(
        SUCCESS_STYLE, 
        "parallelAccumulate() took an average of [{}] ms\n"
        "std::accumulate()    took an average of [{}] ms.\n\n",
        parallelAccumulate(concurrencyTime.begin(), concurrencyTime.end(), uint64_t{}) / concurrencyTime.size(),
        parallelAccumulate(stlTime.begin(), stlTime.end(), uint64_t{}) / stlTime.size()
    );

    print(SUCCESS_STYLE, "{} [DONE]\n", CurrentTime());

    return EXIT_SUCCESS;
}
