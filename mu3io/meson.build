mu3io_lib = static_library(
    'mu3io',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        xinput_lib,
    ],
	link_with : [
	    api_lib,
        util_lib,
	],
    sources : [
        'mu3io.c',
        'mu3io.h',
		'config.c',
        'config.h',
    ],
)
