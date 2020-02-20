

#ifndef ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_MODULE_H
#define ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_MODULE_H


#pragma once


#include <shapp.h>

#include "shapp_module_impl.h"
#include "atproxy_cli.h"
#include "libatproxy_proto.h"

namespace atframe {
    namespace proxy {

        class atproxy_cli_module : public ::shapp::module_impl {


        public:
            virtual int init() UTIL_CONFIG_OVERRIDE;

            virtual int stop() UTIL_CONFIG_OVERRIDE;

            virtual int timeout() UTIL_CONFIG_OVERRIDE;

            virtual const char *name() const UTIL_CONFIG_OVERRIDE;

            virtual int tick() UTIL_CONFIG_OVERRIDE;


            int on_connected( ::shapp::app &app, atbus::endpoint & ,  int status);

            int on_disconnected( ::shapp::app &, atbus::endpoint &, int);

            int on_msg(::shapp::app & app, const shapp::app::msg_t &recv_msg, const void *data, size_t l);

            int on_available( ::shapp::app &);

            int on_send_fail(::shapp::app & app, ::shapp::app::app_id_t src_pd, ::shapp::app::app_id_t dst_pd, const shapp::app::msg_t &m);

        public:
            libatproxy_cli_on_msg_fn_t on_msg_;
            void *on_msg_priv_data_;
            libatproxy_cli_on_send_fail_fn_t on_send_fail_;
            void *on_send_fail_priv_data_;

        private:
            int post_ss_msg(::atbus::node::bus_id_t tid, int type,  ss_msg &msg);

        };

    }
}



#endif //ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_MODULE_H
