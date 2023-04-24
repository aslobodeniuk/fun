# Interface trampoline

## Introduction

"Interface trampoline" aka IFTR - is a _yet another approach_ on the performance-critical code.

We all love liborc and consider it to be a great solution for performance-critical loops,
however sometimes it seems to be quite romantic for cases like implementing complicated
projects, such as a video encoder if you have to do that in a tight time schedule.

This project proposes a compromise between coding in fine-tuned C and support of the CPU
features at the same time.

## Description

The idea is to compile files with the performance critical code into various instances,
one per option on enabled CPU features:

example
```
my-interface.c --> (cc -O0 -g   )  my_interface_debug.o
               --> (cc -O3      )  my_interface_default.o  ---> IFTR ---> users code
               --> (cc -O3 -msse)  my_interface_sse.o
	       --> (cc -O3 -mavx)  my_interface_avx.o
```

## Usage

User of the "Interface trampoline" has to do:

1. Define an interface with performance-critical functions, and mark it with IFTR_IFACE
macro:

*my-interface.h:*
```c
typedef struct _MyInterface
{
  void (*my_memcpy) (void *restrict dst, const void *restrict src, int bytes);
  void (*my_float_multiply) (float *restrict dst, const float *restrict src,
      int floats);
} MyInterface;
```

*my-interface.c:*
```c
/* .. implementation of the functions here */

IFTR_IFACE (MyInterface,
    IFTR_FUNCTION (my_memcpy),
    IFTR_FUNCTION (my_float_multiply)
);
```

2. Connect the interface to it's code:

*test.c:*
```c
/* .. */

IFTR_TRAMPOLINE_IFACE (MyInterface);

int main() {
  MyInterface *iface;
  iface = IFTR_GET_IFACE (MyInterface);
  iface->my_memcpy (dst, src, 1024 * sizeof (float));
/*...*/
}
```

3. Integrate the trampoline into meson.build:

*meson.build:*

```meson
trampoline_dep = []

iface_name = 'my_interface'
iface_deps = []
iface_args = ['-Wfatal-errors', '-Wall', '-Werror']
iface_sources = files(['my-interface.c'])


subdir ('iftr')


executable('iftr_test', 'test.c',
                 install: false,
                 link_with: [trampoline_dep])
```

## FAQ

### What if I want to use the same type for various interfaces?

To trampoline various interfaces that share the same type you should use the
typedef and define another type name:

```c
typedef MySharedInterfaceType MyInterfaceTypeX;
```
