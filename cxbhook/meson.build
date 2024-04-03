shared_library(
    'cxbhook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    vs_module_defs : 'cxbhook.def',
    link_with : [
        aimeio_lib,
        amex_lib,
        jvs_lib,
        board_lib,
        hooklib_lib,
        platform_lib,
        util_lib,
    ],
    sources : [
        'config.c',
        'config.h',
        'commio.c',
        'commio.h',
        'dllmain.c',
    ],
)
