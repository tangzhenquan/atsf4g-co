//
// Created by tom on 2020/1/13.
//

#include "node_proxy.h"
#include "atframe/atapp.h"
#include "protocols/libatproxy_proto.h"
#include "utility/random_engine.h"

namespace atframe {
    namespace proxy {
        node_proxy::node_proxy(etcd_mod_ptr etcd_mod) : binded_etcd_mod_(etcd_mod) {}
        int node_proxy::init() {
            if (!binded_etcd_mod_) {
                WLOGERROR("etcd mod not found");
                return -1;
            }

            if (!conf_.watch_paths.empty()){

                for(std::vector<std::string>::const_iterator it = conf_.watch_paths.begin(); it != conf_.watch_paths.end(); ++it){
                    int ret = binded_etcd_mod_->add_watcher_by_path(*it, std::bind(&node_proxy::on_watcher_notify, this, std::placeholders::_1));

                    if (ret < 0) {
                        WLOGERROR("add watcher by  path %s failed, res: %d",  (*it).c_str(), ret);
                        return ret;
                    }
                    WLOGINFO("watch atproxy by  path: %s", (*it).c_str());
                }
            }

            return 0;
        }

        int node_proxy::tick() {
            return 0;
        }

        int node_proxy::reload() {


            // load init cluster member from configure
            util::config::ini_loader &cfg = get_app()->get_configure();

            {
                std::vector<std::string> conf_watch_paths;
                cfg.dump_to("atproxy.etcd.proxy.watch_path", conf_watch_paths);
                if (!conf_watch_paths.empty()) {
                    conf_.watch_paths = conf_watch_paths;
                }
            }

            return 0;
        }

        const char *node_proxy::name() const { return "node_proxy"; }

        int node_proxy::on_connected(const ::atapp::app &, const atbus::endpoint &ep) {

            if (!ep.get_type_name().empty()){
                set_tag(ep.get_type_name(),ep.get_id());
            }
            return 0;
        }

        int node_proxy::on_disconnected(const ::atapp::app &, const atbus::endpoint & ep) {
            if (!ep.get_type_name().empty()){
                remove_tag(ep.get_type_name(),ep.get_id());
            }
            return 0;
        }

        int node_proxy::on_custom_route(const atapp::app &, ::atapp::app::app_id_t src_id , const atbus::protocol::custom_route_data & custom_route_data,
                                        std::vector<uint64_t> & bus_ids) {
            if(custom_route_data.custom_route_type == atbus::protocol::custom_route_data::CUSTOM_ROUTE_UNICAST){
                ::atapp::app::app_id_t id;
                int res = get_sv_by_type_name_roundrobin(custom_route_data.type_name, custom_route_data.src_type_name,  src_id, &id);
                if (res > 0){
                    bus_ids.push_back(id);
                }
            } else if (custom_route_data.custom_route_type == atbus::protocol::custom_route_data::CUSTOM_ROUTE_BROADCAST){
                get_svs_by_type_name_all(custom_route_data.type_name, custom_route_data.src_type_name, src_id, bus_ids);
            } else if  (custom_route_data.custom_route_type == atbus::protocol::custom_route_data::CUSTOM_ROUTE_BROADCAST2){
                get_svs_by_type_name_all2(custom_route_data.type_name, custom_route_data.src_type_name, src_id, bus_ids);
            }

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
            try {
                obj.convert(msg);
            }catch (...){
                WLOGERROR("from server 0x%llx: convert fail", static_cast<unsigned long long>(recv_msg.body.forward->from));
            }




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

                    node_info_t node_info;
                    node_info.type_name = reg->type_name;
                    node_info.id = reg->bus_id;
                    node_info.name = reg->name;
                    node_info.type_name = reg->type_name;
                    node_info.version = reg->engine_version;

                    break;
                }
                case ATFRAME_PROXY_CMD_BROADCAST:{
                }
                default:{
                    WLOGERROR("from server 0x%llx:  recv invalid cmd %d", static_cast<unsigned long long>(recv_msg.body.forward->from), static_cast<int>(msg.head.cmd));
                    break;
                }
            }
            return 0;
        }
        int node_proxy::set(node_info_t &node){
            node_info_t& tmp_node = node_set_[node.id];
            tmp_node = node;
            std::vector<::atapp::app::app_id_t> &sids = node_name_set_[tmp_node.type_name];
            if(std::find(sids.begin(), sids.end(), node.id) == sids.end())
            {
                sids.push_back(node.id);
            }

            if (node_name_tick_set_.find(node.type_name) == node_name_tick_set_.end()){
                node_name_tick_set_[node.type_name] = 0;
            }

            return 0;
        }
        int node_proxy::remove(::atapp::app::app_id_t id){
            node_set_t::iterator iter = node_set_.find(id);
            if (iter == node_set_.end()) {
                WLOGWARNING("remove node %llx can't find it", static_cast<unsigned long long>(id))
                return 0;
            }
            WLOGINFO("lost node %llx", static_cast<unsigned long long>(id));

            //remove from node_name_set
            node_name_set_t::iterator iter2 = node_name_set_.find(iter->second.type_name);
            if (iter2 == node_name_set_.end()){
                WLOGWARNING("can not find node_name_set_ item  node %llx type_name %s", static_cast<unsigned long long>(id),iter->second.type_name.c_str() )
            } else{
                std::vector<::atapp::app::app_id_t>::iterator iter3 = std::find(iter2->second.begin(), iter2->second.end(), iter->second.id);
                if (iter3 == iter2->second.end()){
                    WLOGWARNING("can not find node_name_set_ id map item  node %llx type_name %s", static_cast<unsigned long long>(id),iter->second.type_name.c_str() )
                } else{
                    iter2->second.erase(iter3);
                }
                if (iter2->second.empty()){
                    node_name_set_.erase(iter2);
                }
            }

            node_name_tick_set_t::iterator iter4 = node_name_tick_set_.find(iter->second.type_name);
            if(iter4 == node_name_tick_set_.end()){
                WLOGWARNING("can not find node_name_tick_set_ map item type_name %s", iter->second.type_name.c_str() )
            } else{
                node_name_tick_set_.erase(iter4);
            }

            node_set_.erase(iter);
            return 0;
        }

        void   node_proxy::on_watcher_notify(atframe::component::etcd_module::watcher_sender_one_t &sender){
            node_action_t::type action = sender.node.get().action;
            WLOGINFO("on_watcher_notify action:%d", action);
            if (action == node_action_t::EN_NAT_DELETE) {
                // trigger manager
                remove(sender.node.get().id);
            } else {
                // trigger manager
                set(sender.node.get());
            }
        }
        void node_proxy::swap(node_info_t &l, node_info_t &r){
            using std::swap;
            swap(l.id, r.id);
            swap(l.name, r.name);
            swap(l.hostname, r.hostname);
            swap(l.listens, r.listens);
            swap(l.hash_code, r.hash_code);
            swap(l.type_id, r.type_id);
            swap(l.type_name, r.type_name);
            swap(l.action, r.action);
        }

        int node_proxy::get_sv_by_type_name_random(const std::string &type_name, const std::string& src_type_name, ::atapp::app::app_id_t src_id, ::atapp::app::app_id_t *id) {
            node_name_set_t::iterator iter = node_name_set_.find(type_name);
            if(node_name_set_.end() == iter){
                WLOGWARNING("can not get daemon SV by random, node_name_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }
            std::vector<::atapp::app::app_id_t> tmp_sids;

           if (src_type_name == type_name){
                for(std::vector<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                    if(*sidit != src_id){
                        tmp_sids.push_back(*sidit);
                    }
                }
            }else{

                tmp_sids = iter->second;
            }
            if(tmp_sids.empty()){
                return 0;
            }
            size_t index = (tmp_sids.size() + util::random_engine::random()) % tmp_sids.size();
            *id = tmp_sids[index];
            return 1;
        }

        int node_proxy::get_sv_by_type_name_roundrobin(const std::string &type_name, const std::string& src_type_name, ::atapp::app::app_id_t src_id, ::atapp::app::app_id_t *id) {
            node_name_set_t::iterator iter = node_name_set_.find(type_name);
            if(node_name_set_.end() == iter){
                WLOGWARNING("can not get daemon SV by roundrobin, node_name_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }

            node_name_tick_set_t::iterator iter2 = node_name_tick_set_.find(type_name);
            if(node_name_tick_set_.end() == iter2){
                WLOGWARNING("can not get daemon SV by roundrobin, node_name_tick_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }

            std::vector<::atapp::app::app_id_t> tmp_sids;

            if (src_type_name == type_name){
                for(std::vector<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                    if(*sidit != src_id){
                        tmp_sids.push_back(*sidit);
                    }
                }
            }else{

                tmp_sids = iter->second;
            }
            if(tmp_sids.empty()){
                return 0;
            }

            size_t index = (tmp_sids.size() + (iter2->second++)) % tmp_sids.size();
            *id = tmp_sids[index];
            return 1;
        }
        int node_proxy::get_svs_by_type_name_all( const std::string& type_name, const std::string& , ::atapp::app::app_id_t src_id, std::vector<uint64_t>& bus_ids){
            node_name_set_t::iterator iter = node_name_set_.find(type_name);
            if(node_name_set_.end() == iter){
                WLOGWARNING("can not get all SV , node_name_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }
            int ret = 0;
            for(std::vector<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                if(*sidit != src_id){
                    bus_ids.push_back(*sidit);
                    ret++;
                }
            }
            return ret;
        }


        void node_proxy::remove_tag(const std::string &tag, ::atapp::app::app_id_t id) {
            std::set<::atapp::app::app_id_t> &sids = tags_id_set_[tag];
            sids.erase(id);
        }

        void node_proxy::set_tag(const std::string &tag, ::atapp::app::app_id_t id) {
            std::set<::atapp::app::app_id_t> &sids = tags_id_set_[tag];
            sids.insert(id);
        }

        int node_proxy::get_svs_by_type_name_all2( const std::string& type_name, const std::string& , ::atapp::app::app_id_t src_id, std::vector<uint64_t>& bus_ids){
            tags_id_set_t::iterator iter = tags_id_set_.find(type_name);
            if(tags_id_set_.end() == iter){
                WLOGWARNING("can not get all SV , tags_id_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }
            int ret = 0;
            for(std::set<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                if(*sidit != src_id){
                    bus_ids.push_back(*sidit);
                    ret++;
                }
            }
            return ret;
        }

    }
}