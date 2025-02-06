/**
 * @brief 线程池的测试用例 [3]: 线程池的移动构造和移动赋值。 
*/

#include <cassert>

#include "./include/defs.hpp"
#include "./include/randomGenerator.hpp"
#include "../ThreadPool/ThreadPool.hpp"

void TestMoveConstruct(void);
void TestMoveAssignment(void);

int main(int argc, char const *argv[])
{
    if (system("clear")) {}
    
    TestMoveConstruct();
    TestMoveAssignment();

    print(
        SUCCESS_STYLE, "DONE\n"
    );

    return EXIT_SUCCESS;
}

void TestMoveConstruct(void)
{
    using namespace std::chrono;

    ThreadPool                  threadPool{4};
    RandomGenerator<int64_t>    randomGen{15, 45};

    std::vector<std::future<int64_t>> results;

    for (int index = 0; index < 4; ++index) 
    {
        int64_t randNum = randomGen.rand();

        results.emplace_back(
            threadPool.submit(
                [=](void){ 
                    std::this_thread::sleep_for(milliseconds(randNum)); 
                    return randNum; 
                }
            )
        );
    }

    ThreadPool newPool = std::move(threadPool);

#ifdef DEBUG
    assert(threadPool.isWorkersEmpty());
    assert(threadPool.isTasksEmpty());
    assert(threadPool.isStop());
    assert(threadPool.isThreadStopEmpty());
#endif

    for (int index = 0; index < 8; ++index) 
    {
        int64_t randNum = randomGen.rand();

        results.emplace_back(
            newPool.submit(
                [=](void){ 
                    std::this_thread::sleep_for(milliseconds(randNum)); 
                    return randNum; 
                }
            )
        );
    }

    try 
    {
        for (auto & future : results) 
        {
            print(
                NOTIFY_STYLE, 
                "Thread sleep {} ms.\n", future.get()
            );
        }
    }
    catch (std::exception & execpt) 
    {
        print(
            ERROR_STYLE, "{}\n\n", execpt.what()
        );
    }

    print(
        SUCCESS_STYLE, 
        "TestMoveConstruct() PASSED!\n\n"
    );
}

void TestMoveAssignment(void)
{
    using namespace std::chrono;

    ThreadPool                  threadPoolA{8};
    ThreadPool                  threadPoolB{6};
    RandomGenerator<int64_t>    randomGen{25, 75};

    std::vector<std::future<int64_t>> results;

    for (int index = 0; index < 8; ++index) 
    {
        results.emplace_back(
            threadPoolA.submit(
                [&](void)
                {
                    int64_t randNum = randomGen.rand();
                    std::this_thread::sleep_for(milliseconds(randNum));
                    return randNum; 
                }
            )
        );

        results.emplace_back(
            threadPoolB.submit(
                [&](void)
                {
                    int64_t randNum = randomGen.rand();
                    std::this_thread::sleep_for(milliseconds(randNum));
                    return randNum; 
                }
            )
        );
    }

    threadPoolA = std::move(threadPoolB);

#ifdef DEBUG
    assert(threadPoolB.isWorkersEmpty());
    assert(threadPoolB.isTasksEmpty());
    assert(threadPoolB.isStop());
    assert(threadPoolB.isThreadStopEmpty());
#endif

    for (int index = 0; index < 12; ++index) 
    {
        results.emplace_back(
            threadPoolA.submit(
                [&](void)
                {
                    int64_t randNum = randomGen.rand();
                    std::this_thread::sleep_for(milliseconds(randNum));
                    return randNum; 
                }
            )
        );
    }

    try 
    {
        for (auto & future : results) 
        {
            print(
                NOTIFY_STYLE, 
                "Thread sleep {} ms.\n", future.get()
            );
        }
    }
    catch (std::exception & execpt) 
    {
        print(
            ERROR_STYLE, "{}\n\n", execpt.what()
        );
    }

    print(
        SUCCESS_STYLE, 
        "TestMoveAssignment() PASSED!\n\n"
    );
}
