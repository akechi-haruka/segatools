fgoio_lib = static_library(
    'fgoio',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        xinput_lib,
    ],
    sources : [
        'fgoio.c',
        'fgoio.h',
        'config.c',
        'config.h',
    ],
)
