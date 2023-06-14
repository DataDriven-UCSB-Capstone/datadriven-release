import socket
import pymysql
from datetime import datetime
from dateutil import tz
import hashlib
import struct

myHostName = socket.gethostname()

myIP = socket.gethostbyname(myHostName)
print(f"Private IP: {myIP}")

localIP = "0.0.0.0"
localPort = 65432
bufferSize = 1024

msgFromServer = "Hello UDP Client"
bytesToSend = str.encode(msgFromServer)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
 
# Bind to address and ip
UDPServerSocket.bind((localIP, localPort))

print("UDP server up and listening")

# [TODO] FETCH THESE FROM 'Vehicles' table in the database instead
car_details = {"car_id_ea6bbbae512c64e8": {"fuel_capacity": 19.5, "mileage": 21.0 * 1.609344},
               "car_id_fa091434d0b45449": {"fuel_capacity": 13.2, "mileage": 28.0 * 1.609344},
               "car_id_f4d4ac1125695472": {"fuel_capacity": 1.0, "mileage": 1.0}} # reference car


def decodePID(PID, clientID):
    if PID[1] == 0x41: # LIVE DATA Service 01
        match PID[2]:
            case 0x01: # monitor status since DTCs cleared
                return []
            case 0x03: # Fuel system status
                return []
            case 0x04: # Calculated engine load
                res = 100 * PID[3] / 255
                return [{"name": "engine_load", "type": "REAL", "value": res, "unit": "%"}]
            case 0x06: # short term fuel trim - Bank 1
                res = (100*PID[3])/128 - 100
                return [{"name": "short_term_fuel_trim_bank_1", "type": "REAL", "value": res, "unit": "%"}]
            case 0x07: # long term fuel trim - Bank 1
                res = (100*PID[3])/128 - 100
                return [{"name": "long_term_fuel_trim_bank_1", "type": "REAL", "value": res, "unit": "%"}]
            case 0x08: # short term fuel trim - Bank 2
                res = (100*PID[3])/128 - 100
                return [{"name": "short_term_fuel_trim_bank_2", "type": "REAL", "value": res, "unit": "%"}]
            case 0x09: # long term fuel trim - Bank 2
                res = (100*PID[3])/128 - 100
                return [{"name": "long_term_fuel_trim_bank_2", "type": "REAL", "value": res, "unit": "%"}]
            case 0x0B:
                res = PID[3]
                return [{"name": "intake_manifold_pressure", "type": "INT", "value": res, "unit": "kPa"}]
            case 0x0C:
                res = ((PID[3] * 256) + (PID[4]))/4
                return [{"name": "engine_speed", "type": "REAL", "value": res, "unit": "rpm"}]
            case 0x0D:
                res = PID[3]
                return [{"name": "vehicle_speed", "type": "INT", "value": res, "unit": "km/h"}]
            case 0x0E:
                res = PID[3] / 2 - 64
                return [{"name": "timing_advance", "type": "REAL", "value": res, "unit": "degree before TDC"}]
            case 0x11:
                res = 100 * PID[3] / 255
                return [{"name": "throttle_position", "type": "REAL", "value": res, "unit": "%"}]
            case 0x13: # oxygen sensors present
                return []
            case 0x15: # oxygen sensor 2
                res1 = PID[3] / 200
                res2 = (100 * PID[4])/128 - 100
                return [{"name": "oxygen_sensor_2_voltage", "type": "REAL", "value": res1, "unit": "V"},
                        {"name": "oxygen_sensor_2_short_term_fuel_trim", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x19: # oxygen sensor 6
                res1 = PID[3] / 200
                res2 = (100 * PID[4])/128 - 100
                return [{"name": "oxygen_sensor_6_voltage", "type": "REAL", "value": res1, "unit": "V"},
                        {"name": "oxygen_sensor_6_short_term_fuel_trim", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x1C: # OBD standard
                return []
            case 0x1F: # runtime since engine start
                res = (PID[3] * 256) + (PID[4])
                return [{"name": "runtime", "type": "INT", "value": res, "unit": "s"}]
            case 0x21: # distance traveled with MIL on
                res = (PID[3] * 256) + (PID[4])
                return [{"name": "distance_with_MIL", "type": "INT", "value": res, "unit": "km"}]
            case 0x23: # fuel rail gauge pressure
                res = 10 * ((PID[3] * 256) + (PID[4]))
                return [{"name": "fuel_rail_gauge_pressure", "type": "INT", "value": res, "unit": "kPa"}]
            case 0x24: # Oxygen Sensor 1
                res1 = 2 * (256*PID[3] + PID[4]) / 65536
                res2 = 8 * (256*PID[5] + PID[6]) / 65536
                return [{"name": "oxygen_sensor_1_afr", "type": "REAL", "value": res1, "unit": "1"},
                        {"name": "oxygen_sensor_1_voltage", "type": "REAL", "value": res2, "unit": "V"}]
            case 0x28: # Oxygen Sensor 5
                res1 = 2 * (256*PID[3] + PID[4]) / 65536
                res2 = 8 * (256*PID[5] + PID[6]) / 65536
                return [{"name": "oxygen_sensor_5_afr", "type": "REAL", "value": res1, "unit": "1"},
                        {"name": "oxygen_sensor_5_voltage", "type": "REAL", "value": res2, "unit": "V"}]
            case 0x2C: # Commanded EGR
                res = 100 * PID[3]/255
                return [{"name": "commanded_EGR", "type": "REAL", "value": res, "unit": "%"}]
            case 0x2D: # EGR Error
                res = 100 * PID[3]/128 - 100
                return [{"name": "EGR_error", "type": "REAL", "value": res, "unit": "%"}]
            case 0x2E: # Commanded evaporative purge
                res = 100 * PID[3]/255
                return [{"name": "evaporative_purge", "type": "REAL", "value": res, "unit": "%"}]
            case 0x2F: # Fuel Tank Level Input
                res1 = 100 * PID[3]/255
                if clientID in car_details:
                    actual_fuel_level = res1 * car_details[clientID]["fuel_capacity"] / 100 # fuel level in gallons
                    res2 = car_details[clientID]["mileage"] * actual_fuel_level 
                    return [{"name": "fuel_level", "type": "REAL", "value": res1, "unit": "%"},
                            {"name": "avg_range", "type": "REAL", "value": res2, "unit": "km"}]
                else:
                    return [{"name": "fuel_level", "type": "REAL", "value": res1, "unit": "%"}]
            case 0x30: # Warm ups since codes cleared
                res = PID[3]
                return [{"name": "warmups", "type": "INT", "value": res, "unit": "1"}]
            case 0x31: # Distance traveled since codes cleared
                res = (PID[3] * 256) + (PID[4])
                return [{"name": "distance_with_clear", "type": "INT", "value": res, "unit": "km"}]
            case 0x32: # Evap System Vapor Pressure
                a = int.from_bytes(PID[3].to_bytes(1, 'big', signed=False), byteorder="big", signed=True)
                b = int.from_bytes(PID[4].to_bytes(1, 'big', signed=False), byteorder="big", signed=True)
                res = ((a * 256) + b)/4
                return [{"name": "evap_system_vapor_pressure", "type": "REAL", "value": res, "unit": "Pa"}]
            case 0x33: # Absolute Barometric Pressure i.e. atmospheric pressure
                res = PID[3]
                return [{"name": "barometer", "type": "INT", "value": res, "unit": "kPa"}]
            case 0x3C: # Catalyst Temperature Bank 1, Sensor 1
                res = ((PID[3] * 256) + PID[4])/10 - 40
                return [{"name": "catalyst_temp_bank_1_sensor_1", "type": "INT", "value": res, "unit": "°C"}]
            case 0x3D: # Catalyst Temperature Bank 2, Sensor 1
                res = ((PID[3] * 256) + PID[4])/10 - 40
                return [{"name": "catalyst_temp_bank_2_sensor_1", "type": "INT", "value": res, "unit": "°C"}]
            case 0x41: # Monitor status this drive cycle # like PID 0x01
                return []
            case 0x42: # Control module voltage
                res = ((PID[3] * 256) + PID[4])/1000
                return [{"name": "control_voltage", "type": "REAL", "value": res, "unit": "V"}]
            case 0x43: # Absolute load value
                res = 100*((PID[3] * 256) + PID[4])/255
                return [{"name": "absolute_load_value", "type": "REAL", "value": res, "unit": "%"}]
            case 0x44: # commanded air-fuel ratio
                res = 2*((PID[3] * 256) + PID[4])/65536
                return [{"name": "commanded_afr", "type": "REAL", "value": res, "unit": "%"}]
            case 0x47: # throttle position B
                res = 100 * PID[3] / 255
                return [{"name": "throttle_B", "type": "REAL", "value": res, "unit": "%"}]
            case 0x49: # accelerator position D
                res = 100 * PID[3] / 255
                return [{"name": "accelerator_D", "type": "REAL", "value": res, "unit": "%"}]
            case 0x4A: # accelerator position E
                res = 100 * PID[3] / 255
                return [{"name": "accelerator_E", "type": "REAL", "value": res, "unit": "%"}]
            case 0x51: # fuel type
                return []
            case 0x55: # short term secondary oxygen sensor bank 1 and bank 3
                res1 = 100 * PID[3] / 128 - 100
                res2 = 100 * PID[3] / 128 - 100
                return [{"name": "short_term_oxygen_trim_bank_1", "type": "REAL", "value": res1, "unit": "%"},
                        {"name": "short_term_oxygen_trim_bank_3", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x56: # long term secondary oxygen sensor bank 1 and bank 3
                res1 = 100 * PID[3] / 128 - 100
                res2 = 100 * PID[3] / 128 - 100
                return [{"name": "long_term_oxygen_trim_bank_1", "type": "REAL", "value": res1, "unit": "%"},
                        {"name": "long_term_oxygen_trim_bank_3", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x57: # short term secondary oxygen sensor bank 2 and bank 4
                res1 = 100 * PID[3] / 128 - 100
                res2 = 100 * PID[3] / 128 - 100
                return [{"name": "short_term_oxygen_trim_bank_2", "type": "REAL", "value": res1, "unit": "%"},
                        {"name": "short_term_oxygen_trim_bank_4", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x58: # long term secondary oxygen sensor bank 2 and bank 4
                res1 = 100 * PID[3] / 128 - 100
                res2 = 100 * PID[3] / 128 - 100
                return [{"name": "long_term_oxygen_trim_bank_2", "type": "REAL", "value": res1, "unit": "%"},
                        {"name": "long_term_oxygen_trim_bank_4", "type": "REAL", "value": res2, "unit": "%"}]
            case 0x62: # actual engine percent torque
                res = PID[3] - 125
                return [{"name": "engine_torque_actual", "type": "INT", "value": res, "unit": "%"}]
            case 0x63: # engine reference torque
                res = (PID[3] * 256) + PID[4]
                return [{"name": "engine_torque_reference", "type": "INT", "value": res, "unit": "N⋅m"}]
            case 0x66: # mass air flow sensor
                rt_val = []
                if PID[3] & 0x1 == 0x1:
                    res1 = (256 * PID[4] + PID[5])/32
                    rt_val.append({"name": "mass_air_flow_sensor_A", "type": "REAL", "value": res1, "unit": "g/s"})
                if PID[3] & 0x2 == 0x2:
                    res2 = (256 * PID[6] + PID[7])/32
                    rt_val.append({"name": "mass_air_flow_sensor_B", "type": "REAL", "value": res2, "unit": "g/s"})
                return rt_val
            case 0x67: # engine coolant temperature
                rt_val = []
                if PID[3] & 0x1 == 0x1:
                    res1 = PID[4] - 40
                    rt_val.append({"name": "engine_coolant_temp_1", "type": "INT", "value": res1, "unit": "°C"})
                if PID[3] & 0x2 == 0x2:
                    res2 = PID[5] - 40
                    rt_val.append({"name": "engine_coolant_temp_2", "type": "INT", "value": res2, "unit": "°C"})
                return rt_val
            case 0x68: # intake air temperature sensor
                rt_val = []
                if PID[3] & 0x1 == 0x1:
                    res1 = PID[4] - 40
                    rt_val.append({"name": "intake_air_temp_1", "type": "INT", "value": res1, "unit": "°C"})
                if PID[3] & 0x2 == 0x2:
                    res2 = PID[5] - 40
                    rt_val.append({"name": "intake_air_temp_2", "type": "INT", "value": res2, "unit": "°C"})
                return rt_val
            case 0x6C: # commanded throttle acutator control and relative throttle position
                return []
            case 0x8E: # engine friction percent torque
                res = PID[3] - 125
                return [{"name": "engine_friction", "type": "INT", "value": res, "unit": "%"}]
            case 0x9D: # engine fuel rate
                return []
            case 0x9E: # engine exhaust flow rate
                return []
            case 0x9F: # fuel system percentage use
                return []
            case 0xA6: # odometer
                res = ((PID[3] << 24) + (PID[4] << 16) + (PID[5] << 8) + (PID[6]))/10
                return [{"name": "odometer", "type": "REAL", "value": res, "unit": "km"}]
            case _:
                return None
    else:
        return None

# Listen for incoming datagrams
while(True):
    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)

    message = bytesAddressPair[0]
    address = bytesAddressPair[1]

    server_date_time = datetime.now()
    server_date_time_str = server_date_time.strftime("%Y-%m-%d %H:%M:%S")

    clientIP  = f"Message from ({address[0]}, {address[1]}) at {server_date_time_str}"

    print("-" * 80)
    print(clientIP)
    print(message)

    messageComponents = message.split(b' ', maxsplit = 4)
    if len(messageComponents) != 5:
        print("Unexpected message format: expected 5 parts (VIN/IMEI/Date/Time/Data)")
    else:
        try:
            clientVIN = messageComponents[0]
            clientIMEI = messageComponents[1]
            clientID = f"car_id_{hashlib.blake2b(clientVIN,digest_size=8).hexdigest()}" if clientVIN != b'_' else f"device_id_{hashlib.blake2b(clientIMEI,digest_size=8).hexdigest()}"

            clientDate = messageComponents[2].decode() # YYYY-MM-DD (UTC)
            clientTime = messageComponents[3].decode() # HH:MM:SS (UTC)
            clientTimezone = tz.tzutc()
            dbTimezone = tz.gettz('America/Los_Angeles') # california time
            clientDatetime = datetime.strptime(f'{clientDate} {clientTime}', '%Y-%m-%d %H:%M:%S')
            clientDatetime = clientDatetime.replace(tzinfo=clientTimezone)
            dbDatetime = clientDatetime.astimezone(dbTimezone)
            dbDate = dbDatetime.strftime("%Y-%m-%d")
            dbTime = dbDatetime.strftime("%H:%M:%S")

            clientData = messageComponents[4]
            client_GNSSIMU = clientData[:56]
            decoded_GNSSIMU = struct.unpack("<ddffffffffff", client_GNSSIMU) # Lat/Lon/Alt/Speed/Vert Speed/Heading/Accel[xyz]/Gyro[xyz]
            clientLatitude = decoded_GNSSIMU[0]
            clientLongitude = decoded_GNSSIMU[1]
            clientAltitude = decoded_GNSSIMU[2]
            clientSpeed = decoded_GNSSIMU[3]
            clientVerticalSpeed = decoded_GNSSIMU[4]
            clientHeading = decoded_GNSSIMU[5]
            clientAccelX = decoded_GNSSIMU[6]
            clientAccelY = decoded_GNSSIMU[7]
            clientAccelZ = decoded_GNSSIMU[8]
            clientGyroX = decoded_GNSSIMU[9]
            clientGyroY = decoded_GNSSIMU[10]
            clientGyroZ = decoded_GNSSIMU[11]
            clientOBD = clientData[56:]
            conn = pymysql.connect(host = '<rds-link>', user = 'master', password = '', db = 'datadriven_db', charset = 'utf8')
            cursor = conn.cursor(pymysql.cursors.DictCursor)
            sql = "use datadriven_db"
            cursor.execute(sql)
            sql = f"CREATE TABLE if not exists {clientID} (id INT PRIMARY KEY AUTO_INCREMENT, atDate DATE NOT NULL, atTime TIME NOT NULL"
            sqlCreateAppend = ""
            sqlInsertNamesAppend  = ""
            sqlInsertDataAppend = ""

            decoded_data = {
                "latitude" : {"name": "latitude", "type": "REAL", "value": clientLatitude, "unit": "deg"},
                "longitude" : {"name": "longitude", "type": "REAL", "value": clientLongitude, "unit": "deg"},
                "altitude" : {"name": "altitude", "type": "REAL", "value": clientAltitude, "unit": "m"},
                "speed" : {"name": "speed", "type": "REAL", "value": clientSpeed, "unit": "m/s"},
                "vertical_speed" : {"name": "vertical_speed", "type": "REAL", "value": clientVerticalSpeed, "unit": "m/s"},
                "heading" : {"name": "heading", "type": "REAL", "value": clientHeading, "unit": "deg"},
                "accel_x" : {"name": "accel_x", "type": "REAL", "value": clientAccelX, "unit": "m/s/s"},
                "accel_y" : {"name": "accel_y", "type": "REAL", "value": clientAccelY, "unit": "m/s/s"},
                "accel_z" : {"name": "accel_z", "type": "REAL", "value": clientAccelZ, "unit": "m/s/s"},
                "gyro_x" : {"name": "gyro_x", "type": "REAL", "value": clientGyroX, "unit": "deg/s"},
                "gyro_y" : {"name": "gyro_y", "type": "REAL", "value": clientGyroY, "unit": "deg/s"},
                "gyro_z" : {"name": "gyro_z", "type": "REAL", "value": clientGyroZ, "unit": "deg/s"}
            }

            n = len(clientOBD)
            i = 0
            while i < n:
                datalen = min(clientOBD[i] + 1, n - i)
                obdPID = clientOBD[i:i+datalen]
                i += datalen
                if len(obdPID) < 3:
                    print("\tBroken PID:", obdPID.hex(' ').upper())
                else:
                    # print(f"\tService {obdPID[1] - 0x40:02X} obdPID {obdPID[2]:02X}: {obdPID[3:].hex(' ').upper()}")
                    data = decodePID(obdPID, clientID)
                    if data is None:
                        print(f"\tUnknown obdPID:", obdPID.hex(' ').upper())
                    elif data == []:  # known obdPID but unprocessed
                        pass
                    else:
                        for datum in data:
                            # print(f"\tDecoded: {datum}")
                            name = datum['name']
                            if name not in decoded_data:
                                decoded_data[name] = datum
                            else:
                                if decoded_data[name] != datum:
                                    print(f"Conflict\tOLD: {decoded_data[name]}\tNEW: {datum}")
            for data in decoded_data.values():
                sqlCreateAppend += f", {data['name']} {data['type']}"
                sqlInsertNamesAppend += f", {data['name']}"
                sqlInsertDataAppend += f", {data['value']}"
            sql += sqlCreateAppend + ")"
            print("SQL: " + sql + "\n")
            cursor.execute(sql)
            sql = f"INSERT INTO {clientID} (atDate, atTime"
            sql += sqlInsertNamesAppend
            sql += (f") VALUES ('{dbDate}', '{dbTime}'")
            sql += sqlInsertDataAppend + ")"
            print("SQL: " + sql + "\n")
            cursor.execute(sql)
            # Commit
            conn.commit()
            # Close
            conn.close()
        except Exception as e:
            print(f"Exception thrown: {e}")

    # Sending a reply to client
    # UDPServerSocket.sendto(bytesToSend, address)
