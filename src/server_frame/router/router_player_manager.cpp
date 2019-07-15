//
// Created by owt50 on 2018/05/07.
//

#include <protocol/pbdesc/svr.const.pb.h>

#include <logic/session_manager.h>
#include <proto_base.h>

#include "router_player_manager.h"

router_player_manager::router_player_manager() : base_type(hello::EN_ROT_PLAYER) {}

const char *router_player_manager::name() const {
    return "[player_cache router manager]";
    ;
}

bool router_player_manager::remove_player_object(uint64_t user_id, priv_data_t priv_data) {
#if defined(UTIL_CONFIG_COMPILER_CXX_NULLPTR) && UTIL_CONFIG_COMPILER_CXX_NULLPTR
    return remove_player_object(user_id, nullptr, priv_data);
#else
    return remove_player_object(user_id, NULL, priv_data);
#endif
}

bool router_player_manager::remove_player_object(uint64_t user_id, std::shared_ptr<router_object_base> cache, priv_data_t priv_data) {
    key_t key(get_type_id(), user_id);
    return remove_object(key, cache, priv_data);
}

bool router_player_manager::remove_player_cache(uint64_t user_id, priv_data_t priv_data) {
#if defined(UTIL_CONFIG_COMPILER_CXX_NULLPTR) && UTIL_CONFIG_COMPILER_CXX_NULLPTR
    return remove_player_cache(user_id, nullptr, priv_data);
#else
    return remove_player_cache(user_id, NULL, priv_data);
#endif
}

bool router_player_manager::remove_player_cache(uint64_t user_id, std::shared_ptr<router_object_base> cache, priv_data_t priv_data) {
    key_t key(get_type_id(), user_id);
    return remove_cache(key, cache, priv_data);
}

void router_player_manager::set_create_object_fn(create_object_fn_t fn) { create_fn_ = fn; }

router_player_cache::object_ptr_t router_player_manager::create_player_object(uint64_t user_id, const std::string &openid) {
    router_player_cache::object_ptr_t ret;
    if (create_fn_) {
        ret = create_fn_(user_id, openid);
    }

    if (!ret) {
        ret = player_cache::create(user_id, openid);
    }

    return ret;
}

void router_player_manager::on_evt_remove_object(const key_t &key, const ptr_t &cache, priv_data_t priv_data) {
    player_cache::ptr_t obj = cache->get_object();
    // 释放本地数据, 下线相关Session
    session::ptr_t s = obj->get_session();
    if (s) {
        obj->set_session(NULL);
        s->set_player(NULL);
        session_manager::me()->remove(s, ::atframe::gateway::close_reason_t::EN_CRT_KICKOFF);
    }

    base_type::on_evt_remove_object(key, cache, priv_data);
}