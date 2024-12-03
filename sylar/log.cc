#include "log.h"
#include <iostream>


//日志级别
//switch宏定义简化
const char* LogLevel::ToString(LogLevel::Level level){
    switch(level){
#define XX(name)\
    case LogLevel::name:\
	return #name;
    	break;

    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);

#undef XX
    default:
	return "UNKNOW";
    }
    return "UNKNOW";
}    

//日志事件实现
LogEvent::LogEvent(const std::string& logName, LogLevel::Level level, const char* file, int32_t line, 
		  uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    :m_logName(logName), m_level(level), m_file(file), m_line(line), m_elapse(elapse),
     m_threadId(thread_id), m_fiberId(fiber_id), m_time(time) {}

const char* LogEvent::getFile() const { return m_file; }
int32_t LogEvent::getLine() const { return m_line; }
uint32_t LogEvent::getElapse() const { return m_elapse; }
uint32_t LogEvent::getThreadId() const { return m_threadId; }
uint32_t LogEvent::getFiberId() const { return m_fiberId; }
uint64_t LogEvent::getTime() const { return m_time; }
LogLevel::Level LogEvent::getLevel() const { return m_level; }
const std::string& LogEvent::getLogName() const { return m_logName; }



//日志格式化输出的实现
LogFormatter::LogFormatter(const std::string& pattern)
	:m_pattern(pattern){
	//在初始化时就将pattern解析好
	init();
}


void LogFormatter::init(){
	//我们粗略的把上面的解析对象分成两类 一类是普通字符串 另一类是可被解析的
	//可以用 tuple来定义 需要的格式 std::tuple<std::string,std::string,int> 
	//<符号,子串,类型>  类型0-普通字符串 类型1-可被解析的字符串 
	//可以用一个 vector来存储 std::vector<std::tuple<std::string,std::string,int> > vec;
	std::vector<std::tuple<std::string,std::string,int> > vec;
	//解析后的字符串
	std::string nstr;
	//循环中解析
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        // 如果不是%号
        // nstr字符串后添加1个字符m_pattern[i]
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }
		// m_pattern[i]是% && m_pattern[i + 1] == '%' ==> 两个%,第二个%当作普通字符
        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }
		
		// m_pattern[i]是% && m_pattern[i + 1] != '%', 需要进行解析
        size_t n = i + 1;		// 跳过'%',从'%'的下一个字符开始解析
        int fmt_status = 0;		// 是否解析大括号内的内容: 已经遇到'{',但是还没有遇到'}' 值为1
        size_t fmt_begin = 0;	// 大括号开始的位置

        std::string str;
        std::string fmt;	// 存放'{}'中间截取的字符
        // 从m_pattern[i+1]开始遍历
        while(n < m_pattern.size()) {
        	// m_pattern[n]不是字母 & m_pattern[n]不是'{' & m_pattern[n]不是'}'
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{'
                    && m_pattern[n] != '}')) {
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                	// 遇到'{',将前面的字符截取
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; // 标志进入'{'
                    fmt_begin = n;	// 标志进入'{'的位置
                    ++n;
                    continue;
                }
            } else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                	// 遇到'}',将和'{'之间的字符截存入fmt
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    // 找完一组大括号就退出循环
                    break;
                }
            }
            ++n;
            // 判断是否遍历结束
            if(n == m_pattern.size()) {
                if(str.empty()) {
                    str = m_pattern.substr(i + 1);
                }
            }
        }

        if(fmt_status == 0) {
            if(!nstr.empty()) {
            	// 保存其他字符 '['  ']'  ':'
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            // fmt:寻找到的格式
            vec.push_back(std::make_tuple(str, fmt, 1));
            // 调整i的位置继续向后遍历
            i = n - 1;
        } else if(fmt_status == 1) {
        	// 没有找到与'{'相对应的'}' 所以解析报错，格式错误
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    //以下的编写方式绝对堪称经典！！！
	static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FilenameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
    }
}


std::string LogFormatter::format(LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items) {
        i->format(ss,event);
    }
    return ss.str();
}



// StdoutLogAppender 实现
void StdoutLogAppender::log(LogEvent::ptr event) {
    if(m_formatter){
       std::cout << m_formatter->format(event);
    }else{

        //格式化时间
        const std::string format = "%Y-%m-%d %H:%M:%S";
        struct tm tm;
        time_t t = event->getTime();
    	localtime_r(&t, &tm);
    	char tm_buf[64];
    	strftime(tm_buf, sizeof(tm_buf), format.c_str(), &tm);

    	std::cout << tm_buf << " "
              	  << event->getThreadId() << " "
                  << event->getFiberId() << " ["
                  << LogLevel::ToString(event->getLevel()) << "] "
                  << event->getFile() << ":" << event->getLine() << " "
                  << "输出到控制台的信息" << std::endl;
    }
}

// FileLogAppender 实现
FileLogAppender::FileLogAppender(const std::string& filename) 
    : m_filename(filename) {}

void FileLogAppender::log(LogEvent::ptr event) {
    std::cout << "输出到文件 " << m_filename << std::endl;
}

// Logger 实现
Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::DEBUG) {}

const std::string& Logger::getName() const { return m_name; }
LogLevel::Level Logger::getLevel() const { return m_level; }
void Logger::setLevel(LogLevel::Level val) { m_level = val; }

void Logger::addAppender(LogAppender::ptr appender) {
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
        if(*it == appender) {
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::log(LogEvent::ptr event) {
    if(event->getLevel() >= m_level) {
        for(auto& i : m_appenders) {
            i->log(event);
        }
    }
}

