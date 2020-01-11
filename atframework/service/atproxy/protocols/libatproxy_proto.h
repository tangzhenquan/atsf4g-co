
#ifndef ATFRAME_SERVICE_PROXY_PROTOCOL_PROTO_H
#define ATFRAME_SERVICE_PROXY_PROTOCOL_PROTO_H
#pragma once

#pragma once

#include <cstddef>
#include <ostream>
#include <stdint.h>

#include <msgpack.hpp>

enum ATFRAME_PROXY_PROTOCOL_CMD {
    ATFRAME_PROXY_CMD_INVALID = 0,

    ATFRAME_PROXY_CMD_REG = 1,


    ATFRAME_PROXY_CMD_MAX
};
MSGPACK_ADD_ENUM(ATFRAME_PROXY_PROTOCOL_CMD);

namespace atframe {
    namespace atproxy {

        struct ss_msg_head {
            ATFRAME_PROXY_PROTOCOL_CMD cmd; // ID: 0
            int error_code;                     // ID: 1

            ss_msg_head() : cmd(ATFRAME_PROXY_CMD_INVALID), error_code(0) {}

            MSGPACK_DEFINE(cmd, error_code);


            template <typename CharT, typename Traits>
            friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, const ss_msg_head &mh) {
                os << "{" << std::endl << "    cmd: " << mh.cmd << std::endl << "    error_code: " << mh.error_code << std::endl << "  }";

                return os;
            }
        };

        struct ss_reg {
            int pad = 0;

            MSGPACK_DEFINE(pad);
        };


        class ss_msg_body {
        public:
            ss_reg *reg;


            ss_msg_body(): reg(NULL) {
            }
            ~ss_msg_body() {
                if (NULL != reg) {
                    delete reg;
                }
            }

            template <typename TPtr>
            TPtr *make_body(TPtr *&p) {
                if (NULL != p) {
                    return p;
                }

                return p = new TPtr();
            }

            ss_reg *make_reg() {
                ss_reg *ret = make_body(reg);
                if (NULL == ret) {
                    return ret;
                }

                /*ret->session_ids.clear();
                ret->content.ptr = buffer;
                ret->content.size = s;*/
                return ret;
            }


            template <typename CharT, typename Traits>
            friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, const ss_msg_body &mb) {
                os << "{" << std::endl;


                os << "  }";

                return os;
            }

        private:
            ss_msg_body(const ss_msg_body &);
            ss_msg_body &operator=(const ss_msg_body &);
        };


        struct ss_msg {
            ss_msg_head head; // map.key = 1
            ss_msg_body body; // map.key = 2

            void init(ATFRAME_PROXY_PROTOCOL_CMD cmd) {
                head.cmd = cmd;
            }

            template <typename CharT, typename Traits>
            friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os, const ss_msg &m) {
                os << "{" << std::endl << "  head: " << m.head << std::endl << "  body:" << m.body << std::endl << "}";

                return os;
            }
        };



    }
}

// User defined class template specialization
namespace msgpack {
    MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
        namespace adaptor {
            template <>
            struct convert<atframe::atproxy::ss_msg> {
                msgpack::object const &operator()(msgpack::object const &o, atframe::atproxy::ss_msg &v) const {
                    if (o.type != msgpack::type::MAP) throw msgpack::type_error();
                    msgpack::object body_obj;
                    // just like protobuf buffer
                    for (uint32_t i = 0; i < o.via.map.size; ++i) {
                        if (o.via.map.ptr[i].key.via.u64 == 1) {
                            o.via.map.ptr[i].val.convert(v.head);
                        } else if (o.via.map.ptr[i].key.via.u64 == 2) {
                            body_obj = o.via.map.ptr[i].val;
                        }
                    }


                    // unpack body using head.cmd
                    if (!body_obj.is_nil()) {
                        switch (v.head.cmd) {
                            case ATFRAME_PROXY_CMD_REG: {
                                body_obj.convert(*v.body.make_body(v.body.reg));
                                break;
                            }

                            default: { // invalid cmd
                                break;
                            }
                        }
                    }

                    return o;
                }
            };

            template <>
            struct pack<atframe::atproxy::ss_msg> {
                template <typename Stream>
                packer<Stream> &operator()(msgpack::packer<Stream> &o, atframe::atproxy::ss_msg const &v) const {
                    // packing member variables as an map.
                    o.pack_map(2);
                    o.pack(1);
                    o.pack(v.head);

                    // pack body using head.cmd
                    o.pack(2);
                    switch (v.head.cmd) {

                        case ATFRAME_PROXY_CMD_REG: {
                            if (NULL == v.body.reg) {
                                o.pack_nil();
                            } else {
                                o.pack(*v.body.reg);
                            }
                            break;
                        }

                        default: { // just cmd, body is nil
                            o.pack_nil();
                            break;
                        }
                    }
                    return o;
                }
            };

            template <>
            struct object_with_zone<atframe::atproxy::ss_msg> {
                void operator()(msgpack::object::with_zone &o, atframe::atproxy::ss_msg const &v) const {
                    o.type = type::MAP;
                    o.via.map.size = 2;
                    o.via.map.ptr = static_cast<msgpack::object_kv *>(o.zone.allocate_align(sizeof(msgpack::object_kv) * o.via.map.size));

                    o.via.map.ptr[0] = msgpack::object_kv();
                    o.via.map.ptr[0].key = msgpack::object(1);
                    v.head.msgpack_object(&o.via.map.ptr[0].val, o.zone);

                    // pack body using head.cmd
                    o.via.map.ptr[1].key = msgpack::object(2);
                    switch (v.head.cmd) {

                        case ATFRAME_PROXY_CMD_REG: {
                            if (NULL == v.body.reg) {
                                o.via.map.ptr[1].val = msgpack::object();
                            } else {
                                v.body.reg->msgpack_object(&o.via.map.ptr[1].val, o.zone);
                            }
                            break;
                        }
                        default: { // invalid cmd
                            o.via.map.ptr[1].val = msgpack::object();
                            break;
                        }
                    }
                }
            };

        }
    }
}

#endif //ATFRAME_SERVICE_PROXY_PROTOCOL_PROTO_H
