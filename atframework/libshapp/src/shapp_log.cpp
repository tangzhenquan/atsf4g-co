
#include "shapp_log.h"


static void _log_sink_stdout_handle(const util::log::log_wrapper::caller_info_t &, const char *content, size_t content_size) {
    std::cout.write(content, content_size);
    std::cout << std::endl;
}

namespace util{
    namespace log{
        int init_log(){
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->init();
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_prefix_format("[%L][%F %T.%f][%k:%n(%C)]: ");
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->add_sink(_log_sink_stdout_handle);
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_stacktrace_level(log_formatter::level_t::LOG_LW_ERROR);
            return 0;
        }
    }

}

