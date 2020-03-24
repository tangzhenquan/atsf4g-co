#ifndef ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_H
#define ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_H



#include <stddef.h>
#include <stdint.h>

#include "atproxy_build_feature.h"

#define CONFIG_BUS_LISTEN_MAX 4
#define CONFIG_TAGS_MAX 4
#define CONFIG_ETCD_HOST_MAX 10


#ifdef __cplusplus
extern "C" {
#endif

enum log_type {
    LOG_DISABLE = 0,
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_NOTICE,
    LOG_DEBUG,
    LOG_TRACE,


    LOG_MAX = 10000,
};

typedef struct AT_PROXY_API cli_conf {
    uint64_t id;
    const char* bus_listen[CONFIG_BUS_LISTEN_MAX];
    int32_t bus_listen_count;
    const char* type_name;
    const char* name;
    const char* engine_version;
    const char* tags[CONFIG_TAGS_MAX];
    int32_t tags_count;
    const char* father_address;
    int32_t enable_local_discovery_cli;

    const char* etcd_host[CONFIG_ETCD_HOST_MAX];
    int32_t etcd_host_count;
    const char* etcd_authorization;
    log_type log_level;


} cli_conf_t;

typedef void *libatproxy_cli_context;
typedef const void *libatproxy_cli_message;
//conf
AT_PROXY_API void __cdecl libatproxy_cli_init_conf(cli_conf_t & conf );


typedef int32_t (*libatproxy_cli_on_msg_fn_t)(libatproxy_cli_context context, libatproxy_cli_message msg, const void *msg_data, uint64_t msg_len, void *priv_data);
typedef int32_t (*libatproxy_cli_on_send_fail_fn_t)(libatproxy_cli_context context, uint64_t src_pd, uint64_t dst_pd, libatproxy_cli_message msg, void *priv_data);

AT_PROXY_API int32_t __cdecl libatapp_c_send_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz, int32_t require_rsp = 0);
AT_PROXY_API int32_t __cdecl libatapp_c_broadcast_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz, int32_t cross_group = 0);
AT_PROXY_API void __cdecl libatproxy_cli_set_on_msg_fn(libatproxy_cli_context context, libatproxy_cli_on_msg_fn_t fn, void *priv_data);
AT_PROXY_API void __cdecl libatproxy_cli_set_on_send_fail_fn(libatproxy_cli_context context, libatproxy_cli_on_send_fail_fn_t fn, void *priv_data);
AT_PROXY_API int32_t __cdecl libatproxy_cli_send_msg(libatproxy_cli_context context, uint64_t  bus_id, const void *buffer, uint64_t sz, int32_t require_rsp = 0);

AT_PROXY_API libatproxy_cli_context __cdecl libatproxy_cli_create();
AT_PROXY_API void __cdecl libatproxy_cli_destroy(libatproxy_cli_context context);
AT_PROXY_API int32_t __cdecl libatproxy_cli_init(libatproxy_cli_context context, const cli_conf_t& conf );
AT_PROXY_API int32_t __cdecl libatproxy_cli_run(libatproxy_cli_context context);
AT_PROXY_API int32_t __cdecl libatproxy_cli_run_noblock(libatproxy_cli_context context, uint64_t max_event_count = 20000);
AT_PROXY_API int32_t __cdecl libatproxy_cli_stop(libatproxy_cli_context context);

// =========================== message ===========================
AT_PROXY_API uint64_t __cdecl libatproxy_cli_msg_get_src_bus_id(libatproxy_cli_message msg);
AT_PROXY_API uint64_t __cdecl libatproxy_cli_msg_get_forward_from(libatproxy_cli_message msg);
AT_PROXY_API uint64_t __cdecl libatproxy_cli_msg_get_forward_to(libatproxy_cli_message msg);
AT_PROXY_API uint32_t __cdecl libatproxy_cli_msg_get_sequence(libatproxy_cli_message msg);

// ===========================log ===========================
AT_PROXY_API int32_t __cdecl libatproxy_cli_set_log_level(libatproxy_cli_context context, log_type level);


#ifdef __cplusplus
}
#endif


#endif //ATFRAME_SERVICE_ATPROXY_LIBATPROXY_CLI_ATPROXY_CLI_H