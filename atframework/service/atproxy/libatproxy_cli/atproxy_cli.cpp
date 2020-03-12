
#include <libatbus.h>
#include <libatbus_protocol.h>
#include <shapp.h>
#include <shapp_log.h>

//#include <modules/shapp_etcd_module.h>
#include "atproxy_cli.h"
#include "atproxy_cli_module.h"


#define  DEFAULT_STOP_TIMEOUT 10000
#define  DEFAULT_TICK_INTERVAL 16
#define  SHAPP_CONTEXT(x) ((::shapp::app *)(x))

#define  SHAPP_CONTEXT_IS_NULL(x) (NULL == (x))
#define  IS_EMPTY(x) ((!x || !x[0]))

#define SHAPP_MESSAGE(x) ((const ::shapp::app::msg_t *)(x))
#define SHAPP_MESSAGE_IS_NULL(x) (NULL == (x))

#define DEFAULT_ETCD_PATH_PREFIX  "/atapp/game_services/"
#define DEFAULT_ETCD_REQEST_TIMEOUT_SECS  15
#define DEFAULT_ETCD_KEEPALIVE_TIMEOUT_SECS 31
#define DEFAULT_ETCD_KEEPALIVE_TTL_SECS 10
#define DEFAULT_ETCD_INIT_TIMEOUT_TTL_SECS 5


static  atframe::proxy::atproxy_cli_module *g_cli_module = NULL;

namespace detail {
    struct app_handle_on_available {
        std::reference_wrapper<atframe::proxy::atproxy_cli_module> atproxy_cli_module;
        explicit  app_handle_on_available(atframe::proxy::atproxy_cli_module &mod) : atproxy_cli_module(mod) {}

        int operator()(shapp::app & app,  int ) {
            return atproxy_cli_module.get().on_available(app);

        }
    };
}


UTIL_SYMBOL_EXPORT void __cdecl libatproxy_cli_init_conf(cli_conf_t & conf ){
    conf.father_address = NULL;
    conf.tags_count = 0;
    conf.engine_version = NULL;
    conf.bus_listen_count = 0;
    conf.id = 0;
    conf.type_name = NULL;
    conf.name = NULL;
    conf.enable_local_discovery_cli = 0;
    conf.etcd_host_count = 0;
    conf.etcd_authorization = NULL;
    conf.log_level                  = LOG_MAX;


}

UTIL_SYMBOL_EXPORT void __cdecl libatproxy_cli_set_on_msg_fn(libatproxy_cli_context context, libatproxy_cli_on_msg_fn_t fn, void *priv_data){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return;
    }
    if (g_cli_module){
        g_cli_module->on_msg_ = fn;
        g_cli_module->on_msg_priv_data_ = priv_data;
    }
}

UTIL_SYMBOL_EXPORT void __cdecl libatproxy_cli_set_on_send_fail_fn(libatproxy_cli_context context, libatproxy_cli_on_send_fail_fn_t fn, void *priv_data){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return;
    }
    if (g_cli_module){
        g_cli_module->on_send_fail_ = fn;
        g_cli_module->on_send_fail_priv_data_ = priv_data;
    }
}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_send_msg(libatproxy_cli_context context, uint64_t  bus_id, const void *buffer, uint64_t sz, int32_t require_rsp ){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    shapp::app *app =  SHAPP_CONTEXT(context);

    if (app->get_bus_node()){
        app->get_bus_node()->send_data(bus_id, 0, buffer, static_cast<size_t>(sz), require_rsp);
    }
    return EN_ATBUS_ERR_SUCCESS;


}


static int32_t send_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz, int32_t require_rsp,
                                     atbus::protocol::custom_route_data::custom_route_type_t custom_route_type){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    shapp::app *app =  SHAPP_CONTEXT(context);

    if (app->get_bus_node()){
        const atbus::endpoint* parent_ep =  app->get_bus_node()->get_parent_endpoint();
        if (NULL != parent_ep){
            std::shared_ptr<atbus::protocol::custom_route_data> custom_route_data = std::make_shared<atbus::protocol::custom_route_data>();
            custom_route_data->type_name = type_name;
            custom_route_data->src_type_name = app->get_conf().type_name;
            custom_route_data->custom_route_type = custom_route_type;
            return    app->get_bus_node()->send_data(parent_ep->get_id(), 0, buffer, static_cast<size_t>(sz), require_rsp, custom_route_data);
        } else{
            return  shapp::EN_SHAPP_ERR_NO_PARENT;
        }
    }
    return EN_ATBUS_ERR_SUCCESS;



}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatapp_c_send_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz, int32_t require_rsp){
    return  send_msg_by_type_name(context, type_name ,buffer, sz, require_rsp, atbus::protocol::custom_route_data::CUSTOM_ROUTE_UNICAST);
}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatapp_c_broadcast_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz){
    return  send_msg_by_type_name(context, type_name ,buffer, sz,  0, atbus::protocol::custom_route_data::CUSTOM_ROUTE_BROADCAST2);
}



UTIL_SYMBOL_EXPORT libatproxy_cli_context __cdecl libatproxy_cli_create() {
    libatproxy_cli_context ret;
    assert(sizeof(void *) == sizeof(libatproxy_cli_context));
    shapp::app *res = new (std::nothrow) shapp::app();
    ret = res;
    return ret;
}




UTIL_SYMBOL_EXPORT void __cdecl libatproxy_cli_destroy(libatproxy_cli_context context){
    delete SHAPP_CONTEXT(context);
}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_run(libatproxy_cli_context context){
    return SHAPP_CONTEXT(context)->run();
}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_init(libatproxy_cli_context context, const cli_conf_t& conf ){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    shapp::app *app =  SHAPP_CONTEXT(context);

    int ret =   util::log::log_adaptor::get_instance().init_log();
    if (ret != shapp::EN_SHAPP_ERR_SUCCESS){
        return ret;
    }
    util::log::log_adaptor::get_instance().set_log_level(static_cast<util::log::log_type_t>(conf.log_level));
    if (conf.id == 0 || IS_EMPTY( conf.father_address) || IS_EMPTY(conf.type_name) || IS_EMPTY(conf.engine_version))  return  shapp::EN_SHAPP_ERR_CONFIG;


    WLOGINFO("id: 0x%llx engine_version:%s bus_listen_count:%d tags_count:%d father_address:%s", static_cast<unsigned  long long>(conf.id), conf.engine_version, conf.bus_listen_count, conf.tags_count
    , conf.father_address);

    shapp::app_conf app_conf;
    app_conf.id = conf.id;
    app_conf.engine_version = conf.engine_version;
    app_conf.stop_timeout = DEFAULT_STOP_TIMEOUT;
    app_conf.tick_interval = DEFAULT_TICK_INTERVAL;
    if (conf.bus_listen_count > CONFIG_BUS_LISTEN_MAX){
        return  shapp::EN_SHAPP_ERR_CONFIG;
    }
    if ( conf.bus_listen_count > 0){
        app_conf.bus_listen.resize(conf.bus_listen_count);
        for(int i = 0 ; i< conf.bus_listen_count; i++){
            app_conf.bus_listen[i] = conf.bus_listen[i];
        }
    }

    if (conf.tags_count > CONFIG_TAGS_MAX){
        return  shapp::EN_SHAPP_ERR_CONFIG;
    }
    if (conf.tags_count  > 0){
        app_conf.tags.resize(conf.tags_count);
        for(int i = 0 ; i< conf.tags_count; i++){
            app_conf.tags[i] =   conf.tags[i];
        }
    }







    atbus::node::default_conf(&app_conf.bus_conf);
    app_conf.bus_conf.father_address = conf.father_address;

    app_conf.type_name = conf.type_name;
    app_conf.name = conf.name;
    app_conf.bus_conf.type_name = conf.type_name;
    app_conf.bus_conf.tags = app_conf.tags;



    if (conf.enable_local_discovery_cli > 0){
        if (conf.etcd_host_count == 0||conf.etcd_host_count > CONFIG_ETCD_HOST_MAX){
            return  shapp::EN_SHAPP_ERR_CONFIG;
        }
        /*std::shared_ptr<atframe::component::shapp_etcd_module> shapp_etcd_mod =  std::make_shared<atframe::component::shapp_etcd_module>();

        std::vector<std::string> etcd_host(conf.etcd_host_count);
        for(int i = 0 ; i< conf.etcd_host_count; i++){
            etcd_host[i] =   conf.etcd_host[i];
        }
        shapp_etcd_mod->get_raw_etcd_ctx().set_conf_hosts(etcd_host);

        if(conf.etcd_authorization != NULL){
            shapp_etcd_mod->get_raw_etcd_ctx().set_conf_authorization(conf.etcd_authorization);
        }
        shapp_etcd_mod->set_conf_path_prefix(DEFAULT_ETCD_PATH_PREFIX);
        shapp_etcd_mod->set_conf_etcd_init_timeout(std::chrono::seconds(DEFAULT_ETCD_INIT_TIMEOUT_TTL_SECS));
        shapp_etcd_mod->set_conf_report_alive_by_type(true);

        shapp_etcd_mod->get_raw_etcd_ctx().set_conf_http_timeout(std::chrono::seconds(DEFAULT_ETCD_REQEST_TIMEOUT_SECS));
        shapp_etcd_mod->get_raw_etcd_ctx().set_conf_keepalive_timeout(std::chrono::seconds(DEFAULT_ETCD_KEEPALIVE_TIMEOUT_SECS));
        shapp_etcd_mod->get_raw_etcd_ctx().set_conf_keepalive_interval(std::chrono::seconds(DEFAULT_ETCD_KEEPALIVE_TTL_SECS));
        app->add_module(shapp_etcd_mod);*/
    }


    std::shared_ptr<atframe::proxy::atproxy_cli_module> cli_mod =  std::make_shared<atframe::proxy::atproxy_cli_module>();
    g_cli_module = cli_mod.get();
    app->add_module(cli_mod);
    app->set_evt_on_app_connected(std::bind<int>(&atframe::proxy::atproxy_cli_module::on_connected, g_cli_module,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ));
    app->set_evt_on_recv_msg(std::bind<int>(&atframe::proxy::atproxy_cli_module::on_msg, g_cli_module,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4 ));
    app->set_evt_on_app_disconnected(std::bind<int>(&atframe::proxy::atproxy_cli_module::on_disconnected, g_cli_module,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ));
    app->set_evt_on_send_fail(std::bind<int>(&atframe::proxy::atproxy_cli_module::on_send_fail, g_cli_module,
                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    app->set_evt_on_available(::detail::app_handle_on_available(*cli_mod));


    return  app->init(uv_default_loop(), app_conf);
}




UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_run_noblock(libatproxy_cli_context context, uint64_t max_event_count){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    return SHAPP_CONTEXT(context)->run_noblock(max_event_count);
}


UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_stop(libatproxy_cli_context context){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    return SHAPP_CONTEXT(context)->stop();
}

UTIL_SYMBOL_EXPORT uint64_t __cdecl libatproxy_cli_msg_get_src_bus_id(libatproxy_cli_message msg) {
    if (SHAPP_MESSAGE_IS_NULL(msg)) {
        return 0;
    }
    return SHAPP_MESSAGE(msg)->head.src_bus_id;
}

UTIL_SYMBOL_EXPORT uint64_t __cdecl libatproxy_cli_msg_get_forward_from(libatproxy_cli_message msg){
    if (SHAPP_MESSAGE_IS_NULL(msg)) {
        return 0;
    }
    if (NULL == SHAPP_MESSAGE(msg)->body.forward) {
        return 0;
    }

    return SHAPP_MESSAGE(msg)->body.forward->from;
}

UTIL_SYMBOL_EXPORT uint64_t __cdecl libatproxy_cli_msg_get_forward_to(libatproxy_cli_message msg){
    if (SHAPP_MESSAGE_IS_NULL(msg)) {
        return 0;
    }
    if (NULL == SHAPP_MESSAGE(msg)->body.forward) {
        return 0;
    }
    return SHAPP_MESSAGE(msg)->body.forward->to;
}

UTIL_SYMBOL_EXPORT uint32_t __cdecl libatproxy_cli_msg_get_sequence(libatproxy_cli_message msg){
    if (SHAPP_MESSAGE_IS_NULL(msg)) {
        return 0;
    }
    return static_cast<uint32_t>(SHAPP_MESSAGE(msg)->head.sequence);
}


UTIL_SYMBOL_EXPORT int32_t __cdecl libatproxy_cli_set_log_level(libatproxy_cli_context context, log_type level) {
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    util::log::log_adaptor::get_instance().set_log_level(static_cast<util::log::log_type_t>(level));
    return EN_ATBUS_ERR_SUCCESS;
}