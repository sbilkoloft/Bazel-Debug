android_sdk_repository(
    name = "androidsdk",
    path = "/path/to/Android/Sdk",
    build_tools_version = "23.0.2",
    api_level = 23,
)

android_ndk_repository(
    name = "androidndk",
    path = "/path/to/Android/Sdk/ndk-bundle",
    api_level = 21,
)

bind(
    name = "android_sdk_for_testing",
    actual = "//:dummy",
)

bind(
    name = "android_ndk_for_testing",
    actual = "//:dummy",
)

new_http_archive(
    name = "firebase",
    url = "https://dl.google.com/firebase/sdk/cpp/firebase_cpp_sdk_1.2.1.zip",
    build_file = "build_files/firebase.BUILD",
    strip_prefix = "firebase_cpp_sdk"
)