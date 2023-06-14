# datadriven Web-App

## Running Locally
First, make sure flutter is installed. Here are the steps: https://docs.flutter.dev/get-started/install

The web-app also depends on our API hosted on AWS at [api.datadrivenucsb.com](api.datadrivenucsb.com). Ensure it is running.

To run locally, run the following command
```
flutter run -d web-server
```

## Deploying
Once changes are final, you can deploy the changes to the production environment at Firebase at [datadrivenucsb.com](datadrivenucsb.com). Make sure the Firebase CLI is installed: https://firebase.google.com/docs/cli#setup_update_cli

Run this command in the project directory to set up Firebase locally.
```
firebase init hosting 
```

You'll see something like this in the console:
```
     ######## #### ########  ######## ########     ###     ######  ########
     ##        ##  ##     ## ##       ##     ##  ##   ##  ##       ##
     ######    ##  ########  ######   ########  #########  ######  ######
     ##        ##  ##    ##  ##       ##     ## ##     ##       ## ##
     ##       #### ##     ## ######## ########  ##     ##  ######  ########

You're about to initialize a Firebase project in this directory:

  /home/arjun/Downloads/ucsb/datadriven/Web-App

Before we get started, keep in mind:

  * You are initializing within an existing Firebase project directory


=== Account Setup

Which account do you want to use for this project? Choose an account or add a new one now

? Please select an option: (Use arrow keys)
❯ arjunvinod@ucsb.edu 
  (add a new account) 
```

Choose `add a new account` if this is your first time setting up Firebase locally. This will take you to the Google sign-on page and log in with your UCSB credentials.

Then you'll be asked to select a Firebase project:
```
=== Project Setup

First, let's associate this project directory with a Firebase project.
You can create multiple project aliases by running firebase use --add, 
but for now we'll just set up a default project.

? Please select an option: (Use arrow keys)
❯ Use an existing project 
  Create a new project 
  Add Firebase to an existing Google Cloud Platform project 
  Don't set up a default project 
```
Select `Use an existing project`. It will then ask you for a default Firebase project.
```
? Select a default Firebase project for this directory: (Use arrow keys)
❯ datadriven-d30bd (datadriven) 
```

Choose `datadriven-d30bd`.

Finally, it'll ask you for the hosting setup.
```
=== Hosting Setup

Your public directory is the folder (relative to your project directory) that
will contain Hosting assets to be uploaded with firebase deploy. If you
have a build process for your assets, use your build's output directory.

? What do you want to use as your public directory? (public) 
```
Enter `build/web`. Hit enter to choose the default options for the remaining questions.

```
? Configure as a single-page app (rewrite all urls to /index.html)? No
? Set up automatic builds and deploys with GitHub? No
? File build/web/404.html already exists. Overwrite? No
i  Skipping write of build/web/404.html
? File build/web/index.html already exists. Overwrite? No
i  Skipping write of build/web/index.html

i  Writing configuration info to firebase.json...
i  Writing project information to .firebaserc...

✔  Firebase initialization complete!
```
Firebase is now set up! To build and deploy it to [datadrivenucsb.com](datadrivenucsb.com):
```
flutter clean
flutter build web --release
firebase deploy
```

Check out the Firebase hosting console here: https://console.firebase.google.com/project/datadriven-d30bd/hosting/sites

You will see the release history if you scroll down the page a little and should see your latest push there. The website takes a few minutes to update and be accessible.