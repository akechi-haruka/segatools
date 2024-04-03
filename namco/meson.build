namco_lib = static_library(
    'namco',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep')
    ],
    link_with : [
        api_lib,
        hooklib_lib,
        platform_lib,
        board_lib,
        jvs_lib,
        util_lib
    ],
    sources : [
        'bngrw.c',
        'bngrw.h',
        'config.c',
        'config.h',
        'keychip.c',
        'keychip.h',
        'najv.c',
        'najv.h',
        'namco-jvs-usb.c',
        'namco-jvs-usb.h',
        'strpcb.c',
        'strpcb.h',
    ],
)
