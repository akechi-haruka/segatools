shared_library(
    'amvideoex',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'amvideoex.def',
    c_pch : '../../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
    ],
    link_with : [
        board_lib,
        hooklib_lib,
        platform_lib,
        util_lib,
    ],
    sources : [
        'dllmain.c'
    ],
)
