
#ifndef ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
#define ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
#pragma once

#include <atframe/atapp.h>
#include <atframe/atapp_module_impl.h>
#include <modules/etcd_module.h>


namespace atframe {
    namespace proxy {

        class node_proxy : public ::atapp::module_impl {

        public:
            typedef std::shared_ptr<atframe::component::etcd_module> etcd_mod_ptr;

        public:
            explicit node_proxy(etcd_mod_ptr etcd_mod);

            virtual int init() UTIL_CONFIG_OVERRIDE;

            virtual int tick() UTIL_CONFIG_OVERRIDE;

            virtual const char *name() const UTIL_CONFIG_OVERRIDE;

            int on_connected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int on_disconnected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int on_custom_route(const atapp::app &app, const atbus::protocol::custom_route_data &data,  std::vector<uint64_t >& bus_ids );

            int on_msg(const atapp::app & app, const atapp::app::msg_t &recv_msg, const void *data, size_t l);

        private:
            etcd_mod_ptr                                           binded_etcd_mod_;
        };

    }
}



#endif //ATFRAME_SERVICE_ATPROXY_NODE_PROXY_H
