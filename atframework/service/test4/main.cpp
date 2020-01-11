//
// Created by tom on 2020/1/10.
//

#include "shapp_log.h"



int main(int , char *[]) {
    util::log::init_log();

    LOGF_INFO("dsadasdas:%d", 123);

}