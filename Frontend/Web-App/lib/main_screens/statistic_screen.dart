import 'dart:collection';

import 'package:datadriven/main_screens/map_screen.dart';
import 'package:multiselect/multiselect.dart';
import 'package:fl_chart/fl_chart.dart';
import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'dart:async';
import 'dart:convert';
import 'package:intl/intl.dart';
import 'dart:math' as math;

class Statistics extends StatefulWidget {
  const Statistics({super.key});

  @override
  State<Statistics> createState() => _StatisticsState();
}

class _StatisticsState extends State<Statistics> {
  List<String> validVehicleIds = [];
  // TODO more vehicle parameters
  // This is the only section that needs to be modified to add new params
  Set<String> validVehicleParameters = {
    "vehicle_speed",
    "engine_speed",
    "runtime",
  };
  List<FlSpot> vehicleSpeedHistory = [];
  Map<int, String> indexToDateTime = {};
  List<String> vehicleIdsSelected = [];
  bool vehicleIdIsSet = false;
  String? vehicleParameterDropDownValue;
  List<LineChartBarData> displayedData = [];
  Map<double, String> displayedDataTimeAxis = {0: ""};
  Map<double, String> displayedDataDateAxis = {0: ""};
  String displayedDataYAxis = "";

  Future<List<String>> fetchValidVehicleIds() async {
    var response =
        await http.get(Uri.parse('https://api.datadrivenucsb.com/cars'));
    if (response.statusCode == 200) {
      List<dynamic> validVehicleIdsDynamic = jsonDecode(response.body);
      //print(response.body);
      validVehicleIds = [];
      for (var element in validVehicleIdsDynamic) {
        //print(element);
        String vehicleId = element.toString();
        validVehicleIds.add(carIDtoFriendlyName.containsKey(vehicleId)
            ? carIDtoFriendlyName[vehicleId]!
            : vehicleId);
      }
      if (!vehicleIdIsSet) {
        print("setting initial vehicle id");
        vehicleIdIsSet = true;
      }
      return validVehicleIds;
    } else {
      throw Exception('Failed to load album');
    }
  }

  List<Color> dataColors = [
    Colors.blue,
    Colors.green,
    Colors.red,
    Colors.orange,
    Colors.purple,
    Colors.pink
  ];

  Map<String, Color> vehicleIdToColor = {};

  Future<List<LineChartBarData>> fetchMultiHistoricalVehicleData() async {
    displayedData = [];
    int i = 0;
    vehicleIdToColor = {};
    for (var friendlyCarId in vehicleIdsSelected) {
      String carId = carIDtoFriendlyName.entries
          .firstWhere((element) => element.value == friendlyCarId)
          .key;
      List<FlSpot> historicalVehicleData =
          await fetchHistoricalVehicleData(carId);
      displayedData.add(LineChartBarData(
        spots: historicalVehicleData,
        color: dataColors[i],
        barWidth: 4.0,
        belowBarData:
            BarAreaData(show: true, color: dataColors[i].withOpacity(0.15)),
      ));
      vehicleIdToColor[friendlyCarId] = dataColors[i];
      i = (i + 1) % dataColors.length;
    }
    setState(() {
      displayedData;
    });
    return displayedData;
  }

  // Fetch historical data for a vehicle
  Future<List<FlSpot>> fetchHistoricalVehicleData(String vehicleId) async {
    var response = await http.get(
        Uri.parse('https://api.datadrivenucsb.com/fetch_all/${vehicleId}'));
    if (response.statusCode == 200) {
      // If the server did return a 200 OK response,
      // then parse the JSON.
      print(
          "fetchall response for ${vehicleId} ${vehicleParameterDropDownValue}");
      List<dynamic> vehiclesDataDynamic = jsonDecode(response.body);
      vehicleSpeedHistory = [];
      validVehicleParameters = {};
      double i = 0;
      String lastUniqueDate = "";
      String vehicleDataPtDate = "";
      for (var element in vehiclesDataDynamic) {
        try {
          // Find which vehicle parameters are valid dynamically by checking
          // for which fields in the data points are null. Only need to do this
          // once.
          if (i == 0) {
            Map<String, dynamic> firstVehicleDataPt = vehiclesDataDynamic.first;
            Set<String> validVehicleParametersForThisVehicle = {};
            for (var key in firstVehicleDataPt.keys) {
              print("$vehicleId key $key");
              if (firstVehicleDataPt[key] != null) {
                validVehicleParametersForThisVehicle.add(key);
                validVehicleParameters.add(key);
              }
            }
            // If there are multiple vehicles being compared, we only want to provide
            // the option to compare parameters we have for both vehicles
            validVehicleParameters = validVehicleParameters
                .intersection(validVehicleParametersForThisVehicle);
            if (!validVehicleParameters
                .contains(vehicleParameterDropDownValue)) {
              vehicleParameterDropDownValue = validVehicleParameters.first;
            }
          }
          VehicleData vehicleDataPt = VehicleData.fromJson(element);
          if ((dateRange.start.isBefore(vehicleDataPt.atDate!) &&
              dateRange.end.isAfter(vehicleDataPt.atDate!))) {
            // Set y-axis label
            displayedDataYAxis = vehicleParameterToFriendlyName
                    .containsKey(vehicleParameterDropDownValue)
                ? "${vehicleParameterToFriendlyName[vehicleParameterDropDownValue]!}\n(${vehicleParameterToUnits[vehicleParameterDropDownValue]!})"
                : vehicleParameterDropDownValue!;

            // Add data point to dataset
            vehicleSpeedHistory
                .add(FlSpot(i, element[vehicleParameterDropDownValue]));

            // Set top x-axis values (date)
            // For breveity, we'll only display the date when it changes
            vehicleDataPtDate =
                DateFormat('MM-dd-yyyy').format(vehicleDataPt.atDate!);
            if (i == 0 || vehicleDataPtDate != lastUniqueDate) {
              displayedDataDateAxis[i] = vehicleDataPtDate;
              lastUniqueDate = vehicleDataPtDate;
            } else if (vehicleDataPtDate == lastUniqueDate) {
              displayedDataDateAxis[i] = "";
            }

            // Set bottom x-axis values (time)
            displayedDataTimeAxis[i] = vehicleDataPt.atTime.toString();

            i += 1;
          }
        } catch (e) {
          print(e);
        }
      }
      //print(vehicleSpeedHistory);
      setState(() {
        validVehicleParameters;
        vehicleSpeedHistory;
      });
      return vehicleSpeedHistory;
    } else {
      // If the server did not return a 200 OK response,
      // then throw an exception.
      throw Exception('Failed to load album');
    }
  }

  // Date Range picker
  DateTimeRange dateRange =
      DateTimeRange(start: DateTime(2023, 3, 1), end: DateTime(2023, 5, 3));
  void _showDateRangerPicker() async {
    final DateTimeRange? _dateRange = await showDateRangePicker(
      context: context,
      firstDate: DateTime(2022, 9, 1),
      lastDate: DateTime(2023, 9, 1),
      currentDate: DateTime.now(),
      builder: (context, child) {
        return Column(
          children: [
            Align(
              alignment: Alignment.center,
              child: ConstrainedBox(
                constraints: BoxConstraints(
                  maxWidth: 400.0,
                ),
                child: Theme(
                  data: ThemeData.dark(),
                  child: child!,
                ),
              ),
            )
          ],
        );
      },
    );

    if (_dateRange != null) {
      setState(() {
        dateRange = _dateRange;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Data Viz'),
        backgroundColor: Colors.black,
      ),
      body: Column(
        children: [
          SizedBox(height: 10),
          Row(
            children: [
              SizedBox(width: 50),
              Expanded(
                child: FutureBuilder(
                  future: fetchValidVehicleIds(),
                  builder: (context, snapshot) {
                    if (snapshot.hasData) {
                      print("rebuilt dropdown");
                      return DropDownMultiSelect(
                        hint: const Text("Vehicle ID(s)"),
                        onChanged: (List<String> x) {
                          setState(() {
                            vehicleIdsSelected = x;
                          });
                        },
                        options: validVehicleIds,
                        selectedValues: vehicleIdsSelected,
                      );
                    } else {
                      return Placeholder();
                    }
                  },
                ),
              ),
              SizedBox(width: 50),
              Expanded(
                child: Row(
                  children: [
                    Expanded(
                      child: FilledButton(
                        onPressed: () => _showDateRangerPicker(),
                        child: const Icon(Icons.calendar_month_outlined),
                        style: ButtonStyle(
                          backgroundColor:
                              MaterialStateProperty.all(Colors.black38),
                          shape:
                              MaterialStateProperty.all<RoundedRectangleBorder>(
                            RoundedRectangleBorder(
                              borderRadius: BorderRadius.zero,
                            ),
                          ),
                        ),
                      ),
                    ),
                    SizedBox(width: 50),
                    Expanded(
                      child: FilledButton(
                        onPressed: () => _showDateRangerPicker(),
                        child: const Icon(Icons.access_time_outlined),
                        style: ButtonStyle(
                          backgroundColor:
                              MaterialStateProperty.all(Colors.black38),
                          shape:
                              MaterialStateProperty.all<RoundedRectangleBorder>(
                            RoundedRectangleBorder(
                              borderRadius: BorderRadius.zero,
                            ),
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              SizedBox(width: 50),
              Expanded(
                child: DropdownButtonFormField<String>(
                  items: validVehicleParameters
                      .map(
                        (vehicleParameter) => DropdownMenuItem<String>(
                          value: vehicleParameter,
                          child: Text(vehicleParameterToFriendlyName
                                  .containsKey(vehicleParameter)
                              ? vehicleParameterToFriendlyName[
                                  vehicleParameter]!
                              : vehicleParameter),
                        ),
                      )
                      .toList(),
                  onChanged: (String? value) async {
                    // This is called when the user selects an item.
                    setState(() {
                      vehicleParameterDropDownValue = value!;
                    });
                  },
                  value: vehicleParameterDropDownValue,
                  hint: const Text("Data Parameter"),
                ),
              ),
              SizedBox(width: 50),
              FilledButton(
                onPressed: () async => await fetchMultiHistoricalVehicleData(),
                child: const Icon(Icons.manage_search_outlined),
                style: ButtonStyle(
                  backgroundColor: MaterialStateProperty.all(Colors.green),
                  shape: MaterialStateProperty.all<RoundedRectangleBorder>(
                    RoundedRectangleBorder(
                      borderRadius: BorderRadius.zero,
                    ),
                  ),
                ),
              ),
              SizedBox(width: 50),
            ],
          ),
          SizedBox(height: 10),
          Row(
            children: [
              SizedBox(width: 50),
              Card(
                child: Padding(
                  padding: EdgeInsets.all(20),
                  child: Center(
                    child: Text(
                      displayedDataYAxis,
                      style: TextStyle(
                        fontWeight: FontWeight.bold,
                      ),
                    ),
                  ),
                ),
              ),
              SizedBox(width: 10),
              Expanded(
                child: Card(
                  child: Container(
                    constraints: BoxConstraints(
                        maxHeight: MediaQuery.of(context).size.height * 5 / 8),
                    child: LineChart(
                      LineChartData(
                        lineBarsData: displayedData,
                        titlesData: FlTitlesData(
                          topTitles: AxisTitles(
                            axisNameWidget: vehicleIdsSelected.length == 1
                                ? const Text(
                                    "Date (MM-DD-YYYY)",
                                    style: TextStyle(
                                      fontWeight: FontWeight.bold,
                                    ),
                                  )
                                : null,
                            sideTitles: SideTitles(
                              showTitles: true,
                              getTitlesWidget: (value, meta) {
                                return SideTitleWidget(
                                  axisSide: meta.axisSide,
                                  child: Text(
                                    vehicleIdsSelected.length == 1
                                        ? displayedDataDateAxis[value]!
                                        : "",
                                  ),
                                );
                              },
                            ),
                          ),
                          bottomTitles: AxisTitles(
                            axisNameWidget: vehicleIdsSelected.length == 1
                                ? const Text(
                                    "Time (HH:MM:SS)",
                                    style: TextStyle(
                                      fontWeight: FontWeight.bold,
                                    ),
                                  )
                                : const Text(
                                    "Time (nth sample since first data point)",
                                    style: TextStyle(
                                      fontWeight: FontWeight.bold,
                                    ),
                                  ),
                            sideTitles: SideTitles(
                              interval: 20,
                              showTitles: true,
                              getTitlesWidget: (value, meta) {
                                return SideTitleWidget(
                                  axisSide: meta.axisSide,
                                  child: Text(
                                    vehicleIdsSelected.length == 1
                                        ? displayedDataTimeAxis[value]!
                                        : value.toString(),
                                  ),
                                );
                              },
                            ),
                          ),
                        ),
                      ),
                      swapAnimationDuration: Duration(milliseconds: 100),
                      swapAnimationCurve: Curves.linear,
                    ),
                  ),
                ),
              ),
            ],
          ),
          SizedBox(height: 20),
          Card(
            child: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                for (var id in vehicleIdToColor.keys)
                  Padding(
                    padding: EdgeInsets.all(20),
                    child: Text(
                      id,
                      style: TextStyle(
                        color: vehicleIdToColor[id],
                        fontWeight: FontWeight.bold,
                      ),
                      textAlign: TextAlign.center,
                    ),
                  ),
                SizedBox(width: 50),
              ],
            ),
          ),
        ],
      ),
    );
  }
}
