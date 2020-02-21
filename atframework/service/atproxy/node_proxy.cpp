//
// Created by tom on 2020/1/13.
//

#include "node_proxy.h"
#include "atframe/atapp.h"
#include "protocols/libatproxy_proto.h"


namespace atframe {
    namespace proxy {
        node_proxy::node_proxy(etcd_mod_ptr etcd_mod) : binded_etcd_mod_(etcd_mod) {}
        int node_proxy::init() {
            if (!binded_etcd_mod_) {
                WLOGERROR("etcd mod not found");
                return -1;
            }
            return 0;
        }

        int node_proxy::tick() {
            return 0;
        }

        const char *node_proxy::name() const { return "node_proxy"; }

        int node_proxy::on_connected(const ::atapp::app &, ::atapp::app::app_id_t ) {
            return 0;
        }

        int node_proxy::on_disconnected(const ::atapp::app &, ::atapp::app::app_id_t ) {
            return 0;
        }

        int node_proxy::on_custom_route(const atapp::app &, const atbus::protocol::custom_route_data &,
                                        std::vector<uint64_t> &) {
            return 0;
        }

        int node_proxy::on_msg(const atapp::app &, const atapp::app::msg_t &recv_msg, const void *data, size_t l ) {
            if (NULL == data || 0 == l || NULL == recv_msg.body.forward) {
                return 0;
            }
            ss_msg msg;

            msgpack::unpacked result;
            msgpack::unpack(result, reinterpret_cast<const char *>(data), l);
            msgpack::object obj = result.get();
            if (obj.is_nil()) {
                return 0;
            }
            obj.convert(msg);

            switch (msg.head.cmd) {
                case ATFRAME_PROXY_CMD_REG:{
                    if (NULL == msg.body.reg) {
                        WLOGERROR("from server 0x%llx: recv bad reg body", static_cast<unsigned long long>(recv_msg.body.forward->from));
                        break;
                    }
                    auto reg = msg.body.reg;
                    std::stringstream  ss ;
                    ss << *reg;
                    WLOGDEBUG("from server 0x%llx: recv reg:%s ",  static_cast<unsigned long long>(recv_msg.body.forward->from), ss.str().c_str());

                    component::etcd_module::node_info_t node_info;
                    node_info.type_name = reg->type_name;
                    node_info.id = reg->bus_id;
                    node_info.name = reg->name;
                    node_info.type_name = reg->type_name;
                    node_info.version = reg->engine_version;

                    int res1 = binded_etcd_mod_->reg_custom_node(node_info);
                    WLOGINFO("reg_custom_node res:%d", res1);
                    break;
                }
                default:{
                    WLOGERROR("from server 0x%llx:  recv invalid cmd %d", static_cast<unsigned long long>(recv_msg.body.forward->from), static_cast<int>(msg.head.cmd));
                    break;
                }
            }
            return 0;
        }
    }
}