
#include <sstream>
#include <vector>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <common/string_oprs.h>
#include <random/random_generator.h>

#include <shapp.h>


#include "shapp_etcd_module.h"
#include "shapp_log.h"

#define ETCD_MODULE_STARTUP_RETRY_TIMES 5
#define ETCD_MODULE_BY_ID_DIR "by_id"
#define ETCD_MODULE_BY_TYPE_ID_DIR "by_type_id"
#define ETCD_MODULE_BY_TYPE_NAME_DIR "by_type_name"
#define ETCD_MODULE_BY_NAME_DIR "by_name"
#define ETCD_MODULE_BY_TAG "by_tag"
#define ETCD_MODULE_BY_CUSTOM "by_custom"

namespace atframe {
    namespace component {
        namespace detail {


            static void init_timer_timeout_callback(uv_timer_t *handle) {
                assert(handle);
                assert(handle->data);
                assert(handle->loop);

                bool *is_timeout = reinterpret_cast<bool *>(handle->data);
                *is_timeout      = true;
                uv_stop(handle->loop);
            }

            static void init_timer_closed_callback(uv_handle_t *handle) {
                assert(handle);
                assert(handle->data);
                assert(handle->loop);

                bool *is_timeout = reinterpret_cast<bool *>(handle->data);
                *is_timeout      = false;
                uv_stop(handle->loop);
            }

        } // namespace detail


        shapp_etcd_module::shapp_etcd_module() {
            conf_.path_prefix             = "/";
            conf_.etcd_init_timeout       = std::chrono::seconds(5);  // 初始化超时5秒
            conf_.watcher_retry_interval  = std::chrono::seconds(15); // 重试间隔15秒
            conf_.watcher_request_timeout = std::chrono::hours(1);    // 一小时超时时间，相当于每小时重新拉取数据

            conf_.report_alive_by_id   = false;
            conf_.report_alive_by_type = false;
            conf_.report_alive_by_name = false;
            conf_.report_alive_by_tag.clear();
        }

        shapp_etcd_module::~shapp_etcd_module() { reset(); }

        void shapp_etcd_module::reset() {
            if (cleanup_request_) {
                cleanup_request_->set_priv_data(NULL);
                cleanup_request_->set_on_complete(NULL);
                cleanup_request_->stop();
                cleanup_request_.reset();
            }

            etcd_ctx_.reset();

            if (curl_multi_) {
                util::network::http_request::destroy_curl_multi(curl_multi_);
            }
        }

        int shapp_etcd_module::init() {
            // init curl
            int res = curl_global_init(CURL_GLOBAL_ALL);
            if (res) {
                WLOGERROR("init cURL failed, errcode: %d", res);
                return -1;
            }

            if (etcd_ctx_.get_conf_hosts().empty()) {
                WLOGINFO("etcd host not found, start singel mode");
                return 0;
            }

            util::network::http_request::create_curl_multi(get_app()->get_bus_node()->get_evloop(), curl_multi_);
            if (!curl_multi_) {
                WLOGERROR("create curl multi instance failed.");
                return -1;
            }

            etcd_ctx_.init(curl_multi_);

            // generate keepalive data
            std::vector<atframe::component::etcd_keepalive::ptr_t> keepalive_actors;
            std::string                                            keepalive_val;
            keepalive_actors.reserve(4);
            if (conf_.report_alive_by_id) {
                atframe::component::etcd_keepalive::ptr_t actor = add_keepalive_actor(keepalive_val, get_by_id_path());
                if (!actor) {
                    WLOGERROR("create etcd_keepalive for by_id index failed.");
                    return -1;
                }

                keepalive_actors.push_back(actor);
                WLOGINFO("create etcd_keepalive for by_id index %s success", get_by_id_path().c_str());
            }

            if (conf_.report_alive_by_type) {
                /*atframe::component::etcd_keepalive::ptr_t actor = add_keepalive_actor(keepalive_val, get_by_type_id_path());
                if (!actor) {
                    WLOGERROR("create etcd_keepalive for by_type_id index failed.");
                    return -1;
                }
                keepalive_actors.push_back(actor);
                WLOGINFO("create etcd_keepalive for by_type_id index %s success", get_by_type_id_path().c_str());*/

                atframe::component::etcd_keepalive::ptr_t actor = add_keepalive_actor(keepalive_val, get_by_type_name_path());
                if (!actor) {
                    WLOGERROR("create etcd_keepalive for by_type_name index failed.");
                    return -1;
                }
                keepalive_actors.push_back(actor);

                WLOGINFO("create etcd_keepalive for by_type_name index %s success", get_by_type_name_path().c_str());
            }

            if (conf_.report_alive_by_name) {
                atframe::component::etcd_keepalive::ptr_t actor = add_keepalive_actor(keepalive_val, get_by_name_path());
                if (!actor) {
                    WLOGERROR("create etcd_keepalive for by_name index failed.");
                    return -1;
                }

                keepalive_actors.push_back(actor);
                WLOGINFO("create etcd_keepalive for by_name index %s success", get_by_name_path().c_str());
            }

            for (size_t i = 0; i < conf_.report_alive_by_tag.size(); ++i) {
                const std::string &tag_name = conf_.report_alive_by_tag[i];

                if (tag_name.empty()) {
                    continue;
                }

                atframe::component::etcd_keepalive::ptr_t actor = add_keepalive_actor(keepalive_val, get_by_tag_path(tag_name));
                if (!actor) {
                    WLOGERROR("create etcd_keepalive for by_tag %s index failed.", tag_name.c_str());
                    return -1;
                }

                keepalive_actors.push_back(actor);
                WLOGINFO("create etcd_keepalive for by_tag index %s success", get_by_tag_path(tag_name).c_str());
            }

            // 执行到首次检测结束
            bool is_failed  = false;
            bool is_timeout = false;

            // setup timer for timeout
            uv_timer_t timeout_timer;
            uv_timer_init(get_app()->get_bus_node()->get_evloop(), &timeout_timer);
            timeout_timer.data = &is_timeout;

            uint64_t timeout_ms = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(conf_.etcd_init_timeout).count());
            uv_timer_start(&timeout_timer, detail::init_timer_timeout_callback, timeout_ms, 0);

            int ticks = 0;
            while (false == is_failed && false == is_timeout) {
                util::time::time_utility::update();
                etcd_ctx_.tick();
                ++ticks;

                size_t run_count = 0;
                for (size_t i = 0; false == is_failed && i < keepalive_actors.size(); ++i) {
                    if (keepalive_actors[i]->is_check_run()) {
                        if (!keepalive_actors[i]->is_check_passed()) {
                            WLOGERROR("etcd_keepalive lock %s failed.", keepalive_actors[i]->get_path().c_str());
                            is_failed = true;
                        }

                        ++run_count;
                    }
                }

                // 全部成功或任意失败则退出
                if (is_failed || run_count >= keepalive_actors.size()) {
                    break;
                }

                uv_run(get_app()->get_bus_node()->get_evloop(), UV_RUN_ONCE);

                // 任意重试次数过多则失败退出
                for (size_t i = 0; false == is_failed && i < keepalive_actors.size(); ++i) {
                    if (keepalive_actors[i]->get_check_times() >= ETCD_MODULE_STARTUP_RETRY_TIMES ||
                        etcd_ctx_.get_stats().continue_error_requests > ETCD_MODULE_STARTUP_RETRY_TIMES) {
                        size_t retry_times = keepalive_actors[i]->get_check_times();
                        if (etcd_ctx_.get_stats().continue_error_requests > retry_times) {
                            retry_times = etcd_ctx_.get_stats().continue_error_requests > retry_times;
                        }
                        WLOGERROR("etcd_keepalive request %s for %llu times (with %d ticks) failed.", keepalive_actors[i]->get_path().c_str(),
                                  static_cast<unsigned long long>(retry_times), ticks);
                        is_failed = true;
                        break;
                    }
                }

                if (is_failed) {
                    break;
                }
            }

            if (is_timeout) {
                is_failed = true;
                for (size_t i = 0; i < keepalive_actors.size(); ++i) {
                    size_t retry_times = keepalive_actors[i]->get_check_times();
                    if (etcd_ctx_.get_stats().continue_error_requests > retry_times) {
                        retry_times = etcd_ctx_.get_stats().continue_error_requests;
                    }
                    WLOGERROR("etcd_keepalive request %s timeout, retry %llu times (with %d ticks).", keepalive_actors[i]->get_path().c_str(),
                              static_cast<unsigned long long>(retry_times), ticks);
                }
            }

            // close timer for timeout
            uv_timer_stop(&timeout_timer);
            is_timeout = true;
            uv_close((uv_handle_t *)&timeout_timer, detail::init_timer_closed_callback);
            while (is_timeout) {
                uv_run(get_app()->get_bus_node()->get_evloop(), UV_RUN_ONCE);
            }

            // 初始化失败则回收资源
            if (is_failed) {
                stop();
                reset();
                return -1;
            }

            return res;
        }


        int shapp_etcd_module::http_callback_on_etcd_closed(util::network::http_request &req) {
            shapp_etcd_module *self = reinterpret_cast<shapp_etcd_module *>(req.get_priv_data());
            if (NULL == self) {
                WLOGERROR("shapp_etcd_module get request shouldn't has request without private data");
                return 0;
            }

            self->cleanup_request_.reset();

            WLOGDEBUG("Etcd revoke lease finished");

            // call stop to trigger stop process again.
            self->get_app()->stop();

            return 0;
        }

        int shapp_etcd_module::stop() {
            if (!cleanup_request_) {
                cleanup_request_ = etcd_ctx_.close(false);

                if (cleanup_request_ && cleanup_request_->is_running()) {
                    cleanup_request_->set_priv_data(this);
                    cleanup_request_->set_on_complete(http_callback_on_etcd_closed);
                }
            }

            if (cleanup_request_) {
                return 1;
            }

            // recycle all resources
            reset();
            return 0;
        }

        int shapp_etcd_module::timeout() {
            reset();
            return 0;
        }

        const char *shapp_etcd_module::name() const { return "sh etcd module"; }

        int shapp_etcd_module::tick() {
            // single mode
            if (etcd_ctx_.get_conf_hosts().empty()) {
                return 0;
            }

            // first startup when reloaded
            if (!curl_multi_) {
                int res = init();
                if (res < 0) {
                    WLOGERROR("initialize etcd failed, res: %d", res);
                    get_app()->stop();
                    return res;
                }
            }

            return etcd_ctx_.tick();
        }

        std::string shapp_etcd_module::get_by_id_path() const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_ID_DIR ;
            if (!get_app()->get_region().empty()){
                ss << "/" << get_app()->get_region();
            }
            ss << "/" << get_app()->get_id();
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_type_id_path() const {
           //std::stringstream ss;
           //ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_ID_DIR << "/" << get_app()->get_conf(). << "/" << get_app()->get_id();
           //return ss.str();
            return "";
        }

        std::string shapp_etcd_module::get_by_type_name_path() const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_NAME_DIR << "/" <<get_app()->get_conf().type_name ;
            if (!get_app()->get_region().empty()){
                ss << "/" << get_app()->get_region();
            }
            ss << "/" << get_app()->get_id();
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_name_path() const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_NAME_DIR ;
            if (!get_app()->get_region().empty()){
                ss << "/" << get_app()->get_region();
            }
            ss << "/" << get_app()->get_app_name();
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_tag_path(const std::string &tag_name) const {

            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TAG << "/" << tag_name;
            if (!get_app()->get_region().empty()){
                ss << "/" << get_app()->get_region();
            }
            ss << "/" << get_app()->get_id();
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_id_watcher_path() const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_ID_DIR;
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_type_id_watcher_path(uint64_t type_id) const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_ID_DIR << "/" << type_id;
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_type_name_watcher_path(const std::string &type_name) const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_NAME_DIR << "/" << type_name;
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_name_watcher_path() const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_NAME_DIR;
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_tag_watcher_path(const std::string &tag_name) const {
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TAG << "/" << tag_name;
            return ss.str();
        }

        std::string shapp_etcd_module::get_by_custom_type_name_path() const{
            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_CUSTOM ;
            return ss.str();
        }


        int shapp_etcd_module::add_watcher_by_id(watcher_list_callback_t fn) {
            if (!fn) {
                return EN_ATBUS_ERR_PARAMS;
            }

            bool need_setup_callback = watcher_by_id_callbacks_.empty();

            if (need_setup_callback) {
                std::stringstream ss;
                ss << conf_.path_prefix << ETCD_MODULE_BY_ID_DIR;
                // generate watch data
                std::string watch_path = ss.str();

                atframe::component::etcd_watcher::ptr_t p = atframe::component::etcd_watcher::create(etcd_ctx_, watch_path, "+1");
                if (!p) {
                    WLOGERROR("create etcd_watcher by_id failed.");
                    return EN_ATBUS_ERR_MALLOC;
                }

                p->set_conf_request_timeout(conf_.watcher_request_timeout);
                p->set_conf_retry_interval(conf_.watcher_retry_interval);
                etcd_ctx_.add_watcher(p);
                WLOGINFO("create etcd_watcher for by_id index %s success", watch_path.c_str());

                p->set_evt_handle(watcher_callback_list_wrapper_t(*this, watcher_by_id_callbacks_));
            }

            watcher_by_id_callbacks_.push_back(fn);
            return 0;
        }

        int shapp_etcd_module::add_watcher_by_type_id(uint64_t type_id, watcher_one_callback_t fn) {
            if (!fn) {
                return EN_ATBUS_ERR_PARAMS;
            }

            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_ID_DIR << "/" << type_id;
            // generate watch data
            std::string watch_path = ss.str();

            atframe::component::etcd_watcher::ptr_t p = atframe::component::etcd_watcher::create(etcd_ctx_, watch_path, "+1");
            if (!p) {
                WLOGERROR("create etcd_watcher by_type_id failed.");
                return EN_ATBUS_ERR_MALLOC;
            }

            p->set_conf_request_timeout(conf_.watcher_request_timeout);
            p->set_conf_retry_interval(conf_.watcher_retry_interval);
            etcd_ctx_.add_watcher(p);
            WLOGINFO("create etcd_watcher for by_type_id index %s success", watch_path.c_str());

            p->set_evt_handle(watcher_callback_one_wrapper_t(*this, fn));
            return 0;
        }

        int shapp_etcd_module::add_watcher_by_type_name(const std::string &type_name, watcher_one_callback_t fn) {
            if (!fn) {
                return EN_ATBUS_ERR_PARAMS;
            }

            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TYPE_NAME_DIR << "/" << type_name;
            // generate watch data
            std::string watch_path = ss.str();

            atframe::component::etcd_watcher::ptr_t p = atframe::component::etcd_watcher::create(etcd_ctx_, watch_path, "+1");
            if (!p) {
                WLOGERROR("create etcd_watcher by_type_name failed.");
                return EN_ATBUS_ERR_MALLOC;
            }

            p->set_conf_request_timeout(conf_.watcher_request_timeout);
            p->set_conf_retry_interval(conf_.watcher_retry_interval);
            etcd_ctx_.add_watcher(p);
            WLOGINFO("create etcd_watcher for by_type_name index %s success", watch_path.c_str());

            p->set_evt_handle(watcher_callback_one_wrapper_t(*this, fn));
            return 0;
        }

        int shapp_etcd_module::add_watcher_by_name(watcher_list_callback_t fn) {
            if (!fn) {
                return EN_ATBUS_ERR_PARAMS;
            }

            bool need_setup_callback = watcher_by_name_callbacks_.empty();

            if (need_setup_callback) {
                std::stringstream ss;
                ss << conf_.path_prefix << ETCD_MODULE_BY_NAME_DIR;
                // generate watch data
                std::string watch_path = ss.str();

                atframe::component::etcd_watcher::ptr_t p = atframe::component::etcd_watcher::create(etcd_ctx_, watch_path, "+1");
                if (!p) {
                    WLOGERROR("create etcd_watcher by_name failed.");
                    return EN_ATBUS_ERR_MALLOC;
                }

                p->set_conf_request_timeout(conf_.watcher_request_timeout);
                p->set_conf_retry_interval(conf_.watcher_retry_interval);
                etcd_ctx_.add_watcher(p);
                WLOGINFO("create etcd_watcher for by_name index %s success", watch_path.c_str());

                p->set_evt_handle(watcher_callback_list_wrapper_t(*this, watcher_by_name_callbacks_));
            }

            watcher_by_name_callbacks_.push_back(fn);
            return 0;
        }

        int shapp_etcd_module::add_watcher_by_tag(const std::string &tag_name, watcher_one_callback_t fn) {
            if (!fn) {
                return EN_ATBUS_ERR_PARAMS;
            }

            std::stringstream ss;
            ss << conf_.path_prefix << ETCD_MODULE_BY_TAG << "/" << tag_name;
            // generate watch data
            std::string watch_path = ss.str();

            atframe::component::etcd_watcher::ptr_t p = atframe::component::etcd_watcher::create(etcd_ctx_, watch_path, "+1");
            if (!p) {
                WLOGERROR("create etcd_watcher by_tag failed.");
                return EN_ATBUS_ERR_MALLOC;
            }

            etcd_ctx_.add_watcher(p);
            WLOGINFO("create etcd_watcher for by_tag index %s success", watch_path.c_str());

            p->set_evt_handle(watcher_callback_one_wrapper_t(*this, fn));
            return 0;
        }

        shapp_etcd_module::watcher_callback_list_wrapper_t::watcher_callback_list_wrapper_t(shapp_etcd_module &m, std::list<watcher_list_callback_t> &cbks)
                : mod(&m), callbacks(&cbks) {}
        void shapp_etcd_module::watcher_callback_list_wrapper_t::operator()(const ::atframe::component::etcd_response_header &    header,
                                                                      const ::atframe::component::etcd_watcher::response_t &body) {

            if (NULL == callbacks || NULL == mod) {
                return;
            }
            // decode data
            for (size_t i = 0; i < body.events.size(); ++i) {
                const ::atframe::component::etcd_watcher::event_t &evt_data = body.events[i];
                node_info_t                                        node;
                etcd_module_utils::unpack(node, evt_data.kv.key, evt_data.kv.value, true);
                if (node.id == 0){
                    continue;
                }

                if (evt_data.evt_type == ::atframe::component::etcd_watch_event::EN_WEVT_DELETE) {
                    node.action = node_action_t::EN_NAT_DELETE;
                } else {
                    node.action = node_action_t::EN_NAT_PUT;
                }

                watcher_sender_list_t sender(*mod, header, body, evt_data, node);
                for (std::list<watcher_list_callback_t>::iterator iter = callbacks->begin(); iter != callbacks->end(); ++iter) {
                    if (*iter) {
                        (*iter)(std::ref(sender));
                    }
                }
            }
        }


        shapp_etcd_module::watcher_callback_one_wrapper_t::watcher_callback_one_wrapper_t(shapp_etcd_module &m, watcher_one_callback_t cbk) : mod(&m), callback(cbk) {}
        void shapp_etcd_module::watcher_callback_one_wrapper_t::operator()(const ::atframe::component::etcd_response_header &    header,
                                                                     const ::atframe::component::etcd_watcher::response_t &body) {
            if (!callback || NULL == mod) {
                return;
            }

            // decode data
            for (size_t i = 0; i < body.events.size(); ++i) {
                const ::atframe::component::etcd_watcher::event_t &evt_data = body.events[i];
                node_info_t                                        node;
                etcd_module_utils::unpack(node, evt_data.kv.key, evt_data.kv.value, true);
                if (node.id == 0){
                    continue;
                }
                if (evt_data.evt_type == ::atframe::component::etcd_watch_event::EN_WEVT_DELETE) {
                    node.action = node_action_t::EN_NAT_DELETE;
                } else {
                    node.action = node_action_t::EN_NAT_PUT;
                }

                watcher_sender_one_t sender(*mod, header, body, evt_data, node);
                callback(std::ref(sender));
            }
        }

        atframe::component::etcd_keepalive::ptr_t shapp_etcd_module::add_keepalive_actor(std::string &val, const std::string &node_path) {
            atframe::component::etcd_keepalive::ptr_t ret;
            if (val.empty()) {
                node_info_t ni;
                ni.id        = get_app()->get_id();
                ni.name      = get_app()->get_app_name();
                ni.hostname  = ::atbus::node::get_hostname();
                ni.listens   = get_app()->get_bus_node()->get_channels();
                ni.hash_code = get_app()->get_conf().hash_code;
                //ni.type_id   = static_cast<uint64_t>(get_app()->get_type_id());
                ni.type_name = get_app()->get_conf().type_name;
                ni.version   = get_app()->get_conf().engine_version;
                ni.tags = get_app()->get_conf().tags;
                etcd_module_utils::pack(ni, val);
            }
            return add_keepalive_actor2(val, node_path);
        }

        atframe::component::etcd_keepalive::ptr_t shapp_etcd_module::add_keepalive_actor2(const std::string &val, const std::string &node_path){
            atframe::component::etcd_keepalive::ptr_t ret;
            ret = atframe::component::etcd_keepalive::create(etcd_ctx_, node_path);
            if (!ret) {
                WLOGERROR("create etcd_keepalive failed.");
                return ret;
            }

            ret->set_checker(val);
#if defined(UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES) && UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
            ret->set_value(std::move(val));
#else
            ret->set_value(val);
#endif
            if (!etcd_ctx_.add_keepalive(ret)) {
                ret.reset();
            }

            return ret;
        }


    }
}