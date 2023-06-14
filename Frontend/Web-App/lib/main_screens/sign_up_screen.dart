import 'package:flutter/material.dart';
import 'package:firebase_auth/firebase_auth.dart';

class SignUpScreen extends StatefulWidget {
  @override
  _SignUpScreenState createState() => _SignUpScreenState();
}

class _SignUpScreenState extends State<SignUpScreen> {
  final GlobalKey<FormState> _formKey = GlobalKey<FormState>();
  late String _email, _password, _name;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Sign up'),
        backgroundColor: Colors.black,
      ),
      body: SingleChildScrollView(
        child: Form(
          key: _formKey,
          child: Padding(
            padding:
                const EdgeInsets.symmetric(vertical: 50.0, horizontal: 500),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: <Widget>[
                TextFormField(
                  decoration: InputDecoration(labelText: 'Email'),
                  validator: (input) =>
                      input!.isEmpty ? 'Please enter an email' : null,
                  onSaved: (input) => _email = input!.trim(),
                ),
                TextFormField(
                  decoration: InputDecoration(labelText: 'Password'),
                  validator: (input) => input!.length < 6
                      ? 'Password must be at least 6 characters'
                      : null,
                  onSaved: (input) => _password = input!.trim(),
                  obscureText: true,
                ),
                TextFormField(
                  decoration: InputDecoration(labelText: 'Name'),
                  validator: (input) =>
                      input!.isEmpty ? 'Please enter your name' : null,
                  onSaved: (input) => _name = input!.trim(),
                ),
                SizedBox(height: 20),
                Container(
                  height: 50,
                  child: ElevatedButton(
                    child: Text('Sign up'),
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.black,
                      foregroundColor: Colors.white,
                      shape: RoundedRectangleBorder(
                        borderRadius: BorderRadius.circular(10),
                      ),
                    ),
                    onPressed: signUp,
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }

  void signUp() async {
    final formState = _formKey.currentState;
    if (formState!.validate()) {
      formState.save();
      try {
        UserCredential user = await FirebaseAuth.instance
            .createUserWithEmailAndPassword(email: _email, password: _password);
        // Update the user's display name
        await user.user!.updateDisplayName(_name);
        // Navigate to the home screen
        Navigator.pushNamed(context, '/home');
      } catch (e) {
        print(e.toString());
      }
    }
  }
}
