//
// Created by owt50 on 2017/2/6.
//
#include <sstream>

#include <common/string_oprs.h>

#include <protocol/pbdesc/com.const.pb.h>
#include <protocol/pbdesc/svr.const.err.pb.h>

#include "environment_helper.h"

#include "protobuf_mini_dumper.h"

#define MSG_DISPATCHER_DEBUG_PRINT_BOUND 4096

const char *protobuf_mini_dumper_get_readable(const ::google::protobuf::Message &msg, uint8_t idx) {
    //    static char msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND] = {0};
    static std::string debug_string[256];

    ::google::protobuf::TextFormat::Printer printer;
    printer.SetUseUtf8StringEscaping(true);
    // printer.SetExpandAny(true);
    printer.SetUseShortRepeatedPrimitives(true);
    printer.SetSingleLineMode(false);
    printer.SetTruncateStringFieldLongerThan(MSG_DISPATCHER_DEBUG_PRINT_BOUND);
    printer.SetPrintMessageFieldsInIndexOrder(false);

    debug_string[idx].clear();
    printer.PrintToString(msg, &debug_string[idx]);

    //    msg_buffer[0] = 0;
    //    size_t sz = protobuf_mini_dumper_dump_readable(msg, msg_buffer, MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1, 0);
    //
    //    if (sz > MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5) {
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 5] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 4] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 3] = '.';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 2] = '}';
    //        msg_buffer[MSG_DISPATCHER_DEBUG_PRINT_BOUND - 1] = 0;
    //    }
    return debug_string[idx].c_str();
}


static std::string build_error_code_msg(const ::google::protobuf::EnumValueDescriptor &desc) {
    bool              has_descritpion = false;
    std::stringstream ss;

    if (desc.options().HasExtension(hello::error_code::description)) {
        auto description = desc.options().GetExtension(hello::error_code::description);
        if (!description.empty()) {
            ss << description << "[";
            has_descritpion = true;
        }
    }

    ss << desc.name();

    if (has_descritpion) {
        ss << "]";
    }
    ss << "(" << desc.number() << ")";

    return ss.str();
}

const char *protobuf_mini_dumper_get_error_msg(int error_code) {
    const char *ret = "Unknown Error Code";

    typedef UTIL_ENV_AUTO_MAP(int, std::string) error_code_desc_map_t;
    static error_code_desc_map_t cs_error_desc;
    static error_code_desc_map_t ss_error_desc;

    if (0 == error_code) {
        ret = "Success";
    }

    error_code_desc_map_t::const_iterator iter = cs_error_desc.find(error_code);
    if (iter != cs_error_desc.end()) {
        return iter->second.c_str();
    }

    iter = ss_error_desc.find(error_code);
    if (iter != ss_error_desc.end()) {
        return iter->second.c_str();
    }

    const ::google::protobuf::EnumValueDescriptor *desc = hello::EnErrorCode_descriptor()->FindValueByNumber(error_code);
    if (NULL != desc) {
        cs_error_desc[error_code] = build_error_code_msg(*desc);
        ret                       = cs_error_desc[error_code].c_str();
        return ret;
    }

    desc = hello::err::EnSysErrorType_descriptor()->FindValueByNumber(error_code);
    if (NULL != desc) {
        ss_error_desc[error_code] = build_error_code_msg(*desc);
        ret                       = ss_error_desc[error_code].c_str();
        return ret;
    }

    return ret;
}
