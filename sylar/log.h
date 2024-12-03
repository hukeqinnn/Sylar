#ifndef __SYLAR_LOG_H_
#define __SYLAR_LOG_H_

#include <string>
#include <stdint.h>
#include <memory>
#include <iostream>
#include <list>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>
#include <tuple>
#include <map>
#include <sstream>
#include <functional>


//日志级别
class LogLevel{
public:
    enum Level{
        UNKNOW = 0,//未知
        DEBUG = 1,//调试
        INFO = 2,//普通
        WARN = 3,//警告
        ERROR = 4,//错误
        FATAL = 5//灾难
    };
    static const char* ToString(LogLevel::Level level);
};

//日志事件
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;
    LogEvent(const std::string& logName, LogLevel::Level level
           , const char* file, int32_t line, uint32_t elapse
           , uint32_t thread_id, uint32_t fiber_id, uint64_t time);

    const std::string& getLogName() const;
    const char* getFile() const;
    int32_t getLine() const;
    uint32_t getElapse() const;
    uint32_t getThreadId() const;
    uint32_t getFiberId() const;
    uint64_t getTime() const;
    LogLevel::Level getLevel() const;

private:
    std::string m_logName;
    const char* m_file = nullptr;
    int32_t m_line = 0;
    uint32_t m_elapse = 0;
    uint32_t m_threadId = 0;
    uint32_t m_fiberId = 0;
    uint64_t m_time;
    LogLevel::Level m_level;
};



// 日志格式化类
class LogFormatter {
public:
    class FormatItem {
    public:
        typedef std::shared_ptr<FormatItem> ptr;
        virtual ~FormatItem() {}
        virtual void format(std::ostream& os, LogEvent::ptr event) = 0;
    };

    LogFormatter(const std::string& pattern);  // 声明构造函数
    typedef std::shared_ptr<LogFormatter> ptr;

    void init();
    std::string format(LogEvent::ptr event);

private:
    std::string m_pattern;
    std::vector<FormatItem::ptr> m_items;
};

// 各种 FormatItem 子类的声明
class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << "Message";
    }
};
                        
class LevelFormatItem : public LogFormatter::FormatItem {
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << LogLevel::ToString(event->getLevel());
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem {
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override{
	os << event->getLogName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFiberId();
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
public:
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
        :m_format(format) {
        if(m_format.empty()) {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }

    void format(std::ostream& os, LogEvent::ptr event) override {
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
    }
private:
    std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
public:
    FilenameFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem {
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << event->getLine();
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << std::endl;
    }
};

class StringFormatItem : public LogFormatter::FormatItem {
public:
    StringFormatItem(const std::string& str)
        :m_string(str) {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::ostream& os, LogEvent::ptr event) override {
        os << "\t";
    }
private:
    std::string m_string;
};



//日志输出器基类
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;
    virtual ~LogAppender() {}
    virtual void log(LogEvent::ptr event) = 0;
    void setFormatter(std::shared_ptr<class LogFormatter> formatter) { m_formatter = formatter; }
protected:
    std::shared_ptr<class LogFormatter> m_formatter;
};



//控制台日志输出器
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;
    void log(LogEvent::ptr event) override;
};

//文件日志输出器
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;
    FileLogAppender(const std::string& filename);
    void log(LogEvent::ptr event) override;
private:
    std::string m_filename;
};




//日志器
class Logger{
public:
    typedef std::shared_ptr<Logger> ptr;
    Logger(const std::string& name = "root");

    const std::string& getName() const;
    LogLevel::Level getLevel() const;
    void setLevel(LogLevel::Level val);
    void log(LogEvent::ptr event);
    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);

private:
    std::string m_name;
    LogLevel::Level m_level;
    std::list<LogAppender::ptr> m_appenders;
};

#endif


