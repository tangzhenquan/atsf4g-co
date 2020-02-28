#ifndef ATFRAME_SERVICE_COMPONENT_MODULES_SHAPP_ETCD_MODULE_H
#define ATFRAME_SERVICE_COMPONENT_MODULES_SHAPP_ETCD_MODULE_H

#pragma once

#include <ctime>
#include <list>
#include <std/smart_ptr.h>
#include <string>
#include <vector>

#include <shapp_module_impl.h>

#include <network/http_request.h>
#include <random/random_generator.h>
#include <time/time_utility.h>


#include <etcdcli/etcd_cluster.h>
#include <etcdcli/etcd_keepalive.h>
#include <etcdcli/etcd_watcher.h>


#include "etcd_module_utils.h"

namespace atframe {
    namespace component {
        class shapp_etcd_module: public  ::shapp::module_impl{
        public:
            struct node_list_t {
                std::list<node_info_t> nodes;
            };

            struct conf_t {
                std::string                         path_prefix;
                std::chrono::system_clock::duration etcd_init_timeout;
                std::chrono::system_clock::duration watcher_retry_interval;
                std::chrono::system_clock::duration watcher_request_timeout;

                bool                     report_alive_by_id;
                bool                     report_alive_by_type;
                bool                     report_alive_by_name;
                std::vector<std::string> report_alive_by_tag;
            };

            struct watcher_sender_list_t {
                std::reference_wrapper<shapp_etcd_module>                                          atapp_module;
                std::reference_wrapper<const ::atframe::component::etcd_response_header>     etcd_header;
                std::reference_wrapper<const ::atframe::component::etcd_watcher::response_t> etcd_body;
                std::reference_wrapper<const ::atframe::component::etcd_watcher::event_t>    event;
                std::reference_wrapper<const node_info_t>                                    node;

                inline watcher_sender_list_t(shapp_etcd_module &m, const ::atframe::component::etcd_response_header &h,
                                             const ::atframe::component::etcd_watcher::response_t &b, const ::atframe::component::etcd_watcher::event_t &e,
                                             const node_info_t &n)
                        : atapp_module(std::ref(m)), etcd_header(std::cref(h)), etcd_body(std::cref(b)), event(std::cref(e)), node(std::cref(n)) {}
            };

            struct watcher_sender_one_t {
                std::reference_wrapper<shapp_etcd_module>                                          atapp_module;
                std::reference_wrapper<const ::atframe::component::etcd_response_header>     etcd_header;
                std::reference_wrapper<const ::atframe::component::etcd_watcher::response_t> etcd_body;
                std::reference_wrapper<const ::atframe::component::etcd_watcher::event_t>    event;
                std::reference_wrapper<node_info_t>                                          node;

                inline watcher_sender_one_t(shapp_etcd_module &m, const ::atframe::component::etcd_response_header &h,
                                            const ::atframe::component::etcd_watcher::response_t &b, const ::atframe::component::etcd_watcher::event_t &e,
                                            node_info_t &n)
                        : atapp_module(std::ref(m)), etcd_header(std::cref(h)), etcd_body(std::cref(b)), event(std::cref(e)), node(std::ref(n)) {}
            };

            typedef std::function<void(watcher_sender_list_t &)> watcher_list_callback_t;
            typedef std::function<void(watcher_sender_one_t &)>  watcher_one_callback_t;



        public:
            shapp_etcd_module();
            virtual ~shapp_etcd_module();

        public:
            void reset();

            virtual int init() UTIL_CONFIG_OVERRIDE;

            //virtual int reload() UTIL_CONFIG_OVERRIDE;

            virtual int stop() UTIL_CONFIG_OVERRIDE;

            virtual int timeout() UTIL_CONFIG_OVERRIDE;

            virtual const char *name() const UTIL_CONFIG_OVERRIDE;

            virtual int tick() UTIL_CONFIG_OVERRIDE;

            /*int reg_custom_node(const node_info_t& node_info);*/

            /*int un_reg_custom_node(uint64_t id);*/

            inline void set_conf_etcd_init_timeout(std::chrono::system_clock::duration v) { conf_.etcd_init_timeout = v; }
            inline void set_conf_etcd_init_timeout_sec(time_t v) { set_conf_etcd_init_timeout(std::chrono::seconds(v)); }
            inline const std::chrono::system_clock::duration &get_conf_etcd_init_timeout() const { return conf_.etcd_init_timeout; }

            inline void set_conf_watcher_retry_interval(std::chrono::system_clock::duration v) { conf_.watcher_retry_interval = v; }
            inline void set_conf_watcher_retry_interval_sec(time_t v) { set_conf_etcd_init_timeout(std::chrono::seconds(v)); }
            inline const std::chrono::system_clock::duration &get_conf_watcher_retry_interval() const { return conf_.watcher_retry_interval; }

            inline void set_conf_watcher_request_timeout(std::chrono::system_clock::duration v) { conf_.watcher_request_timeout = v; }
            inline void set_conf_watcher_request_timeout_sec(time_t v) { set_conf_etcd_init_timeout(std::chrono::seconds(v)); }
            inline const std::chrono::system_clock::duration &get_conf_watcher_request_timeout() const { return conf_.watcher_request_timeout; }

            inline void               set_conf_path_prefix(const std::string &path_prefix) { conf_.path_prefix = path_prefix; }
            inline const std::string &get_conf_path_prefix() const { return conf_.path_prefix; }

            inline void               set_conf_report_alive_by_id(bool report_alive_by_id) { conf_.report_alive_by_id = report_alive_by_id; }
            inline bool  get_conf_report_alive_by_id() const { return conf_.report_alive_by_id; }

            inline void               set_conf_report_alive_by_type(bool report_alive_by_type) { conf_.report_alive_by_type = report_alive_by_type; }
            inline bool  get_conf_report_alive_by_type() const { return conf_.report_alive_by_type; }


            std::string get_by_id_path() const;
            std::string get_by_type_id_path() const;
            std::string get_by_type_name_path() const;
            std::string get_by_name_path() const;
            std::string get_by_tag_path(const std::string &tag_name) const;

            std::string get_by_custom_type_name_path() const ;


            std::string get_by_id_watcher_path() const;
            std::string get_by_type_id_watcher_path(uint64_t type_id) const;
            std::string get_by_type_name_watcher_path(const std::string &type_name) const;
            std::string get_by_name_watcher_path() const;
            std::string get_by_tag_watcher_path(const std::string &tag_name) const;

            int add_watcher_by_id(watcher_list_callback_t fn);
            int add_watcher_by_type_id(uint64_t type_id, watcher_one_callback_t fn);
            int add_watcher_by_type_name(const std::string &type_name, watcher_one_callback_t fn);
            int add_watcher_by_name(watcher_list_callback_t fn);
            int add_watcher_by_tag(const std::string &tag_name, watcher_one_callback_t fn);


            inline const ::atframe::component::etcd_cluster &get_raw_etcd_ctx() const { return etcd_ctx_; }
            inline ::atframe::component::etcd_cluster &      get_raw_etcd_ctx() { return etcd_ctx_; }

        private:
            //static bool unpack(node_info_t &out, const std::string &path, const std::string &json, bool reset_data);
            //static void pack(const node_info_t &out, std::string &json);

            static int http_callback_on_etcd_closed(util::network::http_request &req);

            struct watcher_callback_list_wrapper_t {
                shapp_etcd_module *                       mod;
                std::list<watcher_list_callback_t> *callbacks;

                watcher_callback_list_wrapper_t(shapp_etcd_module &m, std::list<watcher_list_callback_t> &cbks);
                void operator()(const ::atframe::component::etcd_response_header &header, const ::atframe::component::etcd_watcher::response_t &evt_data);
            };

            struct watcher_callback_one_wrapper_t {
                shapp_etcd_module *          mod;
                watcher_one_callback_t callback;

                watcher_callback_one_wrapper_t(shapp_etcd_module &m, watcher_one_callback_t cbk);
                void operator()(const ::atframe::component::etcd_response_header &header, const ::atframe::component::etcd_watcher::response_t &evt_data);
            };

            atframe::component::etcd_keepalive::ptr_t add_keepalive_actor(std::string &val, const std::string &node_path);
            atframe::component::etcd_keepalive::ptr_t add_keepalive_actor2(const std::string &val, const std::string &node_path);


        private:
            conf_t                                         conf_;
            util::network::http_request::curl_m_bind_ptr_t curl_multi_;
            util::network::http_request::ptr_t             cleanup_request_;
            ::atframe::component::etcd_cluster             etcd_ctx_;
            std::list<watcher_list_callback_t>             watcher_by_id_callbacks_;
            std::list<watcher_list_callback_t>             watcher_by_name_callbacks_;
        };


    } // namespace component
} // namespace atframe

#endif //ATFRAME_SERVICE_COMPONENT_MODULES_
