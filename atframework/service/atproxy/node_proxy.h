
#ifndef ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
#define ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
#pragma once

#include <atframe/atapp.h>
#include <atframe/atapp_module_impl.h>
#include <modules/etcd_module.h>


namespace atbus{
    namespace protocol{
        struct custom_route_data;
    }
}

namespace atframe {
    namespace proxy {

        class node_proxy : public ::atapp::module_impl {

        public:
            typedef std::shared_ptr<atframe::component::etcd_module> etcd_mod_ptr;
            typedef atframe::component::node_action_t node_action_t;
            typedef atframe::component::node_info_t node_info_t;
            struct conf_t {
                std::vector<std::string> watch_paths;
            };


        public:
            explicit node_proxy(etcd_mod_ptr etcd_mod);

            virtual int init() UTIL_CONFIG_OVERRIDE;

            virtual int tick() UTIL_CONFIG_OVERRIDE;

            virtual const char *name() const UTIL_CONFIG_OVERRIDE;

            virtual int reload() UTIL_CONFIG_OVERRIDE;

            int on_connected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int on_disconnected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int on_custom_route(const atapp::app &app,::atapp::app::app_id_t src_id, const atbus::protocol::custom_route_data &data,  std::vector<uint64_t >& bus_ids );

            int on_msg(const atapp::app & app, const atapp::app::msg_t &recv_msg, const void *data, size_t l);

            int set(node_info_t &node);

            int remove(::atapp::app::app_id_t id);

        private:
            void on_watcher_notify(atframe::component::etcd_module::watcher_sender_one_t &sender);
            void swap(node_info_t &l, node_info_t &r);

            typedef std::map< ::atapp::app::app_id_t, node_info_t> node_set_t;
            node_set_t node_set_;
            typedef std::map< std::string, std::vector<::atapp::app::app_id_t> > node_name_set_t;
            node_name_set_t node_name_set_;

            typedef std::map< std::string, size_t>  node_name_tick_set_t;
            node_name_tick_set_t node_name_tick_set_;

            int get_sv_by_type_name_random( const std::string& type_name, const std::string& src_type_name, ::atapp::app::app_id_t src_id,  ::atapp::app::app_id_t *id);
            int get_sv_by_type_name_roundrobin( const std::string& type_name, const std::string& src_type_name, ::atapp::app::app_id_t src_id, ::atapp::app::app_id_t *id);
            int get_svs_by_type_name_all( const std::string& type_name, const std::string& src_type_name, ::atapp::app::app_id_t src_id, std::vector<uint64_t>& bus_ids);

        private:
            etcd_mod_ptr                                           binded_etcd_mod_;
            conf_t                                                 conf_;
        };

    }
}



#endif //ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
