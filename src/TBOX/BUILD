cc_library(
    name = "TboxService",
    hdrs = [
        "TboxAidlHandler.h",
        "TboxService.h",
        "TboxFdbusHandler.h",
        "FdbusServerMock.h",
        "AidlClientMock.h",
    ],
    srcs = [
        "TboxAidlHandler.cpp",
        "TboxService.cpp",
        "TboxFdbusHandler.cpp",
        "AidlClientMock.cpp",
    ],
    deps = [
        "//src/attribute:attribute",
        "//include:mylog",
        "//src/msgQueue:msgqueue",
    ],
    includes = [
        "../../include",
        "../../src/attribute",
        "../../src/msgQueue",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "TboxSocketServer",
    hdrs = ["TboxSocketServer.h"],
    srcs = ["TboxSocketServer.cpp"],
    deps = [
        ":TboxService",
        "//include:mylog",
    ],
    includes = [
        "../../include",
        "../../src/attribute",
        "../../src/msgQueue",
        "../../src/TBOX",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "TboxSocketClient",
    hdrs = ["TboxSocketClient.h"],
    srcs = ["TboxSocketClient.cpp"],
    deps = ["//include:mylog"],
    includes = ["../../include"],
    visibility = ["//visibility:public"],
)

# TboxService server process — run this first
cc_binary(
    name = "tboxService",
    srcs = ["TboxMain.cpp"],
    deps = [
        ":TboxService",
        ":TboxSocketServer",
        "//include:mylog",
    ],
    includes = [
        "../../include",
        "../../src/attribute",
        "../../src/msgQueue",
        "../../src/TBOX",
    ],
)

# AIDL client — connects to tboxService via Unix socket
cc_binary(
    name = "aidlClient",
    srcs = ["AidlSim.cpp"],
    deps = [
        ":TboxSocketClient",
        "//include:mylog",
    ],
    includes = [
        "../../include",
        "../../src/TBOX",
    ],
)

# FdBus server simulator — connects to tboxService via Unix socket
cc_binary(
    name = "fdbusClient",
    srcs = ["FdbusSim.cpp"],
    deps = [
        ":TboxSocketClient",
        "//include:mylog",
    ],
    includes = [
        "../../include",
        "../../src/TBOX",
    ],
)