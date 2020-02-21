
#include <libatbus.h>
#include <libatbus_protocol.h>
#include <shapp.h>
#include <shapp_log.h>


#include "atproxy_cli.h"
#include "atproxy_cli_module.h"





#define  DEFAULT_STOP_TIMEOUT 10000
#define  DEFAULT_TICK_INTERVAL 16
#define  SHAPP_CONTEXT(x) ((::shapp::app *)(x))

#define  SHAPP_CONTEXT_IS_NULL(x) (NULL == (x))
#define  IS_EMPTY(x) ((!x || !x[0]))

#define SHAPP_MESSAGE(x) ((const ::shapp::app::msg_t *)(x))
#define SHAPP_MESSAGE_IS_NULL(x) (NULL == (x))

static  atframe::proxy::atproxy_cli_module *g_cli_module = NULL;

namespace detail {
    struct app_handle_on_available {
        std::reference_wrapper<atframe::proxy::atproxy_cli_module> atproxy_cli_module;
        app_handle_on_available(atframe::proxy::atproxy_cli_module &mod) : atproxy_cli_module(mod) {}

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
    return  SHAPP_CONTEXT(context)->get_bus_node()->send_data(bus_id, 0, buffer, sz, require_rsp);
}

UTIL_SYMBOL_EXPORT int32_t __cdecl libatapp_c_send_msg_by_type_name(libatproxy_cli_context context, const char*  type_name , const void *buffer, uint64_t sz, int32_t require_rsp){
    if (SHAPP_CONTEXT_IS_NULL(context)) {
        return EN_ATBUS_ERR_PARAMS;
    }
    shapp::app *app =  SHAPP_CONTEXT(context);
    const atbus::endpoint* parent_ep =  app->get_bus_node()->get_parent_endpoint();
    if (NULL != parent_ep){
        std::shared_ptr<atbus::protocol::custom_route_data> custom_route_data = std::make_shared<atbus::protocol::custom_route_data>();
        custom_route_data->type_name = type_name;
        printf("%d\n", custom_route_data->custom_route_type);
        return    SHAPP_CONTEXT(context)->get_bus_node()->send_data(parent_ep->get_id(), 0, buffer, sz, require_rsp, custom_route_data);
    } else{
        return  shapp::EN_SHAPP_ERR_NO_PARENT;
    }


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

    int ret =   util::log::init_log();
    if (ret != shapp::EN_SHAPP_ERR_SUCCESS){
        return ret;
    }

    LOGF_DEBUG("%d", IS_EMPTY( conf.father_address));
    if (conf.id == 0 || IS_EMPTY( conf.father_address) || IS_EMPTY(conf.type_name) || IS_EMPTY(conf.name) || IS_EMPTY(conf.engine_version))  return  shapp::EN_SHAPP_ERR_CONFIG;


    LOGF_INFO("id: 0x%llx engine_version:%s bus_listen_count:%d tags_count:%d father_address:%s", static_cast<unsigned  long long>(conf.id), conf.engine_version, conf.bus_listen_count, conf.tags_count
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
    return SHAPP_MESSAGE(msg)->head.sequence;
}