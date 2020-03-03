
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <std/ref.h>
#include <vector>

#include <uv.h>

#include <atframe/atapp.h>
#include <common/file_system.h>
#include <time/time_utility.h>
#include <libatbus_protocol.h>
#include "protocols/libatproxy_proto.h"
#include "atproxy_manager.h"
#include "node_proxy.h"

static int app_handle_on_send_fail(atapp::app &, atapp::app::app_id_t src_pd, atapp::app::app_id_t dst_pd, const atbus::protocol::msg &) {
    WLOGERROR("send data from 0x%llx to 0x%llx failed", static_cast<unsigned long long>(src_pd), static_cast<unsigned long long>(dst_pd));
    return 0;
}

struct app_handle_on_connected {
    std::reference_wrapper<atframe::proxy::atproxy_manager> atproxy_mgr_module;
    app_handle_on_connected(atframe::proxy::atproxy_manager &mod) : atproxy_mgr_module(mod) {}

    int operator()(atapp::app &app, atbus::endpoint &ep, int status) {
        WLOGINFO("node 0x%llx connected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);

        atproxy_mgr_module.get().on_connected(app, ep.get_id());
        return 0;
    }
};

struct app_handle_on_disconnected {
    std::reference_wrapper<atframe::proxy::atproxy_manager> atproxy_mgr_module;
    app_handle_on_disconnected(atframe::proxy::atproxy_manager &mod) : atproxy_mgr_module(mod) {}

    int operator()(atapp::app &app, atbus::endpoint &ep, int status) {
        WLOGINFO("node 0x%llx disconnected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);

        atproxy_mgr_module.get().on_disconnected(app, ep.get_id());
        return 0;
    }
};
struct app_handle_on_msg {
    std::reference_wrapper<atframe::proxy::atproxy_manager> atproxy_mgr_module;
    std::reference_wrapper<atframe::proxy::node_proxy> node_proxy_module;
    app_handle_on_msg(atframe::proxy::atproxy_manager &mod, atframe::proxy::node_proxy &node_proxy_mod) : atproxy_mgr_module(mod), node_proxy_module(node_proxy_mod) {}
    int operator()(atapp::app &app, const atapp::app::msg_t &msg, const void * buffer, size_t l) {
        WLOGINFO("receive a message(from 0x%llx, type=%d) ", static_cast<unsigned long long>(msg.head.src_bus_id), msg.head.type);
        return  node_proxy_module.get().on_msg(app, msg, buffer, l);

    }

};

struct app_handle_on_custom_route {
    std::reference_wrapper<atframe::proxy::node_proxy> node_proxy_module;
    app_handle_on_custom_route(atframe::proxy::node_proxy &node_proxy_mod) : node_proxy_module(node_proxy_mod) {}
    int operator()(atapp::app & app ,  atapp::app::app_id_t src_id , const atbus::protocol::custom_route_data &data,  std::vector<uint64_t >& bus_ids ) {

        /*std::stringstream ss ;
        ss << data;
        bus_ids.push_back(123);
        WLOGINFO("receive a custom_route_data:%s ", ss.str().c_str());*/
        return node_proxy_module.get().on_custom_route(app,src_id,  data, bus_ids);
    }
};


int main(int argc, char *argv[]) {
    atapp::app                                       app;
    std::shared_ptr<atframe::component::etcd_module> etcd_mod = std::make_shared<atframe::component::etcd_module>();
    if (!etcd_mod) {
        fprintf(stderr, "create etcd module failed\n");
        return -1;
    }

    std::shared_ptr<atframe::proxy::atproxy_manager> proxy_mgr_mod = std::make_shared<atframe::proxy::atproxy_manager>(etcd_mod);
    if (!proxy_mgr_mod) {
        fprintf(stderr, "create atproxy manager module failed\n");
        return -1;
    }

    std::shared_ptr<atframe::proxy::node_proxy> node_proxy_mod = std::make_shared<atframe::proxy::node_proxy>(etcd_mod);
    if (!proxy_mgr_mod) {
        fprintf(stderr, "create node_proxy_mod failed\n");
        return -1;
    }

    // project directory
    {
        std::string proj_dir;
        util::file_system::dirname(__FILE__, 0, proj_dir, 4);
        util::log::log_formatter::set_project_directory(proj_dir.c_str(), proj_dir.size());
    }

    // setup module
    app.add_module(etcd_mod);
    app.add_module(proxy_mgr_mod);
    app.add_module(node_proxy_mod);

    // setup message handle
    app.set_evt_on_send_fail(app_handle_on_send_fail);
    app.set_evt_on_app_connected(app_handle_on_connected(*proxy_mgr_mod));
    app.set_evt_on_app_disconnected(app_handle_on_disconnected(*proxy_mgr_mod));
    app.set_evt_on_recv_msg(app_handle_on_msg(*proxy_mgr_mod, *node_proxy_mod));
    app.set_evt_on_on_custom_route(app_handle_on_custom_route(*node_proxy_mod));

    // run
    return app.run(uv_default_loop(), argc, (const char **)argv, NULL);
}
