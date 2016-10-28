package(default_visibility = ["//visibility:public"])

cc_library(
    name = 'main',
    srcs = ['main.cpp'],
    linkopts = ["-llog", "-landroid", "-lEGL", "-lGLESv1_CM"],
    deps = [
        '//third_party:nag',
        '@firebase//:admob'
    ],
    visibility = [
        '//visibility:public'
    ]
)
