#!/usr/bin/env python3

import socket
import time
import logging


logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

counter = 0


def udp_server_listen(host='127.0.0.1', port=3333, timeout=5):
    global counter

    server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    server_socket.bind((host, port))

    server_socket.settimeout(timeout)

    logging.debug(f"starting server at {host}:{port} with timeout {timeout}")

    try:
        start_time = time.time()

        while True:
            data, client_address = server_socket.recvfrom(1024)

            decoded_data = data.decode()

            logging.debug(f"received data from {client_address}: {decoded_data}")

            response_data = int(decoded_data) + counter

            server_socket.sendto(str(response_data).encode(), client_address)

            logging.debug(f"sent {response_data} to {client_address}")

            counter += 1
            if counter > 99:
                counter = 0

            if time.time() - start_time > timeout:
                logging.debug(f"timeout reached")
                break

    except socket.timeout:
        logging.debug(f"timeout reached")

    finally:
        server_socket.close()


if __name__ == '__main__':
    while True:
        udp_server_listen()
        logging.debug(f"UDP server finished, starting again in 2")
        time.sleep(2)