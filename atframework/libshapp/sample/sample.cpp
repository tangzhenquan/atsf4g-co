#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>



#include <common/file_system.h>
#include <libatbus.h>
#include <libatbus_protocol.h>
#include <time/time_utility.h>
#include <uv.h>

#include "shapp.h"
#include "shapp_log.h"



class echo_module : public shapp::module_impl {
public:
    virtual int init() {
        WLOGINFO("echo module init");
        return 0;
    };


    virtual int stop() {
        WLOGINFO("echo module stop");
        return 0;
    }

    virtual int timeout() {
        WLOGINFO("echo module timeout");
        return 0;
    }

    virtual const char *name() const { return "echo_module"; }

    virtual int tick() {
        time_t cur_print = util::time::time_utility::get_now() / 20;
        static time_t print_per_sec = cur_print;
        if (print_per_sec != cur_print) {
            WLOGINFO("echo module tick");
            print_per_sec = cur_print;
        }

        return 0;
    }
};




static int app_handle_on_msg(shapp::app &app, const shapp::app::msg_t &msg, const void *buffer, size_t len) {
    std::string data;
    data.assign(reinterpret_cast<const char *>(buffer), len);
    WLOGINFO("receive a message(from 0x%llx, type=%d) %s", static_cast<unsigned long long>(msg.head.src_bus_id), msg.head.type, data.c_str());

    if (NULL != msg.body.forward && 0 != msg.body.forward->from) {
        return app.get_bus_node()->send_data(msg.body.forward->from, msg.head.type, buffer, len);
    }

    return 0;
}

static int app_handle_on_send_fail(shapp::app &, shapp::app::app_id_t src_pd, shapp::app::app_id_t dst_pd, const atbus::protocol::msg &) {
    WLOGERROR("send data from 0x%llx to 0x%llx failed", static_cast<unsigned long long>(src_pd), static_cast<unsigned long long>(dst_pd));
    return 0;
}

static int app_handle_on_connected(shapp::app &, atbus::endpoint &ep, int status) {
    WLOGINFO("app 0x%llx connected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return 0;
}

static int app_handle_on_disconnected(shapp::app &, atbus::endpoint &ep, int status) {
    WLOGINFO("app 0x%llx disconnected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return 0;
}


int main(int , char *[]) {
    util::log::log_adaptor::get_instance().init_log();

    shapp::app app;


    // setup module
    app.add_module(std::make_shared<echo_module>());

    // setup handl
    app.set_evt_on_recv_msg(app_handle_on_msg);
    app.set_evt_on_send_fail(app_handle_on_send_fail);
    app.set_evt_on_app_connected(app_handle_on_connected);
    app.set_evt_on_app_disconnected(app_handle_on_disconnected);

    shapp::app_conf conf;

    conf.tags = {"hh", "tt"};
    conf.name = "name";
    conf.type_name = "hhh";
    conf.id = 0x23401;
    conf.engine_version = "1.1.5";
    conf.bus_listen = {"ipv4://192.168.2.88:40401"};
    conf.stop_timeout = 10000;
    conf.tick_interval = 12;
    atbus::node::default_conf(&conf.bus_conf);
    conf.bus_conf.father_address = "ipv4://192.168.2.88:20101";


    // run
    return app.run(uv_default_loop(), conf);

}












