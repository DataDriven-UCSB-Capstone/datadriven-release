import 'package:datadriven/main_screens/log_in_screen.dart';
import 'package:datadriven/main_screens/map_screen.dart';
import 'package:firebase_core/firebase_core.dart';
import 'package:flutter/material.dart';

import 'main_screens/sign_up_screen.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  await Firebase.initializeApp(
      options: const FirebaseOptions(
          apiKey: "",
          projectId: "",
          messagingSenderId: "",
          appId: ""));
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
        debugShowCheckedModeBanner: false,
        title: 'DataDriven Admin',
        routes: {
          '/signup': (context) => SignUpScreen(),
          '/Maps': (context) => const MapScreen(),
          '/home': (context) => const LogIn(),
        },
        theme: ThemeData(
          primarySwatch: Colors.blue,
        ),
        home: const LogIn());
  }
}
