import sys
import time
import struct
import datetime

import pytz
import serial
import requests

city_data = [
    ("2358820", pytz.timezone('America/New_York')), # Balmore
    ("1940517", pytz.timezone('Asia/Qatar')), # Doha
    ("1103816", pytz.timezone('Australia/Melbourne')),# Melbourne
    ("906057", pytz.timezone('Europe/Stockholm')), # Stockholm
]
BEGIN = "\x08\x06\x07\x05\x03\x00\x09"
TIME_BETWEEN_UPDATES = 60*10

url = "https://query.yahooapis.com/v1/public/yql"

_, serial_port = sys.argv
if serial_port == 'FAKE':
    conn = open('/dev/null', 'w')
else:
    conn = serial.Serial(serial_port, 1200)

last_update = 0
weather = ['\0'*8] * len(city_data)
while True:
    t = datetime.datetime.utcnow().replace(tzinfo=pytz.utc)
    
    for i, (woeid, tz) in enumerate(city_data):
        if time.time() > last_update + TIME_BETWEEN_UPDATES and False:
            query = "select * from weather.forecast where woeid={0}".format(woeid)

            d = requests.get(url, params=dict(q=query, format='json')).json()

            temperature = float(d[u"query"][u"results"][u"channel"][u"item"][u"condition"][u"temp"])
            pressure = float(d[u"query"][u"results"][u"channel"][u'atmosphere'][u'pressure'])


            weather[i] = struct.pack('<2f', temperature, pressure)
        local_t = t.astimezone(tz)

        msg = BEGIN + chr(i) + chr(local_t.hour) + chr(local_t.minute) + chr(local_t.second) + weather[i]

        conn.write(msg)

    time.sleep(.1)
