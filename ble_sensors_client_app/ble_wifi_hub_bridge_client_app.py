#!/usr/bin/env python3

import threading
import logging
import sys
import argparse

import atomic
import udp_client
import http_server


read_data = [atomic.Atomic('-'),
             atomic.Atomic('-'),
             atomic.Atomic('-'),
             atomic.Atomic('-')]


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='''ble_wifi_nug_bridge_client_app

        Client app. for the BLE WiFi hub bridge device. It reads all the
        sensors' values from the mentioned device over UDP, then publishes
        them through a HTTP server.

        '''
    )

    parser.add_argument('-t',
                        '--target-ip',
                        type=str,
                        default='127.0.0.1',
                        help='Target BLE WiFi hub bridge device\'s IP addr.')

    parser.add_argument('-p',
                        '--target-port',
                        type=int,
                        default=3333,
                        help='Target BLE WiFi hub bridge device\'s port')

    parser.add_argument('-hi',
                        '--http-server-ip',
                        type=str,
                        default='127.0.0.1',
                        help='HTTP server IP addr.')

    parser.add_argument('-hp',
                        '--http-server-port',
                        type=int,
                        default=8000,
                        help='HTTP server port')

    parser.add_argument('-v',
                        '--verbose',
                        action='store_true',
                        help='Print verbose information')

    args = parser.parse_args()

    udp_server_ip = args.target_ip
    udp_server_port = args.target_port

    http_server_ip = args.http_server_ip
    http_server_port = args.http_server_port

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s'
    )

    udp_thread = threading.Thread(target=udp_client.start_udp_client,
                                  args=(read_data,
                                        udp_server_ip,
                                        udp_server_port))
    udp_thread.daemon = True
    udp_thread.start()

    http_server.start_http_server(read_data, http_server_ip, http_server_port)
