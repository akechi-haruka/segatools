shared_library(
    'mkhook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'mkhook.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep'),
        iphlpapi_lib,
        xinput_lib
    ],
    link_with : [
        api_lib,
        hooklib_lib,
		platform_lib,
		board_lib,
		jvs_lib,
        util_lib,
        namco_lib
    ],
    sources : [
        'config.c',
        'config.h',
        'dllmain.c',
        'jvs.c',
        'jvs.h',
    ],
)
