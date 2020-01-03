#include <uv.h>




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
#include <libatbus_protocol.h>
#include <time/time_utility.h>
#include <modules/sh_module.h>
#include <config/atframe_service_types.h>



static int app_handle_on_send_fail(atapp::app &, atapp::app::app_id_t src_pd, atapp::app::app_id_t dst_pd, const atbus::protocol::msg &) {
    WLOGERROR("send data from 0x%llx to 0x%llx failed", static_cast<unsigned long long>(src_pd), static_cast<unsigned long long>(dst_pd));
    fprintf(stderr, "app_handle_on_send_fail 0x%llx to 0x%llx failed \n", static_cast<unsigned long long>(src_pd), static_cast<unsigned long long>(dst_pd));
    return 0;
}

static  int app_handle_on_connected(atapp::app &, atbus::endpoint &ep, int status){
    WLOGINFO("node 0x%llx connected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return  0;
}

static  int app_handle_on_disconnected(atapp::app &, atbus::endpoint &ep, int status){
    WLOGINFO("node 0x%llx disconnected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
    return 0;
}

static  int app_handle_on_recv_msg(atapp::app & , const atapp::app::msg_t & msg, const void *buffer , size_t len){
    WLOGINFO("node recv,type: %d status: %zu", msg.head.type, len);
    std::string data;
    data.assign(reinterpret_cast<const char *>(buffer), len);
    fprintf(stderr, "app_handle_on_recv_msg type: %d  data:%s \n", msg.head.type, data.c_str() );
    return 0;
}

struct app_command_handler_connect {
    atapp::app *      app_;
    std::reference_wrapper<atframe::component::sh_module> sh_mod_;
    app_command_handler_connect(atapp::app *app, atframe::component::sh_module & sh_mod) : app_(app), sh_mod_(sh_mod) {}
    int operator()(util::cli::callback_param params) {
        fprintf(stderr, "sadasdasdasd222%s\n", params[0]->to_cpp_string().c_str() );

        if (params.get_params_number() <= 0) {
            WLOGERROR("connect command must require type name");
            return 0;
        }

        /*::atframe::component::service_type::EN_ATST_CUSTOM_START
        ::atframe::gw::ss_msg msg;
        msg.init(ATFRAME_GW_CMD_SESSION_KICKOFF, sess_id);*/
        /*std::stringstream ss;
        msgpack::pack(ss, msg);
        std::string packed_buffer;
        ss.str().swap(packed_buffer);*/
        std::string hh("hahatest");
        int ret =  sh_mod_.get().send_data_by_name_roundrobin(params[0]->to_cpp_string(),  hh.c_str(), hh.length());
        WLOGINFO("send_data_by_name_roundrobin ret:%d", ret);
        fprintf(stderr, "send_data_by_name_random:%d\n", ret);
        return ret;
    }
};

struct testdata{
    atframe::component::sh_module* sh_mod_;
    atapp::app *app_;
    testdata(atframe::component::sh_module* sh_mod, atapp::app *app):sh_mod_(sh_mod),app_(app){}
};

static void init_timer_timeout_callback(uv_timer_t *handle) {
    assert(handle);
    assert(handle->data);
    assert(handle->loop);
    testdata* data = reinterpret_cast<testdata *>(handle->data);
    fprintf(stderr, "init_timer_timeout_callback\n");
    if (data->app_->get_type_name() == "test"){
        fprintf(stderr, "send \n");
        static int i = 0;
        char buff[1024] = {0};
        std::snprintf(buff, sizeof(buff), "test_data:%d", i++);
        int ret =  data->sh_mod_->send_data_by_name_roundrobin("test2",buff, sizeof(buff));
        WLOGINFO("send_data_by_name_roundrobin ret:%d", ret);
        fprintf(stderr, "send_data_by_name_roundrobin data:%s ret:%d\n",buff, ret);
        //uv_stop(handle->loop);
    }

}


int main(int argc, char *argv[]) {
    atapp::app                                       app;
    std::shared_ptr<atframe::component::sh_module> sh_mod = std::make_shared<atframe::component::sh_module>();
    if (!sh_mod) {
        fprintf(stderr, "create sh_mod module failed\n");
        return -1;
    }
    util::cli::cmd_option_ci::ptr_type cmgr = app.get_command_manager();
    cmgr->bind_cmd("connect", app_command_handler_connect(&app, *sh_mod))->set_help_msg("connect 123123");

    // project directory
    {
        std::string proj_dir;
        util::file_system::dirname(__FILE__, 0, proj_dir, 4);
        util::log::log_formatter::set_project_directory(proj_dir.c_str(), proj_dir.size());
    }

    // setup module
    app.add_module(sh_mod);
    // setup message handle
    app.set_evt_on_send_fail(app_handle_on_send_fail);
    app.set_evt_on_app_connected(app_handle_on_connected);
    app.set_evt_on_app_disconnected(app_handle_on_disconnected);
    app.set_evt_on_recv_msg(app_handle_on_recv_msg);

    uv_timer_t timeout_timer;
    uv_timer_init(uv_default_loop(), &timeout_timer);
    testdata* data = new testdata(sh_mod.get(), &app);
    timeout_timer.data = data;

    uint64_t timeout_ms = 1000*5;
    uv_timer_start(&timeout_timer, init_timer_timeout_callback, timeout_ms, 5000);



    // run
    return app.run(uv_default_loop(), argc, (const char **)argv, NULL);
}
