syncio_dll = static_library(
    'syncio',
    name_prefix : '',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
        capnhook.get_variable('hooklib_dep')
    ],
	link_with : [
        hooklib_lib,
        board_lib,
        util_lib,
        api_lib,
        namco_lib,
	],
    sources : [
        'syncio.c',
        'syncio.h',
		'config.c',
        'config.h',
		'wajv.c',
        'wajv.h',
    ],
)
