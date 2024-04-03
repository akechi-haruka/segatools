shared_library(
    'synchook',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    vs_module_defs : 'synchook.def',
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep')
    ],
    link_with : [
        hooklib_lib,
		platform_lib,
        syncio_dll,
        util_lib,
    ],
    sources : [
        'config.c',
        'config.h',
        'dllmain.c'
    ],
)
