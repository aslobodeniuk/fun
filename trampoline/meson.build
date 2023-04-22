project ('iface-trampoline', 'c', version : '0.0',
                     meson_version : '>= 0.54.0',
                     default_options : ['buildtype=debugoptimized',
                                        'warning_level=1'] )

trampoline_dep = []

iface_name = 'my_interface'
iface_deps = []
iface_args = ['-Wfatal-errors', '-Wall', '-Werror']
iface_sources = files(['my-interface.c'])


subdir ('iftr')


executable('iftr_test', 'test.c',
                 install: false,
                 link_with: [trampoline_dep])
