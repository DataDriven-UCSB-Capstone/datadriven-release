#source: https://pythontic.com/modules/socket/udp-client-server-example
import socket
from datetime import datetime
import struct

# Send list of data, instead of one data per time

current_date_time = datetime.now()
current_date_time_str = current_date_time.strftime("%Y-%m-%d %H:%M:%S")

listOfMsgFromClient = []
headerStr = "9XXX9H99XX999999 999999999999999 " + current_date_time_str + " "
GNSS_IMU_data = struct.pack("<ddffffffffff", 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0, 12.0)
OBDdata =   (# Size ↓↓      ↓↓ <- these two digits are PID #
                b"\x06\x41\x01\x00\x07\xE5\x80" # Monitor status since DTCs cleared.
                b"\x04\x41\x03\x02\x00" # Fuel system status
                b"\x03\x41\x04\x2E" # Calculated engine load
                b"\x03\x41\x06\x7F" # Short term fuel trim—Bank 1
                b"\x03\x41\x07\x7D" # Long term fuel trim—Bank 1
                b"\x03\x41\x08\x81" # Short term fuel trim—Bank 2
                b"\x03\x41\x09\x80" # Long term fuel trim—Bank 2
                b"\x03\x41\x0B\x1A" # Intake manifold absolute pressure
                b"\x04\x41\x0C\x0A\x5C" # Engine speed (Useful)
                b"\x03\x41\x0D\x00" # Vehicle speed (Useful)
                b"\x03\x41\x0E\x90" # Timing advance
                b"\x03\x41\x11\x21" # Throttle position (Useful)
                b"\x03\x41\x13\x33" # Oxygen sensors present (in 2 banks)
                b"\x04\x41\x15\x8E\x7E" # Oxygen Sensor 2
                b"\x04\x41\x19\x77\x80" # Oxygen Sensor 6
                b"\x03\x41\x1C\x01" # OBD standards this vehicle [California]
                b"\x04\x41\x1F\x04\x06" # Run time since engine start in seconds (Useful)
                b"\x04\x41\x21\x00\x00" # Distance traveled with MIL on
                b"\x04\x41\x23\x01\x61" # Fuel Rail Gauge Pressure
                b"\x06\x41\x24\x83\xB0\x41\xF5" # Oxygen Sensor 1
                b"\x06\x41\x28\x81\x64\x41\x4F" # Oxygen Sensor 5
                b"\x03\x41\x2C\x00" # Commanded EGR
                b"\x03\x41\x2D\xFF" # EGR Error
                b"\x03\x41\x2E\x42" # Commanded evaporative purge
                b"\x03\x41\x2F\xE8" # Fuel Tank Level Input
                b"\x03\x41\x30\xFF" # warm ups since codes cleared
                b"\x04\x41\x31\x27\xF0" # distance traveled since codes cleared
                b"\x04\x41\x32\xFE\x8C" # evap system vapor pressure
                b"\x03\x41\x33\x65" # barometer
                b"\x04\x41\x3C\x15\xE0" # catalyst temp bank 1, sensor 1
                b"\x04\x41\x3D\x15\xE0" # catalyst temp bank 2, sensor 1
                b"\x06\x41\x41\x00\x27\xE1\xA5" # monitor status this drive cycle
                b"\x04\x41\x42\x37\xAA" # control module voltage
                b"\x04\x41\x43\x00\x23" # absolute load value
                b"\x04\x41\x44\x7F\xEB" # commanded air-fuel ratio
                b"\x03\x41\x47\x4D" # throttle position B
                b"\x03\x41\x49\x31" # accelerator position D
                b"\x03\x41\x4A\x18" # accelerator position E
                b"\x03\x41\x51\x01" # fuel type
                b"\x03\x41\x55\x80" # short term secondary oxygen sensor bank 1 and bank 3
                b"\x03\x41\x56\x80" # long term secondary oxygen sensor bank 1 and bank 3
                b"\x03\x41\x57\x80" # short term secondary oxygen sensor bank 2 and bank 4
                b"\x03\x41\x58\x80" # long term secondary oxygen sensor bank 2 and bank 4
                b"\x03\x41\x62\x88" # actual engine percent torque
                b"\x04\x41\x63\x01\x1C" # engine reference torque
                b"\x07\x41\x66\x01\x00\x68\x00\x00" # mass air flow sensor
                b"\x05\x41\x67\x03\x80\x50" # engine coolant temperature
                b"\x09\x41\x68\x01\x64\x00\x00\x00\x00\x00" # intake air temperature sensor
                b"\x07\x41\x6C\x03\x05\x04\x00\x00" # commanded throttle acutator control and relative throttle position
                b"\x03\x41\x8E\x84" # engine friction
                b"\x06\x41\x9D\x00\x0B\x00\x0B" # engine fuel rate
                b"\x04\x41\x9E\x00\x44" # engine exhaust flow rate
                b"\x0B\x41\x9F\x05\xFF\x00\xFF\x00\x00\x00\x00\x00" # fuel system percentage use
                b"\x06\x41\xA6\x00\x01\x97\x94" # Odometer (Useful)
            )
bytesToSend = headerStr.encode() + GNSS_IMU_data + OBDdata
serverAddressPort = ("127.0.0.1", 65432) # Change this to api.datadrivenucsb.com to test actual API
bufferSize = 1024

# Create a UDP socket at client side
UDPClientSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

# Send to server using created UDP socket
UDPClientSocket.sendto(bytesToSend, serverAddressPort)

# msgFromServer = UDPClientSocket.recvfrom(bufferSize)

# current_date_time = datetime.now()
# current_date_time_str = current_date_time.strftime("%Y-%m-%d %H:%M:%S")

# msg = "Message from Server at {}: {}".format(current_date_time_str, msgFromServer[0])

# print(msg)
