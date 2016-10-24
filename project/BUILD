package(default_visibility = ["//visibility:public"])

cc_library(
    name = 'main',
    srcs = ['main.cpp'],
    linkopts = ["-llog", "-landroid"],
    deps = [
        '//third_party:nag',
        '@firebase//:admob'
    ],
    visibility = [
        '//visibility:public'
    ]
)
