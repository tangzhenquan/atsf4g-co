//
// Created by tom on 2020/1/10.
//

#include "atproxy_cli.h"
#include <signal.h>


libatproxy_cli_context  g_ctx = NULL;

void stop_context(int  ){
    if (g_ctx) {
        libatproxy_cli_stop(g_ctx);
    }
}


int main(int , char *[]) {
    //cli_conf_t *ss = NULL;
    //ss->name = "sdsadas";
    //conf
    cli_conf_t conf;
    libatproxy_cli_init_conf(conf);
    conf.bus_listen[0] = "ipv4://192.168.2.71:50501";
    conf.bus_listen[1] = "shm://0x1010150501";
    conf.bus_listen_count = 2;
    conf.engine_version = "1.2.3";
    conf.father_address =  "ipv4://192.168.2.71:10101";
    conf.id =  0x15050;
    conf.name = "name";
    conf.type_name = "type_name";

    conf.tags[0] = "sss";
    conf.tags[1] = "22222";
    conf.tags_count = 2;


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

    //loop
    libatproxy_cli_run(ctx);

    //destory
    libatproxy_cli_destroy(ctx);

}