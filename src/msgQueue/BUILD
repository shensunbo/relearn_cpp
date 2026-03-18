cc_library(
    name = "msgqueue",
    hdrs = [
        "Message.h",
        "MessageQueue.h",
        "MsgDefine.h",
        "MsgUtils.h",
    ],
    # srcs = [

    # ],
    deps = [
        "//include:mylog",
    ],
    includes = ["../../include"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "test",
    srcs = [
        "msgQueueTest.cpp",
    ],
    deps = [
        ":msgqueue",
        "//include:mylog",
    ],
    includes = ["../../include"],
)