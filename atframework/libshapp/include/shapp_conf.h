#ifndef ATFRAME_LIBSHAPP_SHAPP_CONF_H
#define ATFRAME_LIBSHAPP_SHAPP_CONF_H
#pragma once



#include "libatbus.h"
#include <string>
#include <vector>
namespace shapp{
    struct app_conf {
        // bus configure
        uint64_t id;
        std::vector<uint64_t> id_mask; // convert a.b.c.d -> id
        std::vector<std::string> bus_listen;
        atbus::node::conf_t bus_conf;


        // app configure
        uint64_t stop_timeout;  // module timeout when receive a stop message, libuv use uint64_t
        uint64_t tick_interval; // tick interval, libuv use uint64_t

        std::string type_name;
        std::string name;
        std::string hash_code;
        std::vector<std::string> tags;
        std::string engine_version;
    };

    typedef enum {
        EN_SHAPP_ERR_SUCCESS = 0,
        EN_ATAPP_ERR_NOT_INITED = -1001,
        EN_ATAPP_ERR_ALREADY_INITED = -1002,
        EN_ATAPP_ERR_SETUP_TIMER = -1004,
        EN_ATAPP_ERR_ALREADY_CLOSED = -1005,
        EN_ATAPP_ERR_SETUP_ATBUS = -1101,
        EN_ATAPP_ERR_SEND_FAILED = -1102,
        EN_ATAPP_ERR_NO_AVAILABLE_ADDRESS = -1802,
        EN_ATAPP_ERR_CONNECT_ATAPP_FAILED = -1803,
        EN_ATAPP_ERR_MIN = -1999,
    } ATAPP_ERROR_TYPE;
}



#endif //ATFRAME_LIBSHAPP_SHAPP_CONF_H
