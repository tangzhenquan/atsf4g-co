#!/usr/bin/env python
# -*- coding: utf-8 -*-


def gen_server_id(group_id, ipv4_str, port):
    ipv4_change = lambda x: sum([256 ** j * int(i) for j, i in enumerate(x.split('.')[::-1])])
    ipv4_int = ipv4_change(ipv4_str)
    s = 0
    s += group_id << 48
    s += ipv4_int << 16
    s += port
    return s


if __name__ == '__main__':
    server_id = gen_server_id(10, "192.168.1.90", 10002)
    print server_id,hex(server_id)
