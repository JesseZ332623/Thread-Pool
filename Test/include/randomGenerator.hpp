#ifndef __RANDOM_GERNERATOR_H__
#define __RANDOM_GERNERATOR_H__

#include <random>
#include <mutex>
#include <type_traits>
#include <memory>

#include <sstream>

/**
 * @brief 随机数生成器（使用互斥锁确保线程安全）。
 * 
 * @tparam RandomType 随机数类型。
*/
template <typename RandomType>
class RandomGenerator
{
    public:
        /**
         * @brief 静态断言，这个类不允许使用引用类型进行具体化，
         *        否则就是编译错误。
        */ 
        static_assert(
            !std::is_reference_v<RandomType>,
            "[ERROR] Type of RandomGenerator<T> must have non-reference type."
        );

        /**
         * @brief 由于要支持整数和小数的随机生成，
         *        可以使用 `std::conditional<>` 根据 RandomType
         *        的具体类型来判断使用 `std::uniform_int_distribution<RandomType>`
         *        或 `std::uniform_real_distribution<RandomType>`。
         */
        using DistributionType = typename std::conditional<
                                    std::is_integral<RandomType>::value,
                                    std::uniform_int_distribution<RandomType>,
                                    std::uniform_real_distribution<RandomType>
                                >::type;

    private:
        static std::random_device   deveice;
        std::mt19937_64             engine;     // 随机数引擎
        DistributionType            dist;       // 随机数分布器
        mutable std::mutex          mutex;      // 互斥锁

        /**
         * @brief 边界检查，违反会抛 `std::invalid_argument` 异常。
        */
        void boundCheck(const RandomType & __left, const RandomType & __right) const;

    public:
        /**
         * @brief 构造函数，
         *        默认生成范围是 `[0, 0]`，需要调用 `resetRange()` 方法设置，
         *        如果没有的话，在调用 rand() 方法时会抛出 `std::runtime_error` 异常。
        */
        RandomGenerator(void);

        /**
         * @brief 构造函数，
         *        划定生成范围 `[__left, __right]`。
        */
        RandomGenerator(RandomType __left, RandomType __right);

        /**
         * @brief 拷贝构造函数（禁用）。
        */
        RandomGenerator(const RandomGenerator &) = delete;

        /**
         * @brief 移动构造函数（禁用）。
        */
        RandomGenerator(RandomGenerator && __other) = delete;

        /**
         * @brief 拷贝构造运算符（禁用）。
        */
        RandomGenerator & operator=(const RandomGenerator &) = delete;

        /**
         * @brief 移动构造运算符（禁用）。
        */
        RandomGenerator & operator=(RandomGenerator && __other) = delete;
        
        /**
         * @brief 重设随机数生成范围 `[__left, __right]`。
        */
        void resetRange(RandomType __left, RandomType __right);

        /**
         * @brief 重设随机数种子（使用默认构造的 `std::random_device`）。
        */
        void resetSeed(void) noexcept;

        /**
         * @brief 重设随机数种子（使用外部传入的种子）。
        */
        void resetSeed(uint64_t __seed) noexcept;

        /**
         * @brief 生成随机数。
        */
        RandomType rand(void);

        /**
         * @brief 工厂函数，返回一个新的 `std::unique_ptr<RandomGenerator<RandomType>>` 类对象。
         * 
         * @brief - 用法示例：
         *         `std::unique_ptr<RandomGenerator<int>> randomGenPtr = RandomGenerator<int>::create();`
        */
        static std::unique_ptr<RandomGenerator<RandomType>>
        create(RandomType __left, RandomType __right) { 
            return std::make_unique<RandomGenerator<RandomType>>(__left, __right); 
        }
};

template <typename RandomType>
std::random_device RandomGenerator<RandomType>::deveice{};

template <typename RandomType>
inline void RandomGenerator<RandomType>::
boundCheck(const RandomType & __left, const RandomType & __right) const
{
    if (__left > __right) 
    {
        std::ostringstream exceptionInfo;

        exceptionInfo << std::fixed << "[INVALID-ARGUMENT] __left: [" << __left  
                      << "] > " << "__right: [" << __right << "]\n";

        throw std::invalid_argument{exceptionInfo.str()};
    }
}

template <typename RandomType>
RandomGenerator<RandomType>::RandomGenerator(void) 
    : engine{this->deveice()}, dist{RandomType{0}, RandomType{0}} {}

template <typename RandomType>
RandomGenerator<RandomType>::RandomGenerator(RandomType __left, RandomType __right) 
    : engine{this->deveice()}, dist{__left, __right}
{
    this->boundCheck(__left, __right);
}

template <typename RandomType>
inline void RandomGenerator<RandomType>::resetRange(RandomType __left, RandomType __right)
{
    this->boundCheck(__left, __right);

    std::scoped_lock<std::mutex> lock{this->mutex};

    this->dist.param(DistributionType::param_type(__left, __right));
}

template <typename RandomType>
inline void RandomGenerator<RandomType>::resetSeed(void) noexcept
{
    std::scoped_lock<std::mutex> lock{this->mutex};
    this->engine.seed(this->deveice());
}

template <typename RandomType>
inline void RandomGenerator<RandomType>::resetSeed(uint64_t __seed) noexcept
{
    std::scoped_lock<std::mutex> lock{this->mutex};
    this->engine.seed(__seed);
}

template <typename RandomType>
inline RandomType RandomGenerator<RandomType>::rand(void)
{
    std::scoped_lock<std::mutex> lock{this->mutex};

    return this->dist(this->engine);
}

#endif // __RANDOM_GERNERATOR_H__