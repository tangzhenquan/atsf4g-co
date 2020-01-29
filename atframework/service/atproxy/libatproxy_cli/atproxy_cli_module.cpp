

#include <shapp_log.h>
#include <shapp.h>
#include <libatbus_protocol.h>
#include <sstream>
#include "atproxy_cli_module.h"
#include "config/atframe_service_types.h"

namespace atframe {
    namespace proxy {




        int atproxy_cli_module::init() {

            LOGF_INFO("atproxy_cli_module init")

            const atbus::endpoint* parent_ep =  get_app()->get_bus_node()->get_parent_endpoint();
            if (NULL != parent_ep){
                const shapp::app_conf& conf = get_app()->get_conf();
                ss_msg msg;
                msg.init(ATFRAME_PROXY_CMD_REG);
                msg.head.error_code = shapp::EN_SHAPP_ERR_SUCCESS;
                msg.body.make_reg();
                msg.body.reg->bus_id = get_app()->get_id();
                msg.body.reg->tags = conf.tags;
                msg.body.reg->name = conf.name;
                msg.body.reg->type_name = conf.type_name;
                return post_ss_msg( parent_ep->get_id(),::atframe::component::service_type::EN_ATST_ATPROXY, msg);
            }

            return 0;
        }

        int atproxy_cli_module::stop() {
            LOGF_INFO("atproxy_cli_module stop")
            return 0;
        }

        int atproxy_cli_module::timeout() {
            LOGF_INFO("atproxy_cli_module timeout")
            return 0;
        }

        int atproxy_cli_module::tick() {
            //LOGF_INFO("atproxy_cli_module tick")
            return 0;
        }

        const char *atproxy_cli_module::name() const {
            return "atproxy_cli_module";
        }

        int atproxy_cli_module::on_connected( ::shapp::app & , atbus::endpoint & ep  , int status) {
            LOGF_INFO("node 0x%llx connected, status: %d", static_cast<unsigned long long>(ep.get_id()), status);
            return 0;
        }

        int atproxy_cli_module::on_disconnected( ::shapp::app &, atbus::endpoint & , int ) {
            LOGF_INFO("atproxy_cli_module on_disconnected")
            return 0;
        }

        int atproxy_cli_module::on_msg( shapp::app &, const shapp::app::msg_t & msg, const void *,
                                       size_t ) {
            LOGF_INFO("receive a message(from 0x%llx, type=%d) ", static_cast<unsigned long long>(msg.head.src_bus_id), msg.head.type);
            return 0;
        }

        int atproxy_cli_module::on_available(::shapp::app &) {
            LOGF_INFO("atproxy_cli_module on_available")
            return  0;
        }

        int atproxy_cli_module::post_ss_msg(::atbus::node::bus_id_t tid, int type, ss_msg &msg) {

            std::stringstream ss;
            msgpack::pack(ss, msg);
            std::string packed_buffer;
            ss.str().swap(packed_buffer);

            return  get_app()->get_bus_node()->send_data(tid, type, packed_buffer.data(), packed_buffer.size());
        }


    }
}