#ifndef ATFRAME_SERVICE_COMPONENT_MODULES_SH_MODULE_H
#define ATFRAME_SERVICE_COMPONENT_MODULES_SH_MODULE_H
#pragma once

#include <atframe/atapp.h>

#include "config/compiler_features.h"
#include "etcd_module.h"


namespace atframe {
    namespace component {
        class sh_module :public etcd_module{

        public:
            sh_module();
            virtual ~sh_module();

        public:
            int get_node_by_id(::atapp::app::app_id_t id, node_info_t* info);
            virtual int init() UTIL_CONFIG_OVERRIDE;

            virtual int reload() UTIL_CONFIG_OVERRIDE;

            virtual int stop() UTIL_CONFIG_OVERRIDE;

            virtual int timeout() UTIL_CONFIG_OVERRIDE;

            virtual const char *name() const UTIL_CONFIG_OVERRIDE;

            virtual int tick() UTIL_CONFIG_OVERRIDE;
            int set(node_info_t &node);
            int remove(::atapp::app::app_id_t id);

            int on_connected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int on_disconnected(const ::atapp::app &app, ::atapp::app::app_id_t id);

            int send_data_by_id(::atapp::app::app_id_t id, const void *buffer, size_t s, bool require_rsp = false );

            int send_data_by_name_random(const std::string& name, const void *buffer, size_t s, bool require_rsp = false );
            int send_data_by_name_roundrobin(const std::string& name, const void *buffer, size_t s, bool require_rsp = false );


        private:
            void swap(node_info_t &l, node_info_t &r);
            typedef std::map< ::atapp::app::app_id_t, node_info_t> node_set_t;
            node_set_t node_set_;
            typedef std::map< std::string, std::vector<::atapp::app::app_id_t> > node_name_set_t;
            node_name_set_t node_name_set_;

            typedef std::map< std::string, size_t>  node_name_tick_set_t;
            node_name_tick_set_t node_name_tick_set_;

            int connect_by_id(::atapp::app::app_id_t id);
            int get_sv_by_type_name_random( const std::string& type_name, ::atapp::app::app_id_t *id);
            int get_sv_by_type_name_roundrobin( const std::string& type_name, ::atapp::app::app_id_t *id);
            void on_watcher_notify( etcd_module::watcher_sender_one_t &sender);
        };

    }
}
#endif //ATFRAME_SERVICE_COMPONENT_MODULES_SH_MODULE_H
