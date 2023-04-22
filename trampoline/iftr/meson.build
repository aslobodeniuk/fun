trampoline_dep = []

all_backends = [
  ['DEBUG', ['-O0', '-g']],
  ['DEFAULT', ['-O3']],
  ['SSE', ['-O3', '-msse']],
  ['AVX', ['-O3', '-mavx512f']],
]

iface_result = []

foreach b : all_backends

  backend_name = b.get(0)
  backend_cflags = [b.get(1), '-DIFTR_BACKEND=' + backend_name]
  
  lib = static_library ('iftr_' + iface_name + '_' + backend_name,
                        sources: iface_sources,
                        c_args : iface_args + backend_cflags,
                        dependencies : iface_deps,
                        install : false)

  iface_result += [lib]
endforeach

actual_trampoline = static_library('iftr',
                                   sources: files('actual_trampoline.c'),
                                   include_directories : [],
                                   c_args: [],
                                   dependencies: [],
                                   install: false
                                  )


trampoline_dep += [actual_trampoline, iface_result]
