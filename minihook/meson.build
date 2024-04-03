shared_library(
    'minihook',
    name_prefix : '',
    include_directories: inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    vs_module_defs : 'minihook.def',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    link_with : [
        aimeio_lib,
        amex_lib,
        board_lib,
        hooklib_lib,
        platform_lib,
        util_lib,
        jvs_lib,
    ],
    sources : [
        'dllmain.c',
    ],
)
