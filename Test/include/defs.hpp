#ifndef __DEFS_H__
#define __DEFS_H__

#include <fmt/chrono.h>
#include <fmt/color.h>

using namespace fmt;

#define NOTIFY_STYLE  (fg(terminal_color::blue)   | emphasis::italic)
#define SUCCESS_STYLE (fg(terminal_color::green))
#define WARNING_STYLE (fg(terminal_color::yellow) | emphasis::italic)
#define ERROR_STYLE   (fg(terminal_color::red)    | emphasis::bold)

/**
 * @brief 返回当前时间字符串（例：[2024-11-24 16:11:35.279]）
*/
static std::string CurrentTime(void);

static std::string CurrentTime(void)
{
    using namespace std::chrono;

    // 获取当前时间（东八区）
    auto now                    = system_clock::now() + hours(8);
    // 转换成时间戳
    auto durationEpoch          = now.time_since_epoch();
    // 提取毫秒数
    auto millisecondsSinceEpoch = duration_cast<milliseconds>(durationEpoch);

    /**
     * @brief 返回格式化后的字符串（年-月-日 时:分:秒.毫秒） 
    */
    return format(
        "[{:%Y-%m-%d} {}]", 
        now, fmt::format("{:%T}", millisecondsSinceEpoch)
    );
}

#endif // __DEFS_H__