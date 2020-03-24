//
// Created by tom on 2020/1/9.
//
#include "shapp.h"
#include "shapp_log.h"
#include "std/foreach.h"
#include <cli/shell_font.h>
#include <common/file_system.h>
#include <common/string_oprs.h>
#include <libatbus_protocol.h>
#include <stdarg.h>

namespace shapp {


    app::flag_guard_t::flag_guard_t(app &owner, flag_t::type f) : owner_(&owner), flag_(f) {
        if (owner_->check_flag(flag_)) {
            owner_ = NULL;
            return;
        }

        owner_->set_flag(flag_, true);
    }

    app::flag_guard_t::~flag_guard_t() {
        if (NULL == owner_) {
            return;
        }

        owner_->set_flag(flag_, false);
    }

    static std::pair<uint64_t, const char *> make_size_showup(uint64_t sz) {
        const char *unit = "KB";
        if (sz > 102400) {
            sz /= 1024;
            unit = "MB";
        }

        if (sz > 102400) {
            sz /= 1024;
            unit = "GB";
        }

        if (sz > 102400) {
            sz /= 1024;
            unit = "TB";
        }

        return std::pair<uint64_t, const char *>(sz, unit);
    }

    void app::ev_stop_timeout(uv_timer_t *handle) {
        assert(handle && handle->data);

        if (NULL != handle && NULL != handle->data) {
            app *self = reinterpret_cast<app *>(handle->data);
            self->set_flag(flag_t::TIMEOUT, true);
        }

        if (NULL != handle) {
            uv_stop(handle->loop);
        }
    }


    app::app() : setup_result_(0), last_proc_event_count_(0) {
        conf_.id            = 0;
        conf_.stop_timeout  = 30000; // 30s
        conf_.tick_interval = 32;    // 32ms

        tick_timer_.sec_update = util::time::time_utility::raw_time_t::min();
        tick_timer_.sec        = 0;
        tick_timer_.usec       = 0;

        tick_timer_.tick_timer.is_activited    = false;
        tick_timer_.timeout_timer.is_activited = false;

        stat_.last_checkpoint_min = 0;
    }

    app::~app() {
        owent_foreach(module_ptr_t & mod, modules_) {
            if (mod && mod->owner_ == this) {
                mod->owner_ = NULL;
            }
        }

        // reset atbus first, make sure atbus ref count is greater than 0 when reset it
        // some inner async deallocate action will add ref count and we should make sure
        // atbus is not destroying
        if (bus_node_) {
            bus_node_->reset();
            bus_node_.reset();
        }

        assert(!tick_timer_.tick_timer.is_activited);
        assert(!tick_timer_.timeout_timer.is_activited);
    }
    int app::run(uv_loop_t *ev_loop, const app_conf &conf) {
        if (0 != setup_result_) {
            return setup_result_;
        }

        if (check_flag(flag_t::IN_CALLBACK)) {
            return 0;
        }

        if (is_closed()) {
            return EN_SHAPP_ERR_ALREADY_CLOSED;
        }

        if (false == check_flag(flag_t::INITIALIZED)) {
            int res = init(ev_loop, conf);
            if (res < 0) {
                return res;
            }
        }

        int ret = 0;
        while (!is_closed()) {
            ret = run_inner(UV_RUN_DEFAULT);
        }
        return ret;
    }

    int app::run() {
        if (check_flag(flag_t::IN_CALLBACK)) {
            return 0;
        }
        if (false == check_flag(flag_t::INITIALIZED)) {
            return EN_SHAPP_ERR_NOT_INITED;
        }
        if (is_closed()) {
            return EN_SHAPP_ERR_ALREADY_CLOSED;
        }

        int ret = 0;
        while (!is_closed()) {
            ret = run_inner(UV_RUN_DEFAULT);
        }
        return ret;
    }

    int app::init(uv_loop_t *ev_loop, const app_conf &conf) {
        if (check_flag(flag_t::INITIALIZED)) {
            return EN_SHAPP_ERR_ALREADY_INITED;
        }
        setup_result_ = 0;

        if (check_flag(flag_t::IN_CALLBACK)) {
            return 0;
        }

        // update time first
        util::time::time_utility::update();

        util::cli::shell_stream ss(std::cerr);
        // step 4. load options from cmd line

        int ret = set_conf(conf);
        if (ret < 0) {
            ss() << util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "check config fail" << std::endl;
            return setup_result_ = ret;
        }

        conf_.bus_conf.ev_loop = ev_loop;

        ret = setup_atbus();
        if (ret < 0) {
            ss() << util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "setup atbus failed" << std::endl;
            bus_node_.reset();
            return setup_result_ = ret;
        }

        /* // step 7. all modules reload
         owent_foreach(module_ptr_t & mod, modules_) {
             if (mod->is_enabled()) {
                 ret = mod->reload();
                 if (ret < 0) {
                     ss() << util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "load configure of " << mod->name() << " failed" << std::endl;
                     return setup_result_ = ret;
                 }
             }
         }*/

        // step 8. all modules init
        size_t inited_mod_idx = 0;
        int    mod_init_res   = 0;
        for (; mod_init_res >= 0 && inited_mod_idx < modules_.size(); ++inited_mod_idx) {
            if (modules_[inited_mod_idx]->is_enabled()) {
                mod_init_res = modules_[inited_mod_idx]->init();
                if (mod_init_res < 0) {
                    ss() << util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "initialze " << modules_[inited_mod_idx]->name() << " failed" << std::endl;
                    break;
                }
            }
        }

        // cleanup all inited modules if failed
        if (mod_init_res < 0) {
            for (; inited_mod_idx < modules_.size(); --inited_mod_idx) {
                if (modules_[inited_mod_idx]) {
                    modules_[inited_mod_idx]->cleanup();
                }

                if (0 == inited_mod_idx) {
                    break;
                }
            }
            return setup_result_ = mod_init_res;
        }


        // callback of all modules inited
        if (evt_on_all_module_inited_) {
            evt_on_all_module_inited_(*this);
        }

        if (setup_timer() < 0) {
            // cleanup modules
            for (std::vector<module_ptr_t>::reverse_iterator rit = modules_.rbegin(); rit != modules_.rend(); ++rit) {
                if (*rit) {
                    (*rit)->cleanup();
                }
            }

            return EN_SHAPP_ERR_SETUP_TIMER;
        }

        set_flag(flag_t::STOPPED, false);
        set_flag(flag_t::STOPING, false);
        set_flag(flag_t::INITIALIZED, true);
        set_flag(flag_t::RUNNING, true);

        return EN_SHAPP_ERR_SUCCESS;
    }

    int app::run_noblock(uint64_t max_event_count) {
        uint64_t evt_count = 0;
        int      ret       = 0;
        do {
            ret = run_inner(UV_RUN_NOWAIT);
            if (ret < 0) {
                break;
            }

            if (0 == last_proc_event_count_) {
                break;
            }

            evt_count += last_proc_event_count_;
        } while (0 == max_event_count || evt_count < max_event_count);

        return ret;
    }

    int app::stop() {
        WLOGINFO("============ receive stop signal and ready to stop all modules ============");
        // step 1. set stop flag.
        // bool is_stoping = set_flag(flag_t::STOPING, true);
        set_flag(flag_t::STOPING, true);

        // TODO stop reason = manual stop
        if (bus_node_ && ::atbus::node::state_t::CREATED != bus_node_->get_state() && !bus_node_->check_flag(::atbus::node::flag_t::EN_FT_SHUTDOWN)) {
            bus_node_->shutdown(0);
        }

        // step 2. stop libuv and return from uv_run
        // if (!is_stoping) {
        if (bus_node_ && NULL != bus_node_->get_evloop()) {
            uv_stop(bus_node_->get_evloop());
        }
        // }
        return 0;
    }

    int app::tick() {
        int active_count;
        util::time::time_utility::update();
        // record start time point
        util::time::time_utility::raw_time_t start_tp = util::time::time_utility::now();
        util::time::time_utility::raw_time_t end_tp   = start_tp;
        do {
            if (tick_timer_.sec != util::time::time_utility::get_now()) {
                tick_timer_.sec        = util::time::time_utility::get_now();
                tick_timer_.usec       = 0;
                tick_timer_.sec_update = util::time::time_utility::now();
            } else {
                tick_timer_.usec = static_cast<time_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(util::time::time_utility::now() - tick_timer_.sec_update).count());
            }

            active_count = 0;
            int res;
            // step 1. proc available modules
            owent_foreach(module_ptr_t & mod, modules_) {
                if (mod->is_enabled()) {
                    res = mod->tick();
                    if (res < 0) {
                        WLOGERROR("module %s run tick and return %d", mod->name(), res);
                    } else {
                        active_count += res;
                    }
                }
            }

            // step 2. proc atbus
            if (bus_node_ && ::atbus::node::state_t::CREATED != bus_node_->get_state()) {
                res = bus_node_->proc(tick_timer_.sec, tick_timer_.usec);
                if (res < 0) {
                    WLOGERROR("atbus run tick and return %d", res);
                } else {
                    active_count += res;
                }
            }

            // only tick time less than tick interval will run loop again
            util::time::time_utility::update();
            end_tp = util::time::time_utility::now();

            if (active_count > 0) {
                last_proc_event_count_ += static_cast<uint64_t>(active_count);
            }
        } while (active_count > 0 && (end_tp - start_tp) < std::chrono::milliseconds(conf_.tick_interval));

        // if is stoping, quit loop  every tick
        if (check_flag(flag_t::STOPING) && bus_node_ && NULL != bus_node_->get_evloop()) {
            uv_stop(bus_node_->get_evloop());
        }

        // stat log
        do {
            time_t now_min = util::time::time_utility::get_now() / util::time::time_utility::MINITE_SECONDS;
            if (now_min != stat_.last_checkpoint_min) {
                time_t last_min           = stat_.last_checkpoint_min;
                stat_.last_checkpoint_min = now_min;
                if (last_min + 1 == now_min) {
                    uv_rusage_t last_usage;
                    memcpy(&last_usage, &stat_.last_checkpoint_usage, sizeof(uv_rusage_t));
                    if (0 != uv_getrusage(&stat_.last_checkpoint_usage)) {
                        break;
                    }
                    long offset_usr = stat_.last_checkpoint_usage.ru_utime.tv_sec - last_usage.ru_utime.tv_sec;
                    long offset_sys = stat_.last_checkpoint_usage.ru_stime.tv_sec - last_usage.ru_stime.tv_sec;
                    offset_usr *= 1000000;
                    offset_sys *= 1000000;
                    offset_usr += stat_.last_checkpoint_usage.ru_utime.tv_usec - last_usage.ru_utime.tv_usec;
                    offset_sys += stat_.last_checkpoint_usage.ru_stime.tv_usec - last_usage.ru_stime.tv_usec;

                    std::pair<uint64_t, const char *> max_rss = make_size_showup(last_usage.ru_maxrss);
#ifdef WIN32
                    WLOGINFO("[STAT]: %s CPU usage: user %02.03f%%, sys %02.03f%%, max rss: %llu%s, page faults: %llu", get_app_name().c_str(),
                             offset_usr / (util::time::time_utility::MINITE_SECONDS * 10000.0f), // usec and add %
                             offset_sys / (util::time::time_utility::MINITE_SECONDS * 10000.0f), // usec and add %
                             static_cast<unsigned long long>(max_rss.first), max_rss.second, static_cast<unsigned long long>(last_usage.ru_majflt));
#else
                    std::pair<uint64_t, const char *> ru_ixrss = make_size_showup(last_usage.ru_ixrss);
                    std::pair<uint64_t, const char *> ru_idrss = make_size_showup(last_usage.ru_idrss);
                    std::pair<uint64_t, const char *> ru_isrss = make_size_showup(last_usage.ru_isrss);
                    WLOGINFO("[STAT]: %s CPU usage: user %02.03f%%, sys %02.03f%%, max rss: %llu%s, shared size: %llu%s, unshared data size: %llu%s, unshared "
                             "stack size: %llu%s, page faults: %llu",
                             get_app_name().c_str(),
                             offset_usr / (util::time::time_utility::MINITE_SECONDS * 10000.0f), // usec and add %
                             offset_sys / (util::time::time_utility::MINITE_SECONDS * 10000.0f), // usec and add %
                             static_cast<unsigned long long>(max_rss.first), max_rss.second, static_cast<unsigned long long>(ru_ixrss.first), ru_ixrss.second,
                             static_cast<unsigned long long>(ru_idrss.first), ru_idrss.second, static_cast<unsigned long long>(ru_isrss.first), ru_isrss.second,
                             static_cast<unsigned long long>(last_usage.ru_majflt));
#endif
                } else {
                    uv_getrusage(&stat_.last_checkpoint_usage);
                }

                if (bus_node_ && NULL != bus_node_->get_evloop()) {
                    uv_stop(bus_node_->get_evloop());
                }
            }
        } while (false);
        return 0;
    }

    void app::add_module(app::module_ptr_t module) {
        if (this == module->owner_) {
            return;
        }

        assert(NULL == module->owner_);

        module->owner_ = this;
        modules_.push_back(module);
    }

    const std::shared_ptr<atbus::node> app::get_bus_node() const { return bus_node_; }

    std::shared_ptr<atbus::node> app::get_bus_node() { return bus_node_; }

    bool app::is_remote_address_available(const std::string &, const std::string &address) const {
        if (0 == UTIL_STRFUNC_STRNCASE_CMP("mem:", address.c_str(), 4)) {
            return false;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("shm:", address.c_str(), 4) || 0 == UTIL_STRFUNC_STRNCASE_CMP("unix:", address.c_str(), 5)) {
            // return hostname == ::atbus::node::get_hostname();
            // shm can not used as a remote address, it can only connect with a exist endpoint
            return false;
        }

        return true;
    }

    bool app::is_inited() const UTIL_CONFIG_NOEXCEPT { return check_flag(flag_t::INITIALIZED); }

    bool app::is_running() const UTIL_CONFIG_NOEXCEPT { return check_flag(flag_t::RUNNING); }

    bool app::is_closing() const UTIL_CONFIG_NOEXCEPT { return check_flag(flag_t::STOPING); }

    bool app::is_closed() const UTIL_CONFIG_NOEXCEPT { return check_flag(flag_t::STOPPED); }


    bool app::set_flag(flag_t::type f, bool v) {
        if (f < 0 || f >= flag_t::FLAG_MAX) {
            return false;
        }

        bool ret = flags_.test(f);
        flags_.set(f, v);
        return ret;
    }

    bool app::check_flag(app::flag_t::type f) const {
        if (f < 0 || f >= flag_t::FLAG_MAX) {
            return false;
        }
        return flags_.test(f);
    }


    void app::set_evt_on_recv_msg(callback_fn_on_msg_t fn) { evt_on_recv_msg_ = fn; }
    void app::set_evt_on_send_fail(callback_fn_on_send_fail_t fn) { evt_on_send_fail_ = fn; }
    void app::set_evt_on_app_connected(callback_fn_on_connected_t fn) { evt_on_app_connected_ = fn; }
    void app::set_evt_on_app_disconnected(callback_fn_on_disconnected_t fn) { evt_on_app_disconnected_ = fn; }
    void app::set_evt_on_all_module_inited(callback_fn_on_all_module_inited_t fn) { evt_on_all_module_inited_ = fn; }
    void app::set_evt_on_available(app::callback_fn_on_available_t fn) { evt_on_available_ = fn; }

    const app::callback_fn_on_msg_t &              app::get_evt_on_recv_msg() const { return evt_on_recv_msg_; }
    const app::callback_fn_on_send_fail_t &        app::get_evt_on_send_fail() const { return evt_on_send_fail_; }
    const app::callback_fn_on_connected_t &        app::get_evt_on_app_connected() const { return evt_on_app_connected_; }
    const app::callback_fn_on_disconnected_t &     app::get_evt_on_app_disconnected() const { return evt_on_app_disconnected_; }
    const app::callback_fn_on_all_module_inited_t &app::get_evt_on_all_module_inited() const { return evt_on_all_module_inited_; }
    const app::callback_fn_on_available_t &        app::get_callback_fn_on_available() const { return evt_on_available_; }


    app::app_id_t app::get_id() const { return conf_.id; }

    const std::string &app::get_app_name() const { return conf_.name; }

    int app::set_conf(const app_conf &conf) {
        int ret = 0;


        do {
            if (0 == conf.id) {
                ret = -1;
                break;
            }
        } while (0);

        conf_ = conf;
        return ret;
    }


    void app::run_ev_loop(int run_mode) {
        util::time::time_utility::update();

        if (bus_node_) {
            // step X. loop uv_run util stop flag is set
            assert(bus_node_->get_evloop());
            if (NULL != bus_node_->get_evloop()) {
                flag_guard_t in_callback_guard(*this, flag_t::IN_CALLBACK);
                uv_run(bus_node_->get_evloop(), static_cast<uv_run_mode>(run_mode));
            }

            if (check_flag(flag_t::RESET_TIMER)) {
                setup_timer();
            }

            if (check_flag(flag_t::STOPING)) {
                set_flag(flag_t::STOPPED, true);

                if (check_flag(flag_t::TIMEOUT)) {
                    // step X. notify all modules timeout
                    owent_foreach(module_ptr_t & mod, modules_) {
                        if (mod->is_enabled()) {
                            WLOGERROR("try to stop module %s but timeout", mod->name());
                            mod->timeout();
                            mod->disable();
                        }
                    }
                } else {
                    // step X. notify all modules to finish and wait for all modules stop
                    owent_foreach(module_ptr_t & mod, modules_) {
                        if (mod->is_enabled()) {
                            int res = mod->stop();
                            if (0 == res) {
                                mod->disable();
                            } else if (res < 0) {
                                mod->disable();
                                WLOGERROR("try to stop module %s but failed and return %d", mod->name(), res);
                            } else {
                                // any module stop running will make app wait
                                set_flag(flag_t::STOPPED, false);
                            }
                        }
                    }

                    // step X. if stop is blocked and timeout not triggered, setup stop timeout and waiting for all modules finished
                    if (false == tick_timer_.timeout_timer.is_activited) {
                        uv_timer_init(bus_node_->get_evloop(), &tick_timer_.timeout_timer.timer);
                        tick_timer_.timeout_timer.timer.data = this;

                        int res = uv_timer_start(&tick_timer_.timeout_timer.timer, ev_stop_timeout, conf_.stop_timeout, 0);
                        if (0 == res) {
                            tick_timer_.timeout_timer.is_activited = true;
                        } else {
                            WLOGERROR("setup stop timeout failed, res: %d", res);
                            set_flag(flag_t::TIMEOUT, false);
                        }
                    }
                }
            }

            // if atbus is at shutdown state, loop event dispatcher using next while iterator
        }
    }
    int app::run_inner(int run_mode) {
        if (false == check_flag(flag_t::INITIALIZED)) {
            return EN_SHAPP_ERR_NOT_INITED;
        }

        last_proc_event_count_ = 0;
        if (check_flag(flag_t::IN_CALLBACK)) {
            return 0;
        }

        // TODO if atbus is reset, init it again

        run_ev_loop(run_mode);

        if (is_closed() && is_inited()) {
            // close timer
            close_timer(tick_timer_.tick_timer);
            close_timer(tick_timer_.timeout_timer);

            // cleanup modules
            for (std::vector<module_ptr_t>::reverse_iterator rit = modules_.rbegin(); rit != modules_.rend(); ++rit) {
                if (*rit) {
                    (*rit)->cleanup();
                }
            }


            set_flag(flag_t::INITIALIZED, false);
            set_flag(flag_t::RUNNING, false);
        }

        if (last_proc_event_count_ > 0) {
            return 1;
        }

        return 0;
    }

    static void _app_tick_timer_handle(uv_timer_t *handle) {
        assert(handle && handle->data);

        if (NULL != handle && NULL != handle->data) {
            app *self = reinterpret_cast<app *>(handle->data);
            self->tick();
        }
    }

    static void ondebug(const char *file_path, size_t  line, const atbus::node &, const atbus::endpoint *, const atbus::connection *,
                        const atbus::protocol::msg *, const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        char output[4097] = {0};
        size_t size = std::vsnprintf(output, 4096, fmt, args);
        va_end(args);
        util::log::log_adaptor::get_instance().on_log(util::log::LOG_DEBUG, file_path, line, "", output, size);

    }

    int app::setup_atbus() {
        int ret = 0, res = 0;
        if (bus_node_) {
            bus_node_->reset();
            bus_node_.reset();
        }

        std::shared_ptr<atbus::node> connection_node = atbus::node::create();
        if (!connection_node) {
            WLOGERROR("create bus node failed.");
            return EN_SHAPP_ERR_SETUP_ATBUS;
        }

        ret = connection_node->init(conf_.id, &conf_.bus_conf);
        if (ret < 0) {
            WLOGERROR("init bus node failed. ret: %d", ret);
            return EN_SHAPP_ERR_SETUP_ATBUS;
        }

        // setup all callbacks
        connection_node->set_on_recv_handle(std::bind(&app::bus_evt_callback_on_recv_msg, this, std::placeholders::_1, std::placeholders::_2,
                                                      std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

        connection_node->set_on_send_data_failed_handle(
            std::bind(&app::bus_evt_callback_on_send_failed, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

        connection_node->set_on_error_handle(std::bind(&app::bus_evt_callback_on_error, this, std::placeholders::_1, std::placeholders::_2,
                                                       std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

        connection_node->set_on_register_handle(
            std::bind(&app::bus_evt_callback_on_reg, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

        connection_node->set_on_shutdown_handle(std::bind(&app::bus_evt_callback_on_shutdown, this, std::placeholders::_1, std::placeholders::_2));

        connection_node->set_on_available_handle(std::bind(&app::bus_evt_callback_on_available, this, std::placeholders::_1, std::placeholders::_2));

        connection_node->set_on_invalid_connection_handle(
            std::bind(&app::bus_evt_callback_on_invalid_connection, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        connection_node->set_on_add_endpoint_handle(
            std::bind(&app::bus_evt_callback_on_add_endpoint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        connection_node->set_on_remove_endpoint_handle(
            std::bind(&app::bus_evt_callback_on_remove_endpoint, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        connection_node->on_debug = ondebug;


        // init listen
        for (size_t i = 0; i < conf_.bus_listen.size(); ++i) {
            res = connection_node->listen(conf_.bus_listen[i].c_str());
            if (res < 0) {
#ifdef _WIN32
                if (EN_ATBUS_ERR_SHM_GET_FAILED == res) {
                    WLOGERROR("Using global shared memory require SeCreateGlobalPrivilege, try to run as Administrator.\nWe will ignore %s this time.",
                              conf_.bus_listen[i].c_str());
                    util::cli::shell_stream ss(std::cerr);
                    ss() << util::cli::shell_font_style::SHELL_FONT_COLOR_RED
                         << "Using global shared memory require SeCreateGlobalPrivilege, try to run as Administrator." << std::endl
                         << "We will ignore " << conf_.bus_listen[i] << " this time." << std::endl;

                    // res = 0; // Value stored to 'res' is never read
                } else {
#endif
                    WLOGERROR("bus node listen %s failed. res: %d", conf_.bus_listen[i].c_str(), res);
                    if (EN_ATBUS_ERR_PIPE_ADDR_TOO_LONG == res) {
                        atbus::channel::channel_address_t address;
                        atbus::channel::make_address(conf_.bus_listen[i].c_str(), address);
                        std::string abs_path = util::file_system::get_abs_path(address.host.c_str());
                        WLOGERROR("listen pipe socket %s, but the length (%llu) exceed the limit %llu", abs_path.c_str(),
                                  static_cast<unsigned long long>(abs_path.size()),
                                  static_cast<unsigned long long>(atbus::channel::io_stream_get_max_unix_socket_length()));
                    }
                    ret = res;
#ifdef _WIN32
                }
#endif
            }
        }

        if (ret < 0) {
            WLOGERROR("bus node listen failed");
            return ret;
        }

        // start
        ret = connection_node->start();
        if (ret < 0) {
            WLOGERROR("bus node start failed, ret: %d", ret);
            return ret;
        }

        // edit by tom

        // if has father node, block and connect to father node
        if (atbus::node::state_t::CONNECTING_PARENT == connection_node->get_state() || atbus::node::state_t::LOST_PARENT == connection_node->get_state()) {
            // setup timeout and waiting for parent connected
            if (false == tick_timer_.timeout_timer.is_activited) {
                uv_timer_init(connection_node->get_evloop(), &tick_timer_.timeout_timer.timer);
                tick_timer_.timeout_timer.timer.data = this;

                res = uv_timer_start(&tick_timer_.timeout_timer.timer, ev_stop_timeout, conf_.stop_timeout, 0);
                if (0 == res) {
                    tick_timer_.timeout_timer.is_activited = true;
                } else {
                    WLOGERROR("setup stop timeout failed, res: %d", res);
                    set_flag(flag_t::TIMEOUT, false);
                }

                while (NULL == connection_node->get_parent_endpoint()) {
                    if (check_flag(flag_t::TIMEOUT)) {
                        WLOGERROR("connection to parent node %s timeout", conf_.bus_conf.father_address.c_str());
                        ret = -1;
                        break;
                    }

                    {
                        flag_guard_t in_callback_guard(*this, flag_t::IN_CALLBACK);
                        uv_run(connection_node->get_evloop(), UV_RUN_ONCE);
                    }
                }

                // if connected, do not trigger timeout
                close_timer(tick_timer_.timeout_timer);

                if (ret < 0) {
                    WLOGERROR("connect to parent node failed");
                    return ret;
                }
            }
        }

        bus_node_ = connection_node;

        return 0;
    }

    int app::setup_timer() {
        set_flag(flag_t::RESET_TIMER, false);

        close_timer(tick_timer_.tick_timer);

        if (conf_.tick_interval < 1) {
            conf_.tick_interval = 1;
            WLOGWARNING("tick interval can not smaller than 1ms, we use 1ms now.");
        } else {
            WLOGINFO("setup tick interval to %llums.", static_cast<unsigned long long>(conf_.tick_interval));
        }

        uv_timer_init(bus_node_->get_evloop(), &tick_timer_.tick_timer.timer);
        tick_timer_.tick_timer.timer.data = this;

        int res = uv_timer_start(&tick_timer_.tick_timer.timer, _app_tick_timer_handle, conf_.tick_interval, conf_.tick_interval);
        if (0 == res) {
            tick_timer_.tick_timer.is_activited = true;
        } else {
            WLOGERROR("setup tick timer failed, res: %d", res);
            return EN_SHAPP_ERR_SETUP_TIMER;
        }

        return 0;
    }


    void app::close_timer(app::timer_info_t &t) {
        if (t.is_activited) {
            uv_timer_stop(&t.timer);
            uv_close(reinterpret_cast<uv_handle_t *>(&t.timer), NULL);
            t.is_activited = false;
        }
    }


    int app::bus_evt_callback_on_recv_msg(const atbus::node &, const atbus::endpoint *, const atbus::connection *, const msg_t &msg, const void *buffer,
                                          size_t len) {
        // call recv callback
        if (evt_on_recv_msg_) {
            return evt_on_recv_msg_(std::ref(*this), std::cref(msg), buffer, len);
        }

        ++last_proc_event_count_;
        return 0;
    }

    int app::bus_evt_callback_on_send_failed(const atbus::node &, const atbus::endpoint *, const atbus::connection *, const atbus::protocol::msg *m) {
        ++last_proc_event_count_;

        // call failed callback if it's message transfer
        if (NULL == m) {
            WLOGERROR("app 0x%llx receive a send failure without message", static_cast<unsigned long long>(get_id()));
            return EN_SHAPP_ERR_SEND_FAILED;
        }

        WLOGERROR("app 0x%llx receive a send failure from 0x%llx, message cmd: %d, type: %d, ret: %d, sequence: %llu",
                  static_cast<unsigned long long>(get_id()), static_cast<unsigned long long>(m->head.src_bus_id), static_cast<int>(m->head.cmd), m->head.type,
                  m->head.ret, static_cast<unsigned long long>(m->head.sequence));

        if ((ATBUS_CMD_DATA_TRANSFORM_REQ == m->head.cmd || ATBUS_CMD_DATA_TRANSFORM_RSP == m->head.cmd) && evt_on_send_fail_) {
            app_id_t origin_from = m->body.forward->to;
            app_id_t origin_to   = m->body.forward->from;
            return evt_on_send_fail_(std::ref(*this), origin_from, origin_to, std::cref(*m));
        }

        return 0;
    }

    int app::bus_evt_callback_on_error(const atbus::node &n, const atbus::endpoint *ep, const atbus::connection *conn, int status, int errcode) {

        const char *print_msg = uv_err_name(errcode);

        // meet eof or reset by peer is not a error
        if (UV_EOF == errcode || UV_ECONNRESET == errcode) {
            const char *msg = UV_EOF == errcode ? "got EOF" : "reset by peer";
            if (NULL != conn) {
                if (NULL != ep) {
                    WLOGINFO("bus node 0x%llx endpoint 0x%llx connection %s closed: %s", static_cast<unsigned long long>(n.get_id()),
                             static_cast<unsigned long long>(ep->get_id()), conn->get_address().address.c_str(), msg);
                } else {
                    WLOGINFO("bus node 0x%llx connection %s closed: %s", static_cast<unsigned long long>(n.get_id()), conn->get_address().address.c_str(), msg);
                }

            } else {
                if (NULL != ep) {
                    WLOGINFO("bus node 0x%llx endpoint 0x%llx closed: %s", static_cast<unsigned long long>(n.get_id()),
                             static_cast<unsigned long long>(ep->get_id()), msg);
                } else {
                    WLOGINFO("bus node 0x%llx closed: %s", static_cast<unsigned long long>(n.get_id()), msg);
                }
            }
            return 0;
        }

        if (NULL != conn) {
            if (NULL != ep) {
                WLOGERROR("bus node 0x%llx endpoint 0x%llx connection %s error, status: %d, error code: %d", static_cast<unsigned long long>(n.get_id()),
                          static_cast<unsigned long long>(ep->get_id()), conn->get_address().address.c_str(), status, errcode);
            } else {
                WLOGERROR("bus node 0x%llx connection %s error, status: %d, error code: %d print_msg:%s", static_cast<unsigned long long>(n.get_id()),
                          conn->get_address().address.c_str(), status, errcode, print_msg);
            }

        } else {
            if (NULL != ep) {
                WLOGERROR("bus node 0x%llx endpoint 0x%llx error, status: %d, error code: %d", static_cast<unsigned long long>(n.get_id()),
                          static_cast<unsigned long long>(ep->get_id()), status, errcode);
            } else {
                WLOGERROR("bus node 0x%llx error, status: %d, error code: %d", static_cast<unsigned long long>(n.get_id()), status, errcode);
            }
        }

        return 0;
    }

    int app::bus_evt_callback_on_reg(const atbus::node &n, const atbus::endpoint *ep, const atbus::connection *conn, int res) {
        ++last_proc_event_count_;

        if (NULL != conn) {
            if (NULL != ep) {
                WLOGINFO("bus node 0x%llx endpoint 0x%llx connection %s registered, res: %d", static_cast<unsigned long long>(n.get_id()),
                         static_cast<unsigned long long>(ep->get_id()), conn->get_address().address.c_str(), res);
            } else {
                WLOGINFO("bus node 0x%llx connection %s registered, res: %d", static_cast<unsigned long long>(n.get_id()), conn->get_address().address.c_str(),
                         res);
            }

        } else {
            if (NULL != ep) {
                WLOGINFO("bus node 0x%llx endpoint 0x%llx registered, res: %d", static_cast<unsigned long long>(n.get_id()),
                         static_cast<unsigned long long>(ep->get_id()), res);
            } else {
                WLOGINFO("bus node 0x%llx registered, res: %d", static_cast<unsigned long long>(n.get_id()), res);
            }
        }

        return 0;
    }

    int app::bus_evt_callback_on_shutdown(const atbus::node &n, int reason) {
        WLOGINFO("bus node 0x%llx shutdown, reason: %d", static_cast<unsigned long long>(n.get_id()), reason);
        return stop();
    }

    int app::bus_evt_callback_on_available(const atbus::node &n, int res) {
        WLOGINFO("bus node 0x%llx initialze done, res: %d", static_cast<unsigned long long>(n.get_id()), res);
        if (evt_on_available_) {
            return evt_on_available_(std::ref(*this), res);
        }
        return res;
    }

    int app::bus_evt_callback_on_invalid_connection(const atbus::node &n, const atbus::connection *conn, int res) {
        ++last_proc_event_count_;

        if (NULL == conn) {
            WLOGERROR("bus node 0x%llx recv a invalid NULL connection , res: %d", static_cast<unsigned long long>(n.get_id()), res);
        } else {
            // already disconncted finished.
            if (atbus::connection::state_t::DISCONNECTED != conn->get_status()) {
                WLOGERROR("bus node 0x%llx make connection to %s done, res: %d", static_cast<unsigned long long>(n.get_id()),
                          conn->get_address().address.c_str(), res);
            }
        }
        return 0;
    }

    int app::bus_evt_callback_on_add_endpoint(const atbus::node &n, atbus::endpoint *ep, int res) {
        ++last_proc_event_count_;

        if (NULL == ep) {
            WLOGERROR("bus node 0x%llx make connection to NULL, res: %d", static_cast<unsigned long long>(n.get_id()), res);
        } else {
            WLOGINFO("bus node 0x%llx make connection to 0x%llx done, res: %d", static_cast<unsigned long long>(n.get_id()),
                     static_cast<unsigned long long>(ep->get_id()), res);

            if (evt_on_app_connected_) {
                evt_on_app_connected_(std::ref(*this), std::ref(*ep), res);
            }
        }
        return 0;
    }

    int app::bus_evt_callback_on_remove_endpoint(const atbus::node &n, atbus::endpoint *ep, int res) {
        ++last_proc_event_count_;

        if (NULL == ep) {
            WLOGERROR("bus node 0x%llx release connection to NULL, res: %d", static_cast<unsigned long long>(n.get_id()), res);
        } else {
            WLOGINFO("bus node 0x%llx release connection to 0x%llx done, res: %d", static_cast<unsigned long long>(n.get_id()),
                     static_cast<unsigned long long>(ep->get_id()), res);

            if (evt_on_app_disconnected_) {
                evt_on_app_disconnected_(std::ref(*this), std::ref(*ep), res);
            }
        }
        return 0;
    }

    const app_conf &app::get_conf() const {
        return std::cref(conf_);
        // return  conf_;
    }
    const std::string &app::get_region() const { return conf_.region; }


} // namespace shapp