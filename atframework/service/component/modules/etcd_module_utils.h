#ifndef ATFRAME_SERVICE_COMPONENT_MODULES_ETCD_MODULE_UTILS_H
#define ATFRAME_SERVICE_COMPONENT_MODULES_ETCD_MODULE_UTILS_H

#pragma once

#include <stdint.h>
#include <string>
#include <list>
#include <vector>
namespace atframe {
    namespace component {

        struct node_action_t {
            enum type {
                EN_NAT_UNKNOWN = 0,
                EN_NAT_PUT,
                EN_NAT_DELETE,
            };
        };
        struct node_info_t {
            uint64_t id;
            std::string            name;
            std::string            hostname;
            std::list<std::string> listens;
            std::string            hash_code;
            uint64_t               type_id;
            std::string            type_name;
            std::string            version;
            std::vector<std::string> tags;
            node_action_t::type action;
            std::string String();
        };

        class etcd_module_utils {
        public:
            static bool unpack(node_info_t &out, const std::string &path, const std::string &json, bool reset_data);
            static void pack(const node_info_t &out, std::string &json);

        };


    }
}



#endif //ATFRAME_SERVICE_COMPONENT_MODULES_ETCD_MODULE_UTILS_H