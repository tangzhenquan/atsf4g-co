//
// Created by owt50 on 2017/2/6.
//

#ifndef UTILITY_PROTOBUF_MINI_DUMPER_H
#define UTILITY_PROTOBUF_MINI_DUMPER_H

#pragma once

#include <cstddef>
#include <stdint.h>


#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

/**
 * @brief 返回易读数据
 * @note 因为protobuf默认的DebugString某些情况下会打印出巨量不易读的内容，故而自己实现。优化一下。
 * @param msg 要打印的message
 * @param ident 缩进层级
 */
const char *protobuf_mini_dumper_get_readable(const ::google::protobuf::Message &msg, uint8_t idx = 0);


/**
 * @brief 返回错误码文本描述
 * @param error_code 错误码，需要定义在MTSvrErrorDefine或MTErrorDefine里
 * @return 错误码的文本描述，永远不会返回NULL
 */
const char *protobuf_mini_dumper_get_error_msg(int error_code);


/**
 * @brief protobuf 数据拷贝
 * @note 加这个接口是为了解决protobuf的CopyFrom重载了CopyFrom(const Message&)。如果类型不匹配只能在运行时发现抛异常。加一层这个接口是为了提到编译期
 * @param dst 拷贝目标
 * @param src 拷贝源
 */
template <typename TMsg>
inline void protobuf_copy_message(TMsg &dst, const TMsg &src) {
    dst.CopyFrom(src);
}

template <typename TField>
inline void protobuf_copy_message(::google::protobuf::RepeatedField<TField> &dst, const ::google::protobuf::RepeatedField<TField> &src) {
    dst.Reserve(src.size());
    dst.CopyFrom(src);
}

#endif //_UTILITY_PROTOBUF_MINI_DUMPER_H
