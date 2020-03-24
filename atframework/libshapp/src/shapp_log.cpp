
#include "shapp_log.h"



namespace util{
    namespace log{
        log_adaptor::log_adaptor() : log_level_(LOG_MAX) {
		 }
        log_adaptor::~log_adaptor() {
		}

		log_type_t log_adaptor::get_log_level() {
			return log_level_;
		}
   


#ifdef G3LOG
        int  log_adaptor::init_log() { return 0; }
        void log_adaptor::on_log(log_type_t log_level, const char *file_path, size_t line, const char *function, const char *content, size_t content_size) {
            switch (log_level) {
            case LOG_DEBUG:
                if (check_level(log_level)) LogCapture(file_path, line, function, G3LOG_DEBUG).stream().write(content, content_size);
				break;
            case LOG_ERROR:
                if (check_level(log_level)) LogCapture(file_path, line, function, G3LOG_ERROR).stream().write(content, content_size);
                break;
            case LOG_FATAL: 
				if (check_level(log_level)) LogCapture(file_path, line, function, G3LOG_FATAL).stream().write(content, content_size);
				break;
            default:
                if (check_level(log_level)) LogCapture(file_path, line, function, G3LOG_DEBUG).stream().write(content, content_size);
                break;
            }
         
        }

        void log_adaptor::set_log_level(log_type level) { 
			if (level > LOG_TRACE && level != LOG_MAX) {
                level = LOG_TRACE;
            }
            log_level_ = level;
		}


#else
		namespace detail {
            static void _log_sink_stdout_handle(const util::log::log_wrapper::caller_info_t &, const char *content, size_t content_size) {
                std::cout.write(content, content_size);
                std::cout << std::endl;
            }
        } // namespace detail

        int log_adaptor::init_log() {
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->init();
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_prefix_format("[%L][%F %T.%f][%k:%n(%C)]: ");
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->add_sink(detail::_log_sink_stdout_handle);
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_stacktrace_level(log_formatter::level_t::LOG_LW_ERROR);
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_level(log_formatter::level_t::LOG_LW_DEBUG);
            return 0;
        }
        void log_adaptor::on_log(log_type_t log_level, const char *file_path, size_t line, const char *function, const char *content, size_t ) {
            util::log::log_wrapper::caller_info_t caller = util::log::log_wrapper::caller_info_t(static_cast<util::log::log_formatter::level_t::type>(log_level), NULL ,file_path, line, function);
            WDTLOGGETCAT(util::log::log_wrapper::categorize_t::DEFAULT)->log(caller, "%s", content);
        }

        void log_adaptor::set_log_level(log_type level) {
            if (level > LOG_TRACE) {
                level = LOG_TRACE;
            }
            WLOG_GETCAT(log_wrapper::categorize_t::DEFAULT)->set_level(static_cast<util::log::log_formatter::level_t::type>(level));
            log_level_ = level;
        }
       

#endif // G3LOG
       
	}


}

