# Bazel-Debug

NOTE: make sure to change WORKSPACE to point to proper Android SDK & NDK.

Sample app to help debug Bazel issues with Android SDK & NDK.

Problem: Need to add firebase.

Build:
bazel build //project/java/native_activity:myapp --cxxopt='-std=c++11' -c dbg --spawn_strategy=standalone

Error:

When running the app you will get this error in the logs:

W/System.err( 1607): java.lang.IllegalStateException: A required meta-data tag in your app's AndroidManifest.xml does not exist.  You must have the following declaration within the <application> element:     &lt meta-data android:name="com.google.android.gms.version" android:value="@integer/google_play_services_version" &gt
