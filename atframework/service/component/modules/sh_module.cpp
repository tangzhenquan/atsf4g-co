//
// Created by tom on 2019/12/19.
//


#include "sh_module.h"
#include <log/log_wrapper.h>
#include "utility/random_engine.h"

static const char *	common_type_name = "test";

namespace atframe {
    namespace component {

        sh_module::sh_module():etcd_module() {

        }

        sh_module::~sh_module() {

        }

        int sh_module::init() {
            int ret = etcd_module::init();
            if (ret != 0){
                return  ret;
            }
            ret = this->add_watcher_by_type_name("",  std::bind(&sh_module::on_watcher_notify, this, std::placeholders::_1));

            if (ret < 0) {
                WLOGERROR("add watcher by type name %s failed, res: %d", common_type_name, ret);
                return ret;
            }

            return 0;
        }

        int sh_module::reload() {
            return etcd_module::reload();
        }

        int sh_module::stop() {
            return etcd_module::stop();
        }

        int sh_module::timeout() {
            return etcd_module::timeout();
        }

        const char *sh_module::name() const {
            return "sh module";
        }

        int sh_module::tick() {
            int ret =  etcd_module::tick();
            if (ret != 0){
                return ret;
            } else{

            };
            return  0;
        }


        int sh_module::get_node_by_id(::atapp::app::app_id_t id, node_info_t* ){
            WLOGINFO("get_node_by_id 0x%llx disconnected", static_cast<unsigned long long>(id));
            return 0;
        }

        void sh_module::on_watcher_notify(atframe::component::etcd_module::watcher_sender_one_t & sender) {
            WLOGINFO("on_watcher_notify info:%s",sender.node.get().String().c_str());
            if (sender.node.get().action == node_action_t::EN_NAT_DELETE) {
                // trigger manager
                remove(sender.node.get().id);
            } else {
                // trigger manager
                set(sender.node);
            }
        }

        int sh_module::on_connected(const ::atapp::app &, ::atapp::app::app_id_t ) {
            return 0;
        }

        int sh_module::on_disconnected(const ::atapp::app &, ::atapp::app::app_id_t ) {
            return 0;
        }

        int sh_module::send_data_by_id(::atapp::app::app_id_t id,  const void *buffer, size_t s,
                                       bool require_rsp) {

            //get_app()->get_bus_node()->is_endpoint_available()
            int res  = get_app()->get_bus_node()->send_data(id, 0, buffer, s, require_rsp);


            if ( res != 0) {
                WLOGINFO("send_data  %d",res)
                res = connect_by_id(id);
                if (res <0 ){
                    return res;
                }else{
                    return get_app()->get_bus_node()->send_data(id, 0, buffer, s, require_rsp);
                }
            }
            return 0;
        }


        int sh_module::send_data_by_name_random(const std::string &name,  const void *buffer, size_t s,
                                         bool require_rsp) {
            ::atapp::app::app_id_t id;
            int st =  get_sv_by_type_name_random(name, &id);
            if (st <= 0 ){
                WLOGERROR("can't find name:%s nodes", name.c_str())
                return -1;
            }
            return  send_data_by_id(id,  buffer, s, require_rsp);
        }

        int sh_module::send_data_by_name_roundrobin(const std::string &name,  const void *buffer, size_t s,
                                                    bool require_rsp) {
            ::atapp::app::app_id_t id;
            int st =  get_sv_by_type_name_roundrobin(name, &id);
            if (st <= 0 ){
                WLOGERROR("can't find name:%s nodes", name.c_str())
                return -1;
            }
            return  send_data_by_id(id,  buffer, s, require_rsp);
        }


        void sh_module::swap(node_info_t &, node_info_t &) {
            //get_app()->get_bus_node()->send_data_msg()
        }

        int sh_module::set(node_info_t &node) {
            etcd_module::node_info_t& tmp_node = node_set_[node.id];
            tmp_node = node;
            WLOGINFO("add node:%s", node.String().c_str());

            //node_name_set_
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

        int sh_module::remove(::atapp::app::app_id_t id) {
            node_set_t::iterator iter = node_set_.find(id);
            if (iter != node_set_.end()) {
                WLOGINFO("lost node %llx", static_cast<unsigned long long>(id));
                node_set_.erase(iter);
            }else{
                WLOGWARNING("remove node %llx can't find it", static_cast<unsigned long long>(id))
                return 0;
            }

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
            return 0;
        }

        int sh_module::get_sv_by_type_name_random(const std::string &type_name, ::atapp::app::app_id_t *id) {
            node_name_set_t::iterator iter = node_name_set_.find(type_name);
            if(node_name_set_.end() == iter){
                WLOGWARNING("can not get daemon SV by random, node_name_set_ item not found type_name %s", type_name.c_str() )
                return -1;
            }
            std::vector<::atapp::app::app_id_t> tmp_sids;

            if (get_app()->get_type_name() == type_name){
                for(std::vector<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                    if(*sidit != get_app()->get_id()){
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

        int sh_module::get_sv_by_type_name_roundrobin(const std::string &type_name, ::atapp::app::app_id_t *id) {
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

            if (get_app()->get_type_name() == type_name){
                for(std::vector<::atapp::app::app_id_t>::iterator sidit = iter->second.begin(); sidit != iter->second.end(); ++sidit){
                    if(*sidit != get_app()->get_id()){
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

        int sh_module::connect_by_id(::atapp::app::app_id_t id) {
            WLOGINFO("connect_by_id  %llx", static_cast<unsigned long long>(id))
            node_set_t::iterator iter = node_set_.find(id);
            if (iter != node_set_.end()) {
                int res = get_app()->get_bus_node()->connect(iter->second.listens.front().c_str());
                if (res < 0) {
                    WLOGERROR("try to connect to id: %llx, address: %s failed, res: %d", static_cast<unsigned long long>(iter->second.id),
                              iter->second.listens.front().c_str(), res);
                    return res;
                }

            } else{
                WLOGERROR("can not find id:%llx ", static_cast<unsigned long long>(id));
                return -1;
            }
            return 0;
        }



    }
}