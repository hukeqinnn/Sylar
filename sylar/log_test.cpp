#include "log.h"  // 确保包含了 log.h
#include <sys/syscall.h>
#include <unistd.h>
#include <ctime>

int main(int argc, char** argv) {
    Logger::ptr lg(new Logger("XYZ"));
    
    LogEvent::ptr event(new LogEvent(
        lg->getName(),              // 日志器名称
        LogLevel::INFO,             // 日志级别
        __FILE__,                   // 文件名称
        __LINE__,                   // 行号
        1234567,                    // 运行时间（需要加上这个参数）
        syscall(SYS_gettid),        // 线程ID
        0,                          // 协程ID
        time(0)                     // 时间戳
    ));

    LogFormatter::ptr formatter(new LogFormatter(
        "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    ));

    // 添加控制台输出适配器
    StdoutLogAppender::ptr stdApd(new StdoutLogAppender());
    stdApd->setFormatter(formatter);
    lg->addAppender(stdApd);

    lg->log(event);  // 调用日志器记录日志

    return 0;
}

