import socket
import time
import logging

import atomic


def udp_client_run(read_data: atomic.Atomic,
                   server_ip,
                   server_port,
                   timeout):

    logging.info('UDP polling for data')

    try:
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        client_socket.settimeout(timeout)

        logging.debug('sending to {}:{}'.format(server_ip, server_port))

        for sensor_id in range(4):
            client_socket.sendto(str(sensor_id).encode(), (server_ip, server_port))

            logging.debug('trying to read...')

            data, _ = client_socket.recvfrom(1024)
            dec_data = data.decode()

            logging.debug('received data {}'.format(dec_data))

            read_data[sensor_id].set(dec_data)

    except socket.timeout:
        logging.debug(f"timeout reached")

    except Exception as e:
        logging.debug('exception {} raised'.format(str(e)))

    finally:
        client_socket.close()



def start_udp_client(read_data: atomic.Atomic,
                     server_ip='127.0.0.1',
                     server_port=3333,
                     timeout=1):
    logging.info('starting UDP client')

    period = 1

    while True:
        udp_client_run(read_data, server_ip, server_port, timeout)

        logging.debug(f'UDP client finished, re-running in {period}')

        time.sleep(period)
