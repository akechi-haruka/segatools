chuniio_lib = static_library(
    'chuniio',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    link_with : [
        api_lib,
        util_lib,
    ],
    sources : [
        'chuniio.c',
        'chuniio.h',
        'config.c',
        'config.h',
    ],
)
