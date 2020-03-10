//
// Created by tom on 2020/2/24.
//

#include "etcd_module_utils.h"
#include <etcdcli/etcd_packer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <common/string_oprs.h>


namespace atframe {
    namespace component {

        std::string node_info_t::String(){
            std::string ret;
            etcd_module_utils::pack(*this, ret);
            return ret;
        }

        bool etcd_module_utils::unpack(node_info_t &out, const std::string &path,
                                                           const std::string &json,
                                                           bool reset_data) {
            if (reset_data) {
                out.action = node_action_t::EN_NAT_UNKNOWN;
                out.id     = 0;
                out.name.clear();
                out.hostname.clear();
                out.listens.clear();
                out.hash_code.clear();
                out.type_id = 0;
                out.type_name.clear();
                out.version.clear();
            }

            if (json.empty()) {
                size_t start_idx = 0;
                for (size_t i = 0; i < path.size(); ++i) {
                    if (path[i] == '/' || path[i] == '\\' || path[i] == ' ' || path[i] == '\t' || path[i] == '\r' || path[i] == '\n') {
                        start_idx = i + 1;
                    }
                }

                // parse id from key if key is a number
                if (start_idx < path.size()) {
                    util::string::str2int(out.id, &path[start_idx]);
                }
                return false;
            }

            rapidjson::Document doc;
            if (::atframe::component::etcd_packer::parse_object(doc, json.c_str())) {
                rapidjson::Value                 val = doc.GetObject();
                rapidjson::Value::MemberIterator atproxy_iter;
                if (val.MemberEnd() != (atproxy_iter = val.FindMember("id"))) {
                    if (atproxy_iter->value.IsUint64()) {
                        out.id = atproxy_iter->value.GetUint64();
                    } else {
                        out.id = 0;
                        return false;
                    }
                } else {
                    return false;
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("name"))) {
                    if (atproxy_iter->value.IsString()) {
                        out.name = atproxy_iter->value.GetString();
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("hostname"))) {
                    if (atproxy_iter->value.IsString()) {
                        out.hostname = atproxy_iter->value.GetString();
                    } else {
                        return false;
                    }
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("listen"))) {
                    if (atproxy_iter->value.IsArray()) {
                        rapidjson::Document::Array nodes = atproxy_iter->value.GetArray();
                        for (rapidjson::Document::Array::ValueIterator iter = nodes.Begin(); iter != nodes.End(); ++iter) {
                            if (iter->IsString()) {
                                out.listens.push_back(iter->GetString());
                            }
                        }
                    }
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("tags"))) {
                    if (atproxy_iter->value.IsArray()) {
                        rapidjson::Document::Array nodes = atproxy_iter->value.GetArray();
                        for (rapidjson::Document::Array::ValueIterator iter = nodes.Begin(); iter != nodes.End(); ++iter) {
                            if (iter->IsString()) {
                                out.tags.push_back(iter->GetString());
                            }
                        }
                    }
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("hash_code"))) {
                    if (atproxy_iter->value.IsString()) {
                        out.hash_code = atproxy_iter->value.GetString();
                    } else {
                        return false;
                    }
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("type_id"))) {
                    if (atproxy_iter->value.IsUint64()) {
                        out.type_id = atproxy_iter->value.GetUint64();
                    } else {
                        out.type_id = 0;
                    }
                }

                if (val.MemberEnd() != (atproxy_iter = val.FindMember("type_name"))) {
                    if (atproxy_iter->value.IsString()) {
                        out.type_name = atproxy_iter->value.GetString();
                    } else {
                        return false;
                    }
                }


                if (val.MemberEnd() != (atproxy_iter = val.FindMember("version"))) {
                    if (atproxy_iter->value.IsString()) {
                        out.version = atproxy_iter->value.GetString();
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            return true;
        }

        void etcd_module_utils::pack(const node_info_t &src, std::string &json) {
            rapidjson::Document doc;
            doc.SetObject();



            doc.AddMember("id", src.id, doc.GetAllocator());
            doc.AddMember("name", rapidjson::StringRef(src.name.c_str(), src.name.size()), doc.GetAllocator());
            doc.AddMember("hostname", rapidjson::StringRef(src.hostname.c_str(), src.hostname.size()), doc.GetAllocator());

            rapidjson::Value listens;
            listens.SetArray();
            for (std::list<std::string>::const_iterator iter = src.listens.begin(); iter != src.listens.end(); ++iter) {
                // only report the channel available on different machine
                listens.PushBack(rapidjson::StringRef((*iter).c_str(), (*iter).size()), doc.GetAllocator());
            }

            doc.AddMember("listen", listens, doc.GetAllocator());

            rapidjson::Value tags;
            tags.SetArray();
            for (std::vector<std::string>::const_iterator iter = src.tags.begin(); iter != src.tags.end(); ++iter) {
                tags.PushBack(rapidjson::StringRef((*iter).c_str(), (*iter).size()), doc.GetAllocator());
            }
            doc.AddMember("tags", tags, doc.GetAllocator());
            doc.AddMember("hash_code", rapidjson::StringRef(src.hash_code.c_str(), src.hash_code.size()), doc.GetAllocator());
            doc.AddMember("type_id", src.type_id, doc.GetAllocator());
            doc.AddMember("type_name", rapidjson::StringRef(src.type_name.c_str(), src.type_name.size()), doc.GetAllocator());
            doc.AddMember("version", rapidjson::StringRef(src.version.c_str(), src.version.size()), doc.GetAllocator());


            // Stringify the DOM
            rapidjson::StringBuffer                    buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);

            json.assign(buffer.GetString(), buffer.GetSize());
        }

    }
}