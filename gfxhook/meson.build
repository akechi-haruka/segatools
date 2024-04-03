gfxhook_lib = static_library(
    'gfxhook',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        dxguid_lib,
    ],
    link_with : [
        hooklib_lib,
        util_lib,
    ],
    sources : [
        'config.c',
        'config.h',
        'd3d9.c',
        'd3d9.h',
        'd3d11.c',
        'd3d11.h',
        'dxgi.c',
        'dxgi.h',
        'gfx.c',
        'gfx.h',
        'util.c',
        'util.h',
    ],
)
