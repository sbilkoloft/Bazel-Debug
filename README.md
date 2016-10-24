# Bazel-Debug

Sample app to help debug Bazel issues with Android SDK & NDK.

Problem: Need to add firebase.

Build: bazel build //project/java/native_activity:myapp --cxxopt="-std=c++11"

NOTE: make sure to change WORKSPACE to point to proper Android SDK & NDK.
