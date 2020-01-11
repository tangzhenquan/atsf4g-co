//
// Created by tom on 2020/1/11.
//


#include "shapp_module_impl.h"

namespace shapp {


    module_impl::module_impl() : enabled_(true), owner_(NULL) {}
    module_impl::~module_impl() {}


    int module_impl::stop() { return 0; }

    void module_impl::cleanup() {}

    int module_impl::timeout() { return 0; }

    int module_impl::tick() { return 0; }

    /*uint64_t module_impl::get_app_id() const {
        if (NULL == owner_) {
            return 0;
        }

        return owner_->get_id();
    }*/

    /*uint64_t module_impl::get_app_type_id() const {
        if (NULL == owner_) {
            return 0;
        }

        return owner_->get_type_id();
    }*/

    const char *module_impl::name() const {
        const char *ret = typeid(*this).name();
        if (NULL == ret) {
            return "RTTI Unavailable";
        }

        // some compiler will generate number to mark the type
        while (ret && *ret >= '0' && *ret <= '9') {
            ++ret;
        }
        return ret;
    }

    bool module_impl::enable() {
        bool ret = enabled_;
        enabled_ = true;
        return ret;
    }

    bool module_impl::disable() {
        bool ret = enabled_;
        enabled_ = false;
        return ret;
    }

    app *module_impl::get_app() { return owner_; }

    const app *module_impl::get_app() const { return owner_; }



}