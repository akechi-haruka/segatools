project('segatools', 'c', version: '0.1.0')

add_project_arguments(
    '-DCOBJMACROS',
    '-DDIRECTINPUT_VERSION=0x0800',
    '-DWIN32_LEAN_AND_MEAN',
    '-D_WIN32_WINNT=_WIN32_WINNT_WIN8',
    '-DMINGW_HAS_SECURE_API=1',
    '-fdiagnostics-color=never',
    language: 'c',
)

# Use get_argument_syntax() instead once Meson 0.49.0 releases
if meson.get_compiler('c').get_id() != 'msvc'
    add_project_arguments(
        '-ffunction-sections',
        '-fdata-sections',
        '-Wno-unknown-pragmas',
        language: 'c',
    )

    add_project_link_arguments(
        '-Wl,--enable-stdcall-fixup',
        '-Wl,--exclude-all-symbols',
        '-Wl,--gc-sections',
        '-Wl,-Bstatic',
        '-static-libgcc',
        '-static',
        language: 'c',
    )
endif

cc = meson.get_compiler('c')
shlwapi_lib = cc.find_library('shlwapi')
dinput8_lib = cc.find_library('dinput8')
dxguid_lib = cc.find_library('dxguid')
xinput_lib = cc.find_library('xinput')
ws2_lib = cc.find_library('ws2_32')
dbghlp_lib = cc.find_library('dbghelp')
iphlpapi_lib = cc.find_library('iphlpapi')
pthread_lib = cc.find_library('pthread', static: true)
thread_dep = dependency('threads')

inc = include_directories('.')

capnhook = subproject('capnhook')

subdir('util')
subdir('api')
subdir('amex')
subdir('iccard')
subdir('board')
subdir('hooklib')
subdir('jvs')
subdir('platform')
subdir('namco')

subdir('aimeio')
subdir('chuniio')
subdir('divaio')
subdir('idzio')
subdir('mu3io')
subdir('syncio')
subdir('fgoio')

subdir('printer')

subdir('chunihook')
subdir('chusanhook')
subdir('divahook')
subdir('idzhook')
subdir('mu3hook')
subdir('synchook')
subdir('mkhook')
subdir('fgohook')
subdir('cxbhook')
subdir('minihook')
subdir('kemonohook')
#subdir('exdlls/printerex320a')
#subdir('exdlls/printerex320fwdl')
subdir('exdlls/amvideoex')

subdir('gfxhook')
subdir('carolio')
subdir('carolhook')
