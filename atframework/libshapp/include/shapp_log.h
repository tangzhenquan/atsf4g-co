

#ifndef ATFRAME_LIBSHAPP_SHAPP_LOG_H
#define ATFRAME_LIBSHAPP_SHAPP_LOG_H
#pragma once

#include <design_pattern/singleton.h>


#define G3LOG


#ifdef G3LOG
#include <g3log/g3log.hpp>
#endif
namespace util {
    namespace log {
        enum log_type {
            LOG_DISABLE = 0,
            LOG_FATAL,
            LOG_ERROR,
            LOG_WARNING,
            LOG_INFO,
            LOG_NOTICE,
            LOG_DEBUG,
            LOG_TRACE,


			LOG_MAX = 10000,
        };
        typedef log_type log_type_t;

        class log_adaptor : public util::design_pattern::singleton<log_adaptor> {
        public:
            log_adaptor();
            ~log_adaptor();

        public:
            int        init_log();
            void       on_log(log_type_t log_level, const char *file_path, size_t line, const char *function, const char *content, size_t content_size);
            void       set_log_level(log_type_t level);
            log_type_t get_log_level();
            inline bool check_level(log_type_t level) const { 
				return log_level_ == LOG_MAX ? true : log_level_ >= level;
			}
          

#ifdef G3LOG
            inline bool check_level(log_type_t level, LEVELS g3_level) const {
				return log_level_ == LOG_MAX ? g3::logLevel(g3_level) : log_level_ >= level;
			}

#endif // G3LOG


        private:
            log_type_t log_level_;
        };

    } // namespace log
} // namespace util


#ifdef G3LOG
#define WLOGF(level, g3_level, printf_like_message, ...) \
    if (util::log::log_adaptor::get_instance().check_level(level, g3_level)) INTERNAL_LOG_MESSAGE(g3_level).capturef(printf_like_message, ##__VA_ARGS__)

#define WLOGTRACE(...) WLOGF((util::log::LOG_TRACE, G3LOG_INFO, __VA_ARGS__);
#define WLOGDEBUG(...) WLOGF(util::log::LOG_DEBUG, G3LOG_DEBUG, __VA_ARGS__);
#define WLOGNOTICE(...) WLOGF(util::log::LOG_NOTICE, G3LOG_INFO, __VA_ARGS__);
#define WLOGINFO(...) WLOGF(util::log::LOG_INFO, G3LOG_INFO, __VA_ARGS__);
#define WLOGWARNING(...) WLOGF(util::log::LOG_WARNING, G3LOG_WARNING, __VA_ARGS__);
#define WLOGERROR(...) WLOGF(util::log::LOG_ERROR, G3LOG_ERROR, __VA_ARGS__);
#define WLOGFATAL(...) WLOGF(util::log::LOG_FATAL, G3LOG_FATAL, __VA_ARGS__);

#else
#include <log/log_wrapper.h>

#endif // G3LOG


#endif // ATFRAME_LIBSHAPP_SHAPP_LOG_H
