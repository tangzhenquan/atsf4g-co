
#ifndef ATFRAME_LIBSHAPP_SHAPP_H
#define ATFRAME_LIBSHAPP_SHAPP_H
#pragma once
#include <stdint.h>

#include "uv.h"
#include "shapp_conf.h"
#include "time/time_utility.h"
#include "shapp_module_impl.h"

namespace atbus {
    namespace protocol {
        struct msg;
    }
    class node;
    class endpoint;
    class connection;
} // namespace atbus

namespace shapp {
    class app {
    public:
        typedef uint64_t app_id_t;
        typedef atbus::protocol::msg msg_t;
        typedef std::shared_ptr<module_impl> module_ptr_t;

        struct timer_info_t {
            bool is_activited;
            uv_timer_t timer;
        };

        struct tick_timer_t {
            util::time::time_utility::raw_time_t sec_update;
            time_t sec;
            time_t usec;

            timer_info_t tick_timer;
            timer_info_t timeout_timer;
        };
        struct flag_t {
            enum type {
                RUNNING = 0, //
                STOPING,     //
                TIMEOUT,
                IN_CALLBACK,
                RESET_TIMER,
                INITIALIZED,
                STOPPED,
                FLAG_MAX
            };
        };

        class flag_guard_t {
        public:
            flag_guard_t(app &owner, flag_t::type f);
            ~flag_guard_t();

        private:
            flag_guard_t(const flag_guard_t &);

            app *owner_;
            flag_t::type flag_;
        };
        friend class flag_guard_t;

        typedef std::function<int(app &, const msg_t &, const void *, size_t)> callback_fn_on_msg_t;
        typedef std::function<int(app &, app_id_t src_pd, app_id_t dst_pd, const msg_t &m)> callback_fn_on_send_fail_t;
        typedef std::function<int(app &, atbus::endpoint &, int)> callback_fn_on_connected_t;
        typedef std::function<int(app &, atbus::endpoint &, int)> callback_fn_on_disconnected_t;
        typedef std::function<int(app &)> callback_fn_on_all_module_inited_t;
        /*typedef std::function<int(app &, const atbus::protocol::custom_route_data&, std::vector<uint64_t >& )>
                callback_fn_on_custom_route_t;*/
        typedef std::function<int(app &, int)> callback_fn_on_available_t;

    public:
        app();
        ~app();


        int run(uv_loop_t *ev_loop, const app_conf& conf);
        int run();
        int init(uv_loop_t *ev_loop, const app_conf& conf);
        int run_noblock(uint64_t max_event_count = 20000);
        int stop();
        int tick();

        void add_module(module_ptr_t module);
        template <typename TModPtr>
        void add_module(TModPtr module) {
            add_module(std::dynamic_pointer_cast<module_impl>(module));
        }

        std::shared_ptr<atbus::node> get_bus_node();

        const std::shared_ptr<atbus::node> get_bus_node() const;

        bool is_remote_address_available(const std::string &hostname, const std::string &address) const;

        bool is_inited() const UTIL_CONFIG_NOEXCEPT;

        bool is_running() const UTIL_CONFIG_NOEXCEPT;

        bool is_closing() const UTIL_CONFIG_NOEXCEPT;

        bool is_closed() const UTIL_CONFIG_NOEXCEPT;

        bool check_flag(flag_t::type f) const;


        void set_evt_on_recv_msg(callback_fn_on_msg_t fn);

        void set_evt_on_send_fail(callback_fn_on_send_fail_t fn);

        void set_evt_on_app_connected(callback_fn_on_connected_t fn);

        void set_evt_on_app_disconnected(callback_fn_on_disconnected_t fn);

        void set_evt_on_all_module_inited(callback_fn_on_all_module_inited_t fn);

        void set_evt_on_available(callback_fn_on_available_t fn);

        //void set_evt_on_on_custom_route(callback_fn_on_custom_route_t fn);

        const callback_fn_on_msg_t &get_evt_on_recv_msg() const;

        const callback_fn_on_send_fail_t &get_evt_on_send_fail() const;

        const callback_fn_on_connected_t &get_evt_on_app_connected() const;

        const callback_fn_on_disconnected_t &get_evt_on_app_disconnected() const;

        const callback_fn_on_all_module_inited_t &get_evt_on_all_module_inited() const;

        const callback_fn_on_available_t &get_callback_fn_on_available() const ;

        //const callback_fn_on_custom_route_t &get_evt_on_on_custom_route() const;

        app_id_t get_id() const;
        const std::string &get_app_name() const;

        const app_conf& get_conf() const;


    private:
        static void ev_stop_timeout(uv_timer_t *handle);

        bool set_flag(flag_t::type f, bool v);

        int setup_atbus();

        int set_conf(const app_conf& conf);

        void run_ev_loop(int run_mode);

        int run_inner(int run_mode);

        void close_timer(timer_info_t &t);

        int setup_timer();

    private:
        int bus_evt_callback_on_recv_msg(const atbus::node &, const atbus::endpoint *, const atbus::connection *, const msg_t &, const void *, size_t);
        int bus_evt_callback_on_send_failed(const atbus::node &, const atbus::endpoint *, const atbus::connection *, const atbus::protocol::msg *m);
        int bus_evt_callback_on_error(const atbus::node &, const atbus::endpoint *, const atbus::connection *, int, int);
        int bus_evt_callback_on_reg(const atbus::node &, const atbus::endpoint *, const atbus::connection *, int);
        int bus_evt_callback_on_shutdown(const atbus::node &, int);
        int bus_evt_callback_on_available(const atbus::node &, int);
        int bus_evt_callback_on_invalid_connection(const atbus::node &, const atbus::connection *, int);
        int bus_evt_callback_on_add_endpoint(const atbus::node &, atbus::endpoint *, int);
        int bus_evt_callback_on_remove_endpoint(const atbus::node &, atbus::endpoint *, int);

    private:

        std::shared_ptr<atbus::node> bus_node_;
        app_conf conf_;
        tick_timer_t tick_timer_;
        std::vector<module_ptr_t> modules_;
        std::bitset<flag_t::FLAG_MAX> flags_;
        int setup_result_;
        uint64_t last_proc_event_count_;

        // callbacks
        callback_fn_on_msg_t evt_on_recv_msg_;
        callback_fn_on_send_fail_t evt_on_send_fail_;
        callback_fn_on_connected_t evt_on_app_connected_;
        callback_fn_on_disconnected_t evt_on_app_disconnected_;
        callback_fn_on_all_module_inited_t evt_on_all_module_inited_;
        callback_fn_on_available_t evt_on_available_;



        // stat
        typedef struct {
            uv_rusage_t last_checkpoint_usage;
            time_t last_checkpoint_min;
        } stat_data_t;
        stat_data_t stat_;
    };


}// namespace shapp

#endif //ATFRAME_LIBSHAPP_SHAPP_H
