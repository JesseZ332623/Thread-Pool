#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <atomic>

class ThreadPool 
{
    public:
        /**
         * @brief 任务的类型，是一个无返回值和形参的可调用对象，
         *        线程池的 submit() 方法会包装外部传入的可调用对象。 
        */
        typedef std::function<void()> Task;

    private:
        std::vector<std::thread>    workers;            // 线程池
        std::queue<Task>            tasks;              // 任务队列
        
        std::mutex                  queue_mutex;             
        std::condition_variable     condition;

        std::atomic_bool            stop;               // 是否停止执行任务的指示

        /**
         * @brief 在线程池构造的时候，
         *        放飞的每一个线程要做的任务。
        */
        void launchThread(void);

    public:

        /**
         * @brief 构造函数，创建 n 个线程并放飞。
        */
        ThreadPool(std::size_t);

        ThreadPool(const ThreadPool &)              = delete;
        ThreadPool & operator=(const ThreadPool &)  = delete;
        ThreadPool & operator=(ThreadPool &)        = delete;

        ThreadPool(ThreadPool &&) noexcept;
        ThreadPool & operator=(ThreadPool &&) noexcept;

        /**
         * @brief           向线程池提交任务。
         * 
         * @tparam F        要提交的任务类型（可调用对象）
         * @tparam Args     要传递给可调用对象的参数类型们
         * 
         * @param f         要提交的任务
         * @param args      要传递给可调用对象的参数们
         * 
         * @return          一个期值，任务的运行结果或异常保存于此，
         *                  允许调用者等待任务的结束。
        */
        template<class F, class... Args>
        std::future<typename std::result_of<F(Args...)>::type>
        submit(F && f, Args && ...args);


        /**
         * @brief 析构函数，发出停止执行任务的指示，
         *        等待池内所有线程回归，再销毁池本身。
        */
        ~ThreadPool();
};

void ThreadPool::launchThread(void)
{
    while (true)
    {
        Task task;

        {
            std::unique_lock<std::mutex> lock{this->queue_mutex};

            /**
             * 在任务队列中没有任务，或者没有收到停止处理任务的指示之前
             * 线程们会在此处阻塞。
            */
            this->condition.wait(
                lock, [this](void) { return (this->stop || !this->tasks.empty()); }
            );

            // 在收到停止处理任务的指示并且任务队列中没有任务时，线程返回。
            if (this->stop && this->tasks.empty()) { return; }

            // 任务出队
            task = std::move(this->tasks.front());
            this->tasks.pop();
        }

        task.operator()();  // 执行任务
    }
}

inline ThreadPool::ThreadPool(std::size_t threads) : stop{false}
{
    // 放飞线程们
    for(size_t i = 0;i < threads; ++i) {
        workers.emplace_back(ThreadPool::launchThread, this);
    }
}

inline ThreadPool::ThreadPool(ThreadPool && other) noexcept
: workers(std::move(other.workers)), tasks(std::move(other.tasks)), stop(other.stop.load())
{
    other.stop.store(true);
}

ThreadPool & ThreadPool::operator=(ThreadPool && other) noexcept
{
    if (this != &other) 
    {
        {
            std::unique_lock<std::mutex> lock{this->queue_mutex};
            
            this->stop.store(true);
            this->condition.notify_all();
        }

        for (std::thread & worker : this->workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        this->workers.clear();

        {
            std::unique_lock<std::mutex> lock{this->queue_mutex};

            while (!this->tasks.empty()) { this->tasks.pop(); }
        }

        {
            std::unique_lock<std::mutex> lock{this->queue_mutex, std::defer_lock};
            std::unique_lock<std::mutex> otherLock{other.queue_mutex, std::defer_lock};

            std::lock(lock, otherLock);

            this->workers = std::move(other.workers);
            this->tasks   = std::move(other.tasks);
        }

        this->stop.store(other.stop.load());
        other.stop.store(true);

        this->condition.notify_all();
        other.condition.notify_all();
    }

    return *this;
}

template<class F, class... Args>
std::future<typename std::result_of<F(Args...)>::type>
ThreadPool::submit(F && f, Args &&... args)
{
    /**
     * std::result_of<> 萃取可调用对象 F 在 Args... 参数下的返回值类型，
     * 它在 C++ 17 被 std::invoke_result_t<> 代替，在 C++ 20 被移除。
    */
    using return_type = typename std::result_of<F(Args...)>::type;

    /**
     * 使用 std::bind() 绑定可调用对象的参数，
     * 包装成一个 std::packaged_task<return_type()> 类型，
     * 交由 std::shared_ptr<> 去管理。
     * 
     * Tips: 在包装过程中，务必使用 std::forward<> 保持参数的值性。
    */
    auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    // 获取这个任务的期值
    std::future<return_type> res = task->get_future();

    {
        std::unique_lock<std::mutex> lock{this->queue_mutex};

        /**
         * 若本线程池已经停止执行任务了（通常发送在析构函数，正在等待池中所有线程回归时），
         * 继续往池中提交任务，则抛出运行时异常。
        */
        if(stop) 
        {    
            throw std::runtime_error(
                "[RUNTIME-ERROR] Submit on stopped ThreadPool\n"
            );
        }

        // 任务入队
        tasks.emplace([task](){ (*task)(); });
    }

    // 通知线程等待队列中的第一个线程起来干活
    condition.notify_one();

    return res;
}

inline ThreadPool::~ThreadPool()
{   
    // 通知池内所有等待的线程放弃执行任务。
    this->stop.store(true);
    this->condition.notify_all();

    // 等待池内所有线程回归。
    for(std::thread & worker: workers) 
    { 
        if (worker.joinable()) {
            worker.join(); 
        }
    }
}

#endif // THREAD_POOL_H
