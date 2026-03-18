cc_binary(
    name = "attribute_test",
    srcs = [
        "attribute_test.cpp",
        "Attribute.h",
    ],
    deps = [
        ":attribute",
        "//include:mylog",
    ],
    includes = ["../../include"],
)

cc_library(
    name = "attribute",
    hdrs = [
        "Attribute.h",
    ],
    visibility = ["//visibility:public"],
)