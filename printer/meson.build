printer_lib = static_library(
    'printer',
    include_directories : inc,
    implicit_include_directories : false,
    c_pch : '../precompiled.h',
    dependencies : [
        capnhook.get_variable('hook_dep'),
    ],
    link_with : [],
    sources : [
        'printer.c',
        'printer.h',
        'printer-stdcall.c',
        'printer-stdcall.h',
    ],
)
