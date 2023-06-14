import 'package:flutter/material.dart';
import 'package:datadriven/main_screens/statistic_screen.dart';
import 'package:flutter_map/flutter_map.dart';
import 'package:latlong2/latlong.dart';

import 'dart:async';
import 'package:http/http.dart' as http;
import 'dart:convert';

class VehicleData {
  double? lat;
  double? long;
  double? ang;
  double? engineSpeed;
  double? vehicleSpeed;
  double? runtime;
  double? odometer;
  String? atTime; // TODO: parse atTime like we do for atDate
  DateTime? atDate;
  String? car_id;

  VehicleData(
      {this.lat,
      this.long,
      this.ang,
      this.engineSpeed,
      this.vehicleSpeed,
      this.runtime,
      this.odometer,
      this.atTime,
      this.atDate,
      this.car_id});

  factory VehicleData.fromJson(Map<String, dynamic> json) {
    return VehicleData(
        lat: json['lat'],
        long: json['lng'],
        ang: json['ang'],
        engineSpeed: json['engine_speed'],
        vehicleSpeed: json['vehicle_speed'],
        runtime: json['runtime'],
        odometer: json['odometer'],
        atTime: json['atTime'],
        atDate: DateTime.parse(json['atDate']),
        car_id: json['car_id']);
  }
}

class MapScreen extends StatefulWidget {
  const MapScreen({super.key});

  @override
  State<MapScreen> createState() => _MapWidgetState();
}

class CarDataPopup extends StatefulWidget {
  const CarDataPopup({super.key});

  @override
  State<CarDataPopup> createState() => _CarDataPopupState();
}

class _CarDataPopupState extends State<CarDataPopup> {
  @override
  Widget build(BuildContext context) {
    return Container(
      width: 100,
      height: 100,
      child: Placeholder(),
    );
  }
}

Map<String, String> rawVehicleDataMap = {};
int selectedMarkerIdx = 0;
String selectedCarId = "";

List<Marker> _markers = [];
Map<String, String> carIDtoFriendlyName = {
  ""
      "car_id_ea6bbbae512c64e8": "Nick's Honda",
  "car_id_fa091434d0b45449": "Brian's VW",
  "car_id_f4d4ac1125695472": "Debug"
};

// TODO more friendly names and icons
Map<String, String> vehicleParameterToFriendlyName = {
  'absolute_load_value': 'absolute load value',
  'accel_x': 'accelerometer in x direction',
  'accel_y': 'accelerometer in y direction',
  'accel_z': 'accelerometer in z direction',
  'accelerator_D': 'accelerator position D',
  'accelerator_E': 'accelerator position E',
  'altitude': 'geographic postion (up/down)',
  'atDate': 'date when the GNSS fixed',
  'atTime': 'time when the GNSS fixed',
  'avg_range': 'estimated distance until car runs out of fuel',
  'barometer': 'absolute barometric pressure',
  'catalyst_temp_bank_1_sensor_1': 'catalyst temperature bank 1 sensor 1',
  'catalyst_temp_bank_2_sensor_1': 'catalyst temperature bank 2 sensor 1',
  'commanded_afr': 'commanded air-fuel ratio',
  'commanded_EGR': 'commanded exhaust gas recirculation',
  'control_voltage': 'control module voltage',
  'distance_with_clear': 'distance traveled since codes cleared',
  'distance_with_MIL': 'distance traveled with malfunction indicator lamp on',
  'EGR_error': 'exhaust gas recirculation error',
  'engine_coolant_temp_1': 'engine coolant temperature sensor 1',
  'engine_coolant_temp_2': 'engine coolant temperature sensor 2',
  'engine_friction': 'engine friction percent torque',
  'engine_load': 'calculated engine load',
  'engine_speed': 'engine rotation speed',
  'engine_torque_actual': 'actual engine percent torque',
  'engine_torque_reference': 'engine reference torque',
  'evap_system_vapor_pressure': 'evap system vapor pressure',
  'evaporative_purge': 'commanded evaporative purge',
  'fuel_level': 'fuel tank level (percentage)',
  'fuel_rail_gauge_pressure': 'fuel rail gauge pressure',
  'gyro_x': 'gyroscope in x direction',
  'gyro_y': 'gyroscope in y direction',
  'gyro_z': 'gyroscope in z direction',
  'heading': 'direction determined by GNSS',
  'intake_air_temp_1': 'intake air temperature sensor 1',
  'intake_air_temp_2': 'intake air temperature sensor 2',
  'intake_manifold_pressure': 'intake manifold pressure',
  'latitude': 'geographic position (north/south)',
  'long_term_fuel_trim_bank_1': 'long term fuel trim - bank 1',
  'long_term_fuel_trim_bank_2': 'long term fuel trim - bank 2',
  'long_term_oxygen_trim_bank_1': 'long term oxygen trim bank 1',
  'long_term_oxygen_trim_bank_2': 'long term oxygen trim bank 2',
  'long_term_oxygen_trim_bank_3': 'long term oxygen trim bank 3',
  'long_term_oxygen_trim_bank_4': 'long term oxygen trim bank 4',
  'longitude': 'geographic position (east/west)',
  'mass_air_flow_sensor_A': 'mass air flow sensor A',
  'mass_air_flow_sensor_B': 'mass air flow sensor B',
  'odometer': 'total distance traveled by the vehicle',
  'oxygen_sensor_1_afr': 'oxygen sensor 1 air-fuel equivalence ratio',
  'oxygen_sensor_1_voltage': 'oxygen sensor 1 voltage',
  'oxygen_sensor_2_short_term_fuel_trim': 'oxygen sensor 2 fuel trim',
  'oxygen_sensor_2_voltage': 'oxygen sensor 2 voltage',
  'oxygen_sensor_5_afr': 'oxygen sensor 5 air-fuel equivalence ratio',
  'oxygen_sensor_5_voltage': 'oxygen sensor 5 voltage',
  'oxygen_sensor_6_short_term_fuel_trim':
      'oxygen sensor 6 short term fuel trim',
  'oxygen_sensor_6_voltage': 'oxygen sensor 6 voltage',
  'runtime': 'run time since engine start',
  'short_term_fuel_trim_bank_1': 'short term fuel trim - bank 1',
  'short_term_fuel_trim_bank_2': 'short term fuel trim - bank 2',
  'short_term_oxygen_trim_bank_1': 'short term oxygen trim bank 1',
  'short_term_oxygen_trim_bank_2': 'short term oxygen trim bank 2',
  'short_term_oxygen_trim_bank_3': 'short term oxygen trim bank 3',
  'short_term_oxygen_trim_bank_4': 'short term oxygen trim bank 4',
  'speed': 'speed determined by GNSS',
  'throttle_B': 'throttle position B',
  'throttle_position': 'throttle position',
  'timing_advance': 'timing advance (in degree before top dead centre)',
  'vehicle_speed': 'vehicle travel speed',
  'vertical_speed': 'vertical speed determined by GNSS',
  'warmups': 'warm ups since codes cleared',
};

Map<String, String> vehicleParameterToUnits = {
  'absolute_load_value': '%',
  'accel_x': 'm/s/s',
  'accel_y': 'm/s/s',
  'accel_z': 'm/s/s',
  'accelerator_D': '%',
  'accelerator_E': '%',
  'altitude': 'm',
  'avg_range': 'km',
  'barometer': 'kPa',
  'catalyst_temp_bank_1_sensor_1': '°C',
  'catalyst_temp_bank_2_sensor_1': '°C',
  'commanded_afr': '%',
  'commanded_EGR': '%',
  'control_voltage': 'V',
  'distance_with_clear': 'km',
  'distance_with_MIL': 'km',
  'EGR_error': '%',
  'engine_coolant_temp_1': '°C',
  'engine_coolant_temp_2': '°C',
  'engine_friction': '%',
  'engine_load': '%',
  'engine_speed': 'rpm',
  'engine_torque_actual': '%',
  'engine_torque_reference': 'N m',
  'evap_system_vapor_pressure': 'Pa',
  'evaporative_purge': '%',
  'fuel_level': '%',
  'fuel_rail_gauge_pressure': 'kPa',
  'gyro_x': 'deg/s',
  'gyro_y': 'deg/s',
  'gyro_z': 'deg/s',
  'heading': 'deg',
  'intake_air_temp_1': '°C',
  'intake_air_temp_2': '°C',
  'intake_manifold_pressure': 'kPa',
  'latitude': 'deg',
  'long_term_fuel_trim_bank_1': '%',
  'long_term_fuel_trim_bank_2': '%',
  'long_term_oxygen_trim_bank_1': '%',
  'long_term_oxygen_trim_bank_2': '%',
  'long_term_oxygen_trim_bank_3': '%',
  'long_term_oxygen_trim_bank_4': '%',
  'longitude': 'deg',
  'mass_air_flow_sensor_A': 'g/s',
  'mass_air_flow_sensor_B': 'g/s',
  'odometer': 'km',
  'oxygen_sensor_1_afr': '1',
  'oxygen_sensor_1_voltage': 'V',
  'oxygen_sensor_2_short_term_fuel_trim': '%',
  'oxygen_sensor_2_voltage': 'V',
  'oxygen_sensor_5_afr': '1',
  'oxygen_sensor_5_voltage': 'V',
  'oxygen_sensor_6_short_term_fuel_trim': '%',
  'oxygen_sensor_6_voltage': 'V',
  'runtime': 's',
  'short_term_fuel_trim_bank_1': '%',
  'short_term_fuel_trim_bank_2': '%',
  'short_term_oxygen_trim_bank_1': '%',
  'short_term_oxygen_trim_bank_2': '%',
  'short_term_oxygen_trim_bank_3': '%',
  'short_term_oxygen_trim_bank_4': '%',
  'speed': 'm/s',
  'throttle_B': '%',
  'throttle_position': '%',
  'timing_advance': 'deg before TDC',
  'vehicle_speed': 'km/h',
  'vertical_speed': 'm/s',
  'warmups': '1',
};

Map<String, Icon> unitToIcon = {
  "m/s/s": const Icon(Icons.speed),
  "m:s": const Icon(Icons.speed),
  "km/h": const Icon(Icons.speed),
  "m/s": const Icon(Icons.speed),
  "V": const Icon(Icons.electric_bolt_outlined),
  "km": const Icon(Icons.turn_left_outlined),
  "m": const Icon(Icons.turn_left_outlined),
  "°C": const Icon(Icons.thermostat_outlined),
  "deg/s": const Icon(Icons.compass_calibration_outlined),
  "s": const Icon(Icons.timer_outlined),
  "kPa": const Icon(Icons.gas_meter_outlined),
  "Pa": const Icon(Icons.gas_meter_outlined),
  "%": const Icon(Icons.numbers)
};

Map<String, String> rawVehicleDataMapList = {
  'Please Select a Vehicle to Track': ''
};

class _MapWidgetState extends State<MapScreen> {
  Future<List<Map<String, String>>> fetchMultiVehicleDataDynamic() async {
    var response = await http
        .get(Uri.parse('https://api.datadrivenucsb.com/live_all_cars'));
    if (response.statusCode == 200) {
      // If the server did return a 200 OK response,
      // then parse the JSON.
      List<dynamic> vehiclesDataDynamic = jsonDecode(response.body);
      List<Map<String, String>> multiVehicleDataMap = [];
      vehiclesDataDynamic.forEach(
        (vehicleElement) {
          print("ve $vehicleElement");
          Map<String, String> vehicleDataMap = {};
          (vehicleElement as Map<String, dynamic>).forEach(
            (key, value) {
              vehicleDataMap[key.toString()] = value.toString();
            },
          );
          multiVehicleDataMap.add(vehicleDataMap);
        },
      );
      print(multiVehicleDataMap.length);
      return multiVehicleDataMap;
    } else {
      // If the server did not return a 200 OK response,
      // then throw an exception.
      throw Exception('Failed to load album');
    }
  }

  Map<String, int> latlongToMarkerId = {};
  late final Timer _timer;

  List<Map<String, String>> multiVehicleDataMap = [];
  Map<String, LatLng> carIdToLatLng = {};

  Future<List<Marker>> generateMarkersFromVehicleData() async {
    List<Map<String, String>> multiVehicleDataMap =
        await fetchMultiVehicleDataDynamic();
    print("multiVehicleDataMap $multiVehicleDataMap");

    _markers = <Marker>[];
    latlongToMarkerId = {};

    int i = 0;
    for (var vehicleData in multiVehicleDataMap) {
      latlongToMarkerId["${vehicleData['lat']}:${vehicleData['lng']}"] = i;
      carIdToLatLng[vehicleData['car_id']!] = LatLng(
        double.parse(vehicleData['lat']!),
        double.parse(vehicleData['lng']!),
      );
      i += 1;
    }
    print("latlongtomarkersid: $latlongToMarkerId");

    for (var vehicleData in multiVehicleDataMap) {
      if (vehicleData["car_id"] == selectedCarId && followSelectedVehicle) {
        mapController.move(
            LatLng(double.parse(vehicleData['lat']!),
                double.parse(vehicleData['lng']!)),
            17);
      }

      selectedCarId == ""
          ? rawVehicleDataMapList = {'Please Select a Vehicle to Track': ''}
          : rawVehicleDataMapList = multiVehicleDataMap[selectedMarkerIdx];

      _markers.add(
        Marker(
          point: LatLng(double.parse(vehicleData['lat']!),
              double.parse(vehicleData['lng']!)),
          width: 200,
          height: 70,
          builder: (context) => Column(
            crossAxisAlignment: CrossAxisAlignment.center,
            children: [
              Expanded(
                child: Tooltip(
                  message: "Last updated ${vehicleData['atTime']} PDT",
                  child: Stack(
                    alignment: Alignment.center,
                    children: [
                      Stack(
                        alignment: Alignment.center,
                        children: [
                          Container(
                            decoration: BoxDecoration(
                              shape: BoxShape.circle,
                              color: Colors.white,
                            ),
                            height: 30,
                            width: 30,
                          ),
                          Container(
                            decoration: BoxDecoration(
                              shape: BoxShape.circle,
                            ),
                            child: Material(
                              color: (selectedCarId == vehicleData["car_id"])
                                  ? Colors.green
                                  : Colors.black,
                              shape: const CircleBorder(),
                              child: InkWell(
                                splashColor: Colors.white,
                                onTap: () {
                                  print("ONTAP STUCK");
                                  if (selectedCarId == vehicleData["car_id"]) {
                                    selectedCarId = ""; // unselect
                                    rawVehicleDataMapList = {
                                      'Please Select a Vehicle to Track': ''
                                    };
                                    vehicleParametersOnSidebar =
                                        Map.from(rawVehicleDataMapList);
                                  } else {
                                    print(
                                        "BEORE selectedMarkerIdx $selectedMarkerIdx");
                                    selectedMarkerIdx = latlongToMarkerId[
                                        "${vehicleData['lat']}:${vehicleData['lng']}"]!;
                                    print(
                                        "BEFORE rawVehicleDataMapList $rawVehicleDataMapList");
                                    rawVehicleDataMapList =
                                        multiVehicleDataMap[selectedMarkerIdx];
                                    print(
                                        "rawVehicleDataMapList $rawVehicleDataMapList");

                                    selectedCarId = vehicleData["car_id"]!;
                                    vehicleParametersOnSidebar =
                                        Map.from(rawVehicleDataMapList);
                                  }
                                  setState(() {
                                    selectedCarId;
                                    rawVehicleDataMapList;
                                    vehicleParametersOnSidebar;
                                  });
                                },
                                customBorder: const CircleBorder(),
                                child: Ink(
                                  decoration: const BoxDecoration(
                                      shape: BoxShape.circle),
                                  height: 20,
                                  width: 20,
                                ),
                              ),
                            ),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              Text(
                carIDtoFriendlyName[vehicleData['car_id']]!,
                textAlign: TextAlign.center,
                style: TextStyle(
                  background: Paint()
                    ..color = Colors.grey[50]!
                    ..strokeWidth = 17
                    ..strokeJoin = StrokeJoin.round
                    ..strokeCap = StrokeCap.round
                    ..style = PaintingStyle.stroke,
                ),
              ),
            ],
          ),
        ),
      );
    }
    print("generated markers $_markers");
    return _markers;
  }

  @override
  void initState() {
    super.initState();
    _markers = [];
    _timer = Timer.periodic(const Duration(seconds: 5), (_) {
      setState(() {
        generateMarkersFromVehicleData();
      });
    });
  }

  @override
  void dispose() {
    super.dispose();
    _timer.cancel();
  }

  TextEditingController vehicleParameterSearchController =
      TextEditingController();

  void updateSideBarWithSearch(String query) {
    vehicleParametersOnSidebar =
        Map.from(vehicleParametersOnSidebar = rawVehicleDataMapList);
    vehicleParametersOnSidebar.removeWhere(
      (parametername, parametervalue) {
        if (vehicleParameterToFriendlyName.containsKey(parametername)) {
          return !(vehicleParameterToFriendlyName[parametername]!
                  .contains(query) ||
              parametername.contains(query));
        } else {
          return !parametername.contains(query);
        }
      },
    );
    setState(() {
      vehicleParametersOnSidebar;
    });
  }

  MapController mapController = MapController();
  Map<String, String> vehicleParametersOnSidebar = {};

  bool followSelectedVehicle = false;

  String tryParseStrToDoubleAndRound(String s) {
    try {
      final double d = double.parse(s);
      return d.toStringAsFixed(3);
    } catch (e) {
      return s;
    }
  }

  Widget generatePleaseRotateView() {
    return Container(
      alignment: Alignment.center,
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(12),
        boxShadow: [
          BoxShadow(
            offset: Offset(0, 1),
            blurRadius: 5,
            color: Colors.black.withOpacity(0.3),
          ),
        ],
      ),
      child: Padding(
        padding: EdgeInsets.all(20),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          mainAxisSize: MainAxisSize.min,
          children: [
            Text("Please rotate your device to landscape"),
            SizedBox(height: 10),
            Icon(Icons.screen_rotation_outlined)
          ],
        ),
      ),
    );
  }

  Widget generateMap() {
    return FutureBuilder(
      future: generateMarkersFromVehicleData(),
      builder: ((context, snapshot) {
        if (snapshot.hasData) {
          return Stack(
            children: <Widget>[
              FlutterMap(
                mapController: mapController,
                options: MapOptions(
                  // Initialize map position
                  // Center at Freebirds (roughly middle of IV)
                  center: LatLng(34.413300, -119.855690),
                  zoom: 15.0,
                  maxZoom: 19.0,
                  // Since we're not using a touchscreen, disable multitouch
                  // features since it seems to interfere w rapid mouse clicks
                  interactiveFlags:
                      InteractiveFlag.drag | InteractiveFlag.pinchZoom,
                ),
                nonRotatedChildren: [
                  AttributionWidget.defaultWidget(
                    source: 'OpenStreetMap contributors',
                    onSourceTapped: () {},
                  ),
                ],
                children: [
                  TileLayer(
                      maxZoom: 19,
                      urlTemplate:
                          "https://tile.openstreetmap.org/{z}/{x}/{y}.png",
                      userAgentPackageName: 'com.datadriven.app'),
                  MarkerLayer(
                    markers: _markers,
                  ),
                ],
              ),
              Align(
                alignment: Alignment.centerRight,
                child: Padding(
                  padding: EdgeInsets.fromLTRB(10, 10, 10, 30),
                  child: Container(
                    width: 300,
                    child: ListView(
                      children: [
                        Card(
                          child: Row(
                            children: [
                              Expanded(
                                flex: 5,
                                child: Padding(
                                  padding: const EdgeInsets.symmetric(
                                      horizontal: 8, vertical: 8),
                                  child: TextField(
                                    controller:
                                        vehicleParameterSearchController,
                                    decoration: const InputDecoration(
                                      prefixIcon: Icon(Icons.search),
                                      border: OutlineInputBorder(),
                                      hintText: 'Search',
                                    ),
                                    onSubmitted: (value) {
                                      updateSideBarWithSearch(value);
                                    },
                                  ),
                                ),
                              ),
                              Expanded(
                                flex: 1,
                                child: FilledButton(
                                  onPressed: () => {
                                    updateSideBarWithSearch(
                                        vehicleParameterSearchController.text)
                                  },
                                  child:
                                      const Icon(Icons.manage_search_outlined),
                                  style: ButtonStyle(
                                    padding: MaterialStateProperty.all(
                                        const EdgeInsets.all(0)),
                                    backgroundColor:
                                        MaterialStateProperty.all(Colors.green),
                                    shape: MaterialStateProperty.all<
                                        RoundedRectangleBorder>(
                                      const RoundedRectangleBorder(
                                        borderRadius: BorderRadius.zero,
                                      ),
                                    ),
                                  ),
                                ),
                              ),
                              SizedBox(width: 8),
                            ],
                          ),
                        ),
                        for (var vehicleParam
                            in vehicleParametersOnSidebar.keys)
                          Card(
                            child: ListTile(
                              leading: vehicleParameterToUnits
                                          .containsKey(vehicleParam) &&
                                      unitToIcon.containsKey(
                                          vehicleParameterToUnits[vehicleParam])
                                  ? unitToIcon[
                                      vehicleParameterToUnits[vehicleParam]]
                                  : const Icon(Icons.numbers_outlined),
                              title: Text(vehicleParameterToUnits
                                      .containsKey(vehicleParam)
                                  ? "${tryParseStrToDoubleAndRound(rawVehicleDataMapList[vehicleParam]!)}${vehicleParameterToUnits[vehicleParam]}"
                                  : tryParseStrToDoubleAndRound(
                                      rawVehicleDataMapList[vehicleParam]!)),
                              subtitle: Text(vehicleParameterToFriendlyName
                                      .containsKey(vehicleParam)
                                  ? vehicleParameterToFriendlyName[
                                      vehicleParam]!
                                  : vehicleParam),
                            ),
                          ),
                        SizedBox(height: 20),
                      ],
                    ),
                  ),
                ),
              ),
              Align(
                alignment: Alignment.bottomLeft,
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    for (String carId in carIdToLatLng.keys)
                      Card(
                        child: Row(
                          mainAxisSize: MainAxisSize.min,
                          children: [
                            SizedBox(width: 10),
                            Container(
                              height: 10,
                              width: 10,
                              decoration: BoxDecoration(
                                shape: BoxShape.circle,
                                color: (selectedCarId == carId)
                                    ? Colors.green
                                    : Colors.black,
                              ),
                            ),
                            TextButton(
                              child: Text("${carIDtoFriendlyName[carId]!}"),
                              onPressed: () {
                                mapController.move(carIdToLatLng[carId]!, 17);
                              },
                            ),
                          ],
                        ),
                      ),
                    Card(
                      child: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Switch(
                            activeColor: Colors.green,
                            value: followSelectedVehicle,
                            onChanged: (value) {
                              if (value) {
                                mapController.move(
                                    LatLng(
                                        double.parse(
                                            rawVehicleDataMapList['lat']!),
                                        double.parse(
                                            rawVehicleDataMapList['lng']!)),
                                    mapController.zoom);
                              }
                              followSelectedVehicle = value;
                            },
                          ),
                          const Text(
                            "Center Selected Vehicle",
                            textAlign: TextAlign.center,
                          ),
                          const SizedBox(width: 10),
                        ],
                      ),
                    ),
                  ],
                ),
              ),
              Align(
                alignment: Alignment.topLeft,
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Card(
                      shape: const RoundedRectangleBorder(
                        borderRadius: BorderRadius.only(
                          topLeft: Radius.circular(5),
                          topRight: Radius.circular(5),
                        ),
                      ),
                      margin: const EdgeInsets.fromLTRB(10, 10, 10, 0),
                      child: IconButton(
                        onPressed: () {
                          if (mapController.zoom < 20) {
                            mapController.move(
                                mapController.center, mapController.zoom + 1);
                          } else {
                            ScaffoldMessenger.of(context).showSnackBar(
                              const SnackBar(
                                content:
                                    Text('Maximum zoom reached (API limit)'),
                              ),
                            );
                          }
                        },
                        icon: const Icon(Icons.add),
                      ),
                    ),
                    Card(
                      shape: const RoundedRectangleBorder(
                        borderRadius: BorderRadius.only(
                          bottomLeft: Radius.circular(5),
                          bottomRight: Radius.circular(5),
                        ),
                      ),
                      margin: EdgeInsets.fromLTRB(10, 0, 10, 10),
                      child: IconButton(
                        onPressed: () => mapController.move(
                            mapController.center, mapController.zoom - 1),
                        icon: const Icon(Icons.remove),
                      ),
                    ),
                  ],
                ),
              ),
            ],
          );
        } else {
          return const CircularProgressIndicator();
        }
      }),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      resizeToAvoidBottomInset: false,
      appBar: AppBar(
        title: const Text('Map'),
        backgroundColor: Colors.black,
        actions: [
          IconButton(
            onPressed: () {},
            icon: Image.asset("assets/images/logo_white.png"),
            iconSize: 70,
          )
        ],
      ),
      body: OrientationBuilder(
        builder: (context, orientation) {
          return orientation == Orientation.portrait
              ? generatePleaseRotateView()
              : generateMap();
        },
      ),
      drawer: Drawer(
        child: ListView(
          // Important: Remove any padding from the ListView.
          padding: EdgeInsets.zero,
          children: [
            DrawerHeader(
              decoration: BoxDecoration(
                color: Colors.black,
              ),
              child: Container(
                alignment: Alignment.center,
                child: Image.asset("assets/images/logo_white.png"),
              ),
            ),
            ListTile(
              title: const Text('Map'),
              onTap: () {
                // Update the state of the app
                // ...
                // Then close the drawer
                Navigator.pop(context);
              },
            ),
            ListTile(
              title: const Text('Data Viz'),
              onTap: () {
                Navigator.push(
                    context,
                    MaterialPageRoute(
                        builder: (context) => const Statistics()));
              },
            ),
          ],
        ),
      ),
    );
  }
}
