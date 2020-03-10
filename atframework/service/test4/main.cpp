//
// Created by tom on 2020/1/10.
//

#include "atproxy_cli.h"
#include <signal.h>
#include <stdio.h>
#include <string>


libatproxy_cli_context  g_ctx = NULL;

void stop_context(int  ){
    if (g_ctx) {
        libatproxy_cli_stop(g_ctx);
    }
}

static int32_t on_msg(libatproxy_cli_context , libatproxy_cli_message msg, const void *msg_data, uint64_t msg_len, void *){
    std::string data;
    data.assign(reinterpret_cast<const char *>(msg_data), msg_len);
    uint64_t src_bus_id =  libatproxy_cli_msg_get_src_bus_id(msg);
    uint32_t sequence = libatproxy_cli_msg_get_sequence(msg);
    uint64_t from = libatproxy_cli_msg_get_forward_from(msg);
    printf("on_msg src_bus_id:0x%llx sequence:%u from:0x%llx msg_data:%s len:%ld\n", static_cast<unsigned long long>(src_bus_id), sequence,
            static_cast<unsigned long long>(from), data.c_str(), msg_len);
    return 0;
}

int main(int , char *[]) {
    //cli_conf_t *ss = NULL;
    //ss->name = "sdsadas";
    //conf
    cli_conf_t conf;
    libatproxy_cli_init_conf(conf);
    conf.bus_listen[0] = "ipv4://0.0.0.0:10501";
    //conf.bus_listen[1] = "shm://0x1010150501";
    conf.bus_listen_count = 1;
    conf.engine_version = "1.2.3";
    conf.father_address =  "ipv4://192.168.2.71:7777";
    conf.id =  0x100100010002;
    conf.name = "name";
    conf.type_name = "game_server";

    conf.tags[0] = "sss";
    conf.tags[1] = "22222";
    conf.tags_count = 2;

    conf.enable_local_discovery_cli = 1;
    conf.etcd_host_count = 1;
    conf.etcd_host[0] = "http://192.168.2.71:2379";

    //create
    libatproxy_cli_context  ctx =  libatproxy_cli_create();
    g_ctx = ctx;
    libatproxy_cli_init(ctx, conf);


    signal(SIGTERM, stop_context);
    signal(SIGINT, stop_context);

#ifndef WIN32
    signal(SIGSTOP, stop_context);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);  // lost parent process
    signal(SIGPIPE, SIG_IGN); // close stdin, stdout or stderr
    signal(SIGTSTP, SIG_IGN); // close tty
    signal(SIGTTIN, SIG_IGN); // tty input
    signal(SIGTTOU, SIG_IGN); // tty output
#endif

    libatproxy_cli_set_on_msg_fn(ctx, on_msg, NULL);

    libatproxy_cli_run(ctx);


    //destory
    libatproxy_cli_destroy(ctx);

}