# Bazel-Debug

NOTE: make sure to change WORKSPACE to point to proper Android SDK & NDK.

Sample app to help debug Bazel issues with Android SDK & NDK.

Build:
bazel build //project/java/native_activity:myapp --cxxopt='-std=c++11' -c dbg --spawn_strategy=standalone

