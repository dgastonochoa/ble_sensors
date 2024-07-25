#!/usr/bin/env python3

import logging
import functools
import datetime
import re

import atomic
import sensors

from http.server import BaseHTTPRequestHandler, HTTPServer

SENSOR_NAMES = [
    'SENSOR_MAGNETIC_FIELD',
    'SENSOR_PHOTOCELL',
    'SENSOR_TEMP_DETECTOR',
    'SENSOR_IR_DETECTOR'
]

class RequestHandler(BaseHTTPRequestHandler):
    def __init__(self, publish_data: atomic.Atomic, *args, **kwargs):
        self._publish_data = publish_data
        super().__init__(*args, **kwargs)


    def do_GET(self):
        global SENSOR_NAMES

        logging.debug('GET request detected')

        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()

        sensor_values = self._publish_data

        formatted_sensor_values = self.format_sensors_values(sensor_values)

        html_content = f'''
        <html>
        <head>
        <style>
            .monospaced {{
                font-family: Courier, "Courier New", monospace;
            }}
            </style>
            <title>UDP Data</title>
        </head>
        <body>
            <h1>Data from UDP Server</h1>
            <p class="monospaced">
                {formatted_sensor_values}
            </p>
        </body>
        </html>
        '''

        self.wfile.write(html_content.encode())



    def format_sensors_values(self, sensor_values):
        sensor_values_formatted = []

        for sens_id in range(4):
            sensor_raw_value = sensor_values[sens_id].get()

            sensor_raw_value_match = re.search(r'[0-9]+', sensor_raw_value)

            sensor_human_read_value = '-'
            if sensor_raw_value_match:
                sensor_num_val = int(sensor_raw_value_match.group())

                sensor_human_read_value = sensors.sens_get_human_readable_value(
                    SENSOR_NAMES[sens_id],
                    int(sensor_num_val)
                )

            sensor_values_formatted.append(self.sensor_entry_add_timestamp(
                SENSOR_NAMES[sens_id],
                sensor_raw_value,
                sensor_human_read_value))

        return '<br>'.join(sensor_values_formatted)


    def sensor_entry_add_timestamp(self,
                                   sensor_name,
                                   sensor_raw_value,
                                   sensor_human_readable_value):
        current_timestamp = datetime.datetime.now().strftime(
            '%Y-%m-%d %H:%M:%S')

        res = '{:<20}{:<40}{:<10}{:<10}'.format(
            current_timestamp,
            sensor_name,
            sensor_raw_value,
            sensor_human_readable_value)

        return res.replace(' ', '&nbsp;')


def start_http_server(publish_data, host, port):
    handler = functools.partial(RequestHandler, publish_data)
    server_address = (host, port)
    httpd = HTTPServer(server_address, handler)
    logging.info(f'HTTP server started on http://{host}:{port}')
    httpd.serve_forever()
