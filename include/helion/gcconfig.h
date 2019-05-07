/*
 * Copyright 1988, 1989 Hans-J. Boehm, Alan J. Demers
 * Copyright (c) 1991-1994 by Xerox Corporation.  All rights reserved.
 * Copyright (c) 1996 by Silicon Graphics.  All rights reserved.
 * Copyright (c) 2000-2004 Hewlett-Packard Development Company, L.P.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

/*
 * This header is private to the gc.  It is almost always included from
 * gc_priv.h.  However it is possible to include it by itself if just the
 * configuration macros are needed.  In that
 * case, a few declarations relying on types declared in gc_priv.h will be
 * omitted.
 */

#ifndef GCCONFIG_H
#define GCCONFIG_H

# define STACK_GRAN 0x1000000


# ifdef I386
#   define MACH_TYPE "I386"
#   if (defined(__LP64__) || defined(_WIN64)) && !defined(CPPCHECK)
#     error This should be handled as X86_64
#   else
#     define CPP_WORDSZ 32
#     define ALIGNMENT 4
                        /* Appears to hold for all "32 bit" compilers   */
                        /* except Borland.  The -a4 option fixes        */
                        /* Borland.  For Watcom the option is -zp4.     */
#   endif
#   ifdef SEQUENT
#       define OS_TYPE "SEQUENT"
        extern int etext[];
#       define DATASTART ((ptr_t)((((word)(etext)) + 0xfff) & ~0xfff))
#       define STACKBOTTOM ((ptr_t)0x3ffff000)
#   endif
#   if defined(__EMSCRIPTEN__)
#     define OS_TYPE "EMSCRIPTEN"
#     define DATASTART (ptr_t)ALIGNMENT
#     define DATAEND (ptr_t)ALIGNMENT
      /* Since JavaScript/asm.js/WebAssembly is not able to access the  */
      /* function call stack or the local data stack, it's not possible */
      /* for GC to perform its stack walking operation to find roots on */
      /* the stack.  To work around that, the clients generally only do */
      /* BDWGC steps when the stack is empty so it is known that there  */
      /* are no objects that would be found on the stack, and BDWGC is  */
      /* compiled with stack walking disabled.                          */
#     define STACK_NOT_SCANNED
#   endif
#   if defined(__QNX__)
#     define OS_TYPE "QNX"
#     define SA_RESTART 0
#     define HEURISTIC1
      extern char etext[];
      extern int _end[];
#     define DATASTART ((ptr_t)etext)
#     define DATAEND ((ptr_t)_end)
#   endif
#   ifdef HAIKU
#     define OS_TYPE "HAIKU"
      EXTERN_C_END
#     include <OS.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)B_PAGE_SIZE
      extern int etext[];
#     define DATASTART ((ptr_t)((((word)(etext)) + 0xfff) & ~0xfff))
#     define DYNAMIC_LOADING
#     define MPROTECT_VDB
#   endif
#   ifdef SOLARIS
#       define OS_TYPE "SOLARIS"
        extern int _etext[], _end[];
        ptr_t GC_SysVGetDataStart(size_t, ptr_t);
#       define DATASTART GC_SysVGetDataStart(0x1000, (ptr_t)_etext)
#       define DATASTART_IS_FUNC
#       define DATAEND ((ptr_t)(_end))
        /* # define STACKBOTTOM ((ptr_t)(_start)) worked through 2.7,   */
        /* but reportedly breaks under 2.8.  It appears that the stack  */
        /* base is a property of the executable, so this should not     */
        /* break old executables.                                       */
        /* HEURISTIC2 probably works, but this appears to be preferable.*/
        /* Apparently USRSTACK is defined to be USERLIMIT, but in some  */
        /* installations that's undefined.  We work around this with a  */
        /* gross hack:                                                  */
        EXTERN_C_END
#       include <sys/vmparam.h>
        EXTERN_C_BEGIN
#       ifdef USERLIMIT
          /* This should work everywhere, but doesn't.  */
#         define STACKBOTTOM ((ptr_t)USRSTACK)
#       else
#         define HEURISTIC2
#       endif
/* At least in Solaris 2.5, PROC_VDB gives wrong values for dirty bits. */
/* It appears to be fixed in 2.8 and 2.9.                               */
#       ifdef SOLARIS25_PROC_VDB_BUG_FIXED
#         define PROC_VDB
#       endif
#       ifndef GC_THREADS
#         define MPROTECT_VDB
#       endif
#       define DYNAMIC_LOADING
#       if !defined(USE_MMAP) && defined(REDIRECT_MALLOC)
#         define USE_MMAP 1
            /* Otherwise we now use calloc.  Mmap may result in the     */
            /* heap interleaved with thread stacks, which can result in */
            /* excessive blacklisting.  Sbrk is unusable since it       */
            /* doesn't interact correctly with the system malloc.       */
#       endif
#       ifdef USE_MMAP
#         define HEAP_START (ptr_t)0x40000000
#       else
#         define HEAP_START DATAEND
#       endif
#   endif
#   ifdef SCO
#       define OS_TYPE "SCO"
        extern int etext[];
#       define DATASTART ((ptr_t)((((word)(etext)) + 0x3fffff) & ~0x3fffff) \
                                 + ((word)(etext) & 0xfff))
#       define STACKBOTTOM ((ptr_t)0x7ffffffc)
#   endif
#   ifdef SCO_ELF
#       define OS_TYPE "SCO_ELF"
        extern int etext[];
#       define DATASTART ((ptr_t)(etext))
#       define STACKBOTTOM ((ptr_t)0x08048000)
#       define DYNAMIC_LOADING
#       define ELF_CLASS ELFCLASS32
#   endif
#   ifdef DGUX
#       define OS_TYPE "DGUX"
        extern int _etext, _end;
        ptr_t GC_SysVGetDataStart(size_t, ptr_t);
#       define DATASTART GC_SysVGetDataStart(0x1000, (ptr_t)(&_etext))
#       define DATASTART_IS_FUNC
#       define DATAEND ((ptr_t)(&_end))
#       define STACK_GROWS_DOWN
#       define HEURISTIC2
        EXTERN_C_END
#       include <unistd.h>
        EXTERN_C_BEGIN
#       define GETPAGESIZE() (unsigned)sysconf(_SC_PAGESIZE)
#       define DYNAMIC_LOADING
#       ifndef USE_MMAP
#         define USE_MMAP 1
#       endif
#       define MAP_FAILED (void *) ((word)-1)
#       define HEAP_START (ptr_t)0x40000000
#   endif /* DGUX */
#   ifdef LINUX
#       define OS_TYPE "LINUX"
#       define LINUX_STACKBOTTOM
#       if !defined(GC_LINUX_THREADS) || !defined(REDIRECT_MALLOC)
#           define MPROTECT_VDB
#       else
            /* We seem to get random errors in incremental mode,        */
            /* possibly because Linux threads is itself a malloc client */
            /* and can't deal with the signals.                         */
#       endif
#       define HEAP_START (ptr_t)0x1000
                /* This encourages mmap to give us low addresses,       */
                /* thus allowing the heap to grow to ~3 GB.             */
#       ifdef __ELF__
#           define DYNAMIC_LOADING
            EXTERN_C_END
#           include <features.h>
            EXTERN_C_BEGIN
#            if defined(__GLIBC__) && __GLIBC__ >= 2 \
                || defined(HOST_ANDROID) || defined(HOST_TIZEN)
#                define SEARCH_FOR_DATA_START
#            else
                 extern char **__environ;
#                define DATASTART ((ptr_t)(&__environ))
                              /* hideous kludge: __environ is the first */
                              /* word in crt0.o, and delimits the start */
                              /* of the data segment, no matter which   */
                              /* ld options were passed through.        */
                              /* We could use _etext instead, but that  */
                              /* would include .rodata, which may       */
                              /* contain large read-only data tables    */
                              /* that we'd rather not scan.             */
#            endif
             extern int _end[];
#            define DATAEND ((ptr_t)(_end))
#            if !defined(GC_NO_SIGSETJMP) && (defined(HOST_TIZEN) \
                    || (defined(HOST_ANDROID) \
                        && !(GC_GNUC_PREREQ(4, 8) || GC_CLANG_PREREQ(3, 2) \
                             || __ANDROID_API__ >= 18)))
               /* Older Android NDK releases lack sigsetjmp in x86 libc */
               /* (setjmp is used instead to find data_start).  The bug */
               /* is fixed in Android NDK r8e (so, ok to use sigsetjmp  */
               /* if gcc4.8+, clang3.2+ or Android API level 18+).      */
#              define GC_NO_SIGSETJMP 1
#            endif
#       else
             extern int etext[];
#            define DATASTART ((ptr_t)((((word)(etext)) + 0xfff) & ~0xfff))
#       endif
#       ifdef USE_I686_PREFETCH
#         define PREFETCH(x) \
            __asm__ __volatile__ ("prefetchnta %0" : : "m"(*(char *)(x)))
            /* Empirically prefetcht0 is much more effective at reducing     */
            /* cache miss stalls for the targeted load instructions.  But it */
            /* seems to interfere enough with other cache traffic that the   */
            /* net result is worse than prefetchnta.                         */
#         ifdef FORCE_WRITE_PREFETCH
            /* Using prefetches for write seems to have a slight negative    */
            /* impact on performance, at least for a PIII/500.               */
#           define GC_PREFETCH_FOR_WRITE(x) \
              __asm__ __volatile__ ("prefetcht0 %0" : : "m"(*(char *)(x)))
#         else
#           define GC_NO_PREFETCH_FOR_WRITE
#         endif
#       elif defined(USE_3DNOW_PREFETCH)
#         define PREFETCH(x) \
            __asm__ __volatile__ ("prefetch %0" : : "m"(*(char *)(x)))
#         define GC_PREFETCH_FOR_WRITE(x) \
            __asm__ __volatile__ ("prefetchw %0" : : "m"(*(char *)(x)))
#       endif
#       if defined(__GLIBC__) && !defined(__UCLIBC__)
          /* Workaround lock elision implementation for some glibc.     */
#         define GLIBC_2_19_TSX_BUG
          EXTERN_C_END
#         include <gnu/libc-version.h> /* for gnu_get_libc_version() */
          EXTERN_C_BEGIN
#       endif
#   endif
#   ifdef CYGWIN32
#       define OS_TYPE "CYGWIN32"
#       define DATASTART ((ptr_t)GC_DATASTART)  /* From gc.h */
#       define DATAEND   ((ptr_t)GC_DATAEND)
#       ifdef USE_WINALLOC
#         define GWW_VDB
#       else
#         define MPROTECT_VDB
#         ifdef USE_MMAP
#           define NEED_FIND_LIMIT
#           define USE_MMAP_ANON
#         endif
#       endif
#   endif
#   ifdef INTERIX
#     define OS_TYPE "INTERIX"
      extern int _data_start__[];
      extern int _bss_end__[];
#     define DATASTART ((ptr_t)_data_start__)
#     define DATAEND   ((ptr_t)_bss_end__)
#     define STACKBOTTOM ({ ptr_t rv; \
                            __asm__ __volatile__ ("movl %%fs:4, %%eax" \
                                                  : "=a" (rv)); \
                            rv; })
#     define USE_MMAP_ANON
#   endif
#   ifdef OS2
#       define OS_TYPE "OS2"
                /* STACKBOTTOM and DATASTART are handled specially in   */
                /* os_dep.c. OS2 actually has the right                 */
                /* system call!                                         */
#       define DATAEND  /* not needed */
#   endif
#   ifdef MSWIN32
#       define OS_TYPE "MSWIN32"
                /* STACKBOTTOM and DATASTART are handled specially in   */
                /* os_dep.c.                                            */
#       define MPROTECT_VDB
#       define GWW_VDB
#       define DATAEND  /* not needed */
#   endif
#   ifdef MSWINCE
#       define OS_TYPE "MSWINCE"
#       define DATAEND  /* not needed */
#   endif
#   ifdef DJGPP
#       define OS_TYPE "DJGPP"
        EXTERN_C_END
#       include "stubinfo.h"
        EXTERN_C_BEGIN
        extern int etext[];
        extern int _stklen;
        extern int __djgpp_stack_limit;
#       define DATASTART ((ptr_t)((((word)(etext)) + 0x1ff) & ~0x1ff))
/* #define STACKBOTTOM ((ptr_t)((word)_stubinfo+_stubinfo->size+_stklen)) */
#       define STACKBOTTOM ((ptr_t)((word)__djgpp_stack_limit + _stklen))
                /* This may not be right.  */
#   endif
#   ifdef OPENBSD
#       define OS_TYPE "OPENBSD"
#       ifndef GC_OPENBSD_THREADS
          EXTERN_C_END
#         include <sys/param.h>
#         include <uvm/uvm_extern.h>
          EXTERN_C_BEGIN
#         ifdef USRSTACK
#           define STACKBOTTOM ((ptr_t)USRSTACK)
#         else
#           define HEURISTIC2
#         endif
#       endif
        extern int __data_start[];
#       define DATASTART ((ptr_t)__data_start)
        extern int _end[];
#       define DATAEND ((ptr_t)(&_end))
#       define DYNAMIC_LOADING
#   endif
#   ifdef FREEBSD
#       define OS_TYPE "FREEBSD"
#       ifndef GC_FREEBSD_THREADS
#           define MPROTECT_VDB
#       endif
#       ifdef __GLIBC__
#           define SIG_SUSPEND          (32+6)
#           define SIG_THR_RESTART      (32+5)
            extern int _end[];
#           define DATAEND ((ptr_t)(_end))
#       else
#           define SIG_SUSPEND SIGUSR1
#           define SIG_THR_RESTART SIGUSR2
                /* SIGTSTP and SIGCONT could be used alternatively.     */
#       endif
#       define FREEBSD_STACKBOTTOM
#       ifdef __ELF__
#           define DYNAMIC_LOADING
#       endif
        extern char etext[];
#       define DATASTART GC_FreeBSDGetDataStart(0x1000, (ptr_t)etext)
#       define DATASTART_USES_BSDGETDATASTART
#   endif
#   ifdef NETBSD
#       define OS_TYPE "NETBSD"
#       ifdef __ELF__
#           define DYNAMIC_LOADING
#       endif
#   endif
#   ifdef THREE86BSD
#       define OS_TYPE "THREE86BSD"
#   endif
#   ifdef BSDI
#       define OS_TYPE "BSDI"
#   endif
#   if defined(NETBSD) || defined(THREE86BSD) || defined(BSDI)
#       define HEURISTIC2
        extern char etext[];
#       define DATASTART ((ptr_t)(etext))
#   endif
#   ifdef NEXT
#       define OS_TYPE "NEXT"
#       define DATASTART ((ptr_t)get_etext())
#       define DATASTART_IS_FUNC
#       define STACKBOTTOM ((ptr_t)0xc0000000)
#       define DATAEND  /* not needed */
#   endif
#   ifdef RTEMS
#       define OS_TYPE "RTEMS"
        EXTERN_C_END
#       include <sys/unistd.h>
        EXTERN_C_BEGIN
        extern int etext[];
        void *rtems_get_stack_bottom(void);
#       define InitStackBottom rtems_get_stack_bottom()
#       define DATASTART ((ptr_t)etext)
#       define STACKBOTTOM ((ptr_t)InitStackBottom)
#       define SIG_SUSPEND SIGUSR1
#       define SIG_THR_RESTART SIGUSR2
#   endif
#   ifdef DOS4GW
#     define OS_TYPE "DOS4GW"
      extern long __nullarea;
      extern char _end;
      extern char *_STACKTOP;
      /* Depending on calling conventions Watcom C either precedes      */
      /* or does not precedes with underscore names of C-variables.     */
      /* Make sure startup code variables always have the same names.   */
      #pragma aux __nullarea "*";
      #pragma aux _end "*";
#     define STACKBOTTOM ((ptr_t)_STACKTOP)
                         /* confused? me too. */
#     define DATASTART ((ptr_t)(&__nullarea))
#     define DATAEND ((ptr_t)(&_end))
#   endif
#   ifdef HURD
#     define OS_TYPE "HURD"
#     define STACK_GROWS_DOWN
#     define HEURISTIC2
#     define SIG_SUSPEND SIGUSR1
#     define SIG_THR_RESTART SIGUSR2
#     define SEARCH_FOR_DATA_START
      extern int _end[];
#     define DATAEND ((ptr_t)(_end))
/* #     define MPROTECT_VDB  Not quite working yet? */
#     define DYNAMIC_LOADING
#     define USE_MMAP_ANON
#   endif
#   ifdef DARWIN
#     define OS_TYPE "DARWIN"
#     define DARWIN_DONT_PARSE_STACK 1
#     define DYNAMIC_LOADING
      /* XXX: see get_end(3), get_etext() and get_end() should not be used. */
      /* These aren't used when dyld support is enabled (it is by default). */
#     define DATASTART ((ptr_t)get_etext())
#     define DATAEND   ((ptr_t)get_end())
#     define STACKBOTTOM ((ptr_t)0xc0000000)
#     define USE_MMAP_ANON
#     define MPROTECT_VDB
      EXTERN_C_END
#     include <unistd.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)getpagesize()
      /* There seems to be some issues with trylock hanging on darwin.  */
      /* This should be looked into some more.                          */
#     define NO_PTHREAD_TRYLOCK
#     if TARGET_OS_IPHONE && !defined(NO_DYLD_BIND_FULLY_IMAGE)
        /* iPhone/iPad simulator */
#       define NO_DYLD_BIND_FULLY_IMAGE
#     endif
#   endif /* DARWIN */
# endif


# ifdef IA64
#   define MACH_TYPE "IA64"
#   ifdef HPUX
#       ifdef _ILP32
#         define CPP_WORDSZ 32
            /* Requires 8 byte alignment for malloc */
#         define ALIGNMENT 4
#       else
#         if !defined(_LP64) && !defined(CPPCHECK)
#           error Unknown ABI
#         endif
#         define CPP_WORDSZ 64
            /* Requires 16 byte alignment for malloc */
#         define ALIGNMENT 8
#       endif
#       define OS_TYPE "HPUX"
        extern int __data_start[];
#       define DATASTART ((ptr_t)(__data_start))
        /* Gustavo Rodriguez-Rivera suggested changing HEURISTIC2       */
        /* to this.  Note that the GC must be initialized before the    */
        /* first putenv call.                                           */
        extern char ** environ;
#       define STACKBOTTOM ((ptr_t)environ)
#       define HPUX_STACKBOTTOM
#       define DYNAMIC_LOADING
        EXTERN_C_END
#       include <unistd.h>
        EXTERN_C_BEGIN
#       define GETPAGESIZE() (unsigned)sysconf(_SC_PAGE_SIZE)
        /* The following was empirically determined, and is probably    */
        /* not very robust.                                             */
        /* Note that the backing store base seems to be at a nice       */
        /* address minus one page.                                      */
#       define BACKING_STORE_DISPLACEMENT 0x1000000
#       define BACKING_STORE_ALIGNMENT 0x1000
        extern ptr_t GC_register_stackbottom;
#       define BACKING_STORE_BASE GC_register_stackbottom
        /* Known to be wrong for recent HP/UX versions!!!       */
#   endif
#   ifdef LINUX
#       define CPP_WORDSZ 64
#       define ALIGNMENT 8
#       define OS_TYPE "LINUX"
        /* The following works on NUE and older kernels:        */
/* #       define STACKBOTTOM ((ptr_t) 0xa000000000000000l)     */
        /* This does not work on NUE:                           */
#       define LINUX_STACKBOTTOM
        /* We also need the base address of the register stack  */
        /* backing store.                                       */
        extern ptr_t GC_register_stackbottom;
#       define BACKING_STORE_BASE GC_register_stackbottom
#       define SEARCH_FOR_DATA_START
#       ifdef __GNUC__
#         define DYNAMIC_LOADING
#       else
          /* In the Intel compiler environment, we seem to end up with  */
          /* statically linked executables and an undefined reference   */
          /* to _DYNAMIC                                                */
#       endif
#       define MPROTECT_VDB
                /* Requires Linux 2.3.47 or later.      */
        extern int _end[];
#       define DATAEND ((ptr_t)(_end))
#       ifdef __GNUC__
#         ifndef __INTEL_COMPILER
#           define PREFETCH(x) \
              __asm__ ("        lfetch  [%0]": : "r"(x))
#           define GC_PREFETCH_FOR_WRITE(x) \
              __asm__ ("        lfetch.excl     [%0]": : "r"(x))
#           define CLEAR_DOUBLE(x) \
              __asm__ ("        stf.spill       [%0]=f0": : "r"((void *)(x)))
#         else
            EXTERN_C_END
#           include <ia64intrin.h>
            EXTERN_C_BEGIN
#           define PREFETCH(x) __lfetch(__lfhint_none, (x))
#           define GC_PREFETCH_FOR_WRITE(x) __lfetch(__lfhint_nta, (x))
#           define CLEAR_DOUBLE(x) __stf_spill((void *)(x), 0)
#         endif /* __INTEL_COMPILER */
#       endif
#   endif
#   ifdef MSWIN32
      /* FIXME: This is a very partial guess.  There is no port, yet.   */
#     define OS_TYPE "MSWIN32"
                /* STACKBOTTOM and DATASTART are handled specially in   */
                /* os_dep.c.                                            */
#     define DATAEND  /* not needed */
#     if defined(_WIN64)
#       define CPP_WORDSZ 64
#     else
#       define CPP_WORDSZ 32   /* Is this possible?     */
#     endif
#     define ALIGNMENT 8
#   endif
# endif



# ifdef S390
#   define MACH_TYPE "S390"
#   ifndef __s390x__
#     define ALIGNMENT 4
#     define CPP_WORDSZ 32
#   else
#     define ALIGNMENT 8
#     define CPP_WORDSZ 64
#     ifndef HBLKSIZE
#       define HBLKSIZE 4096
#     endif
#   endif
#   ifdef LINUX
#       define OS_TYPE "LINUX"
#       define LINUX_STACKBOTTOM
#       define DYNAMIC_LOADING
        extern int __data_start[] __attribute__((__weak__));
#       define DATASTART ((ptr_t)(__data_start))
        extern int _end[] __attribute__((__weak__));
#       define DATAEND ((ptr_t)(_end))
#       define CACHE_LINE_SIZE 256
#       define GETPAGESIZE() 4096
#   endif
# endif

# ifdef AARCH64
#   define MACH_TYPE "AARCH64"
#   ifdef __ILP32__
#     define CPP_WORDSZ 32
#     define ALIGNMENT 4
#   else
#     define CPP_WORDSZ 64
#     define ALIGNMENT 8
#   endif
#   ifndef HBLKSIZE
#     define HBLKSIZE 4096
#   endif
#   ifdef LINUX
#     define OS_TYPE "LINUX"
#     define LINUX_STACKBOTTOM
#     if !defined(GC_LINUX_THREADS) || !defined(REDIRECT_MALLOC)
#       define MPROTECT_VDB
#     endif
#     define DYNAMIC_LOADING
#     if defined(HOST_ANDROID)
#       define SEARCH_FOR_DATA_START
#     else
        extern int __data_start[];
#       define DATASTART ((ptr_t)__data_start)
#     endif
      extern int _end[];
#     define DATAEND ((ptr_t)(&_end))
#   endif
#   ifdef DARWIN
      /* iOS */
#     define OS_TYPE "DARWIN"
#     define DARWIN_DONT_PARSE_STACK 1
#     define DYNAMIC_LOADING
#     define DATASTART ((ptr_t)get_etext())
#     define DATAEND   ((ptr_t)get_end())
#     define STACKBOTTOM ((ptr_t)0x16fdfffff)
#     define USE_MMAP_ANON
#     define MPROTECT_VDB
      EXTERN_C_END
#     include <unistd.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)getpagesize()
      /* FIXME: There seems to be some issues with trylock hanging on   */
      /* darwin. This should be looked into some more.                  */
#     define NO_PTHREAD_TRYLOCK
#     if TARGET_OS_IPHONE && !defined(NO_DYLD_BIND_FULLY_IMAGE)
#       define NO_DYLD_BIND_FULLY_IMAGE
#     endif
#   endif
#   ifdef FREEBSD
#     define OS_TYPE "FREEBSD"
#     ifndef GC_FREEBSD_THREADS
#       define MPROTECT_VDB
#     endif
#     define FREEBSD_STACKBOTTOM
#     ifdef __ELF__
#       define DYNAMIC_LOADING
#     endif
      extern char etext[];
#     define DATASTART GC_FreeBSDGetDataStart(0x1000, (ptr_t)etext)
#     define DATASTART_USES_BSDGETDATASTART
#   endif
#   ifdef NETBSD
#     define OS_TYPE "NETBSD"
#     define HEURISTIC2
      extern ptr_t GC_data_start;
#     define DATASTART GC_data_start
#     define ELF_CLASS ELFCLASS64
#     define DYNAMIC_LOADING
#   endif
#   ifdef NINTENDO_SWITCH
      extern int __bss_end[];
#     define NO_HANDLE_FORK 1
#     define DATASTART (ptr_t)ALIGNMENT /* cannot be null */
#     define DATAEND (ptr_t)(&__bss_end)
      void *switch_get_stack_bottom(void);
#     define STACKBOTTOM ((ptr_t)switch_get_stack_bottom())
#   endif
#   ifdef NOSYS
      /* __data_start is usually defined in the target linker script.   */
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
      extern void *__stack_base__;
#     define STACKBOTTOM ((ptr_t)__stack_base__)
#   endif
# endif




















# ifdef ARM32
#   if defined(NACL)
#     define MACH_TYPE "NACL"
#   else
#     define MACH_TYPE "ARM32"
#   endif
#   define CPP_WORDSZ 32
#   define ALIGNMENT 4
#   ifdef NETBSD
#       define OS_TYPE "NETBSD"
#       define HEURISTIC2
#       ifdef __ELF__
           extern ptr_t GC_data_start;
#          define DATASTART GC_data_start
#          define DYNAMIC_LOADING
#       else
           extern char etext[];
#          define DATASTART ((ptr_t)(etext))
#       endif
#   endif
#   ifdef LINUX
#       define OS_TYPE "LINUX"
#       define LINUX_STACKBOTTOM
#       if !defined(GC_LINUX_THREADS) || !defined(REDIRECT_MALLOC)
#           define MPROTECT_VDB
#       endif
#       ifdef __ELF__
#           define DYNAMIC_LOADING
            EXTERN_C_END
#           include <features.h>
            EXTERN_C_BEGIN
#           if defined(__GLIBC__) && __GLIBC__ >= 2 \
                || defined(HOST_ANDROID) || defined(HOST_TIZEN)
#                define SEARCH_FOR_DATA_START
#           else
                 extern char **__environ;
#                define DATASTART ((ptr_t)(&__environ))
                              /* hideous kludge: __environ is the first */
                              /* word in crt0.o, and delimits the start */
                              /* of the data segment, no matter which   */
                              /* ld options were passed through.        */
                              /* We could use _etext instead, but that  */
                              /* would include .rodata, which may       */
                              /* contain large read-only data tables    */
                              /* that we'd rather not scan.             */
#           endif
            extern int _end[];
#           define DATAEND ((ptr_t)(_end))
#       else
            extern int etext[];
#           define DATASTART ((ptr_t)((((word)(etext)) + 0xfff) & ~0xfff))
#       endif
#   endif
#   ifdef MSWINCE
#     define OS_TYPE "MSWINCE"
#     define DATAEND /* not needed */
#   endif
#   ifdef FREEBSD
      /* FreeBSD/arm */
#     define OS_TYPE "FREEBSD"
#     ifndef GC_FREEBSD_THREADS
#       define MPROTECT_VDB
#     endif
#     define SIG_SUSPEND SIGUSR1
#     define SIG_THR_RESTART SIGUSR2
#     define FREEBSD_STACKBOTTOM
#     ifdef __ELF__
#       define DYNAMIC_LOADING
#     endif
      extern char etext[];
#     define DATASTART GC_FreeBSDGetDataStart(0x1000, (ptr_t)etext)
#     define DATASTART_USES_BSDGETDATASTART
#   endif
#   ifdef DARWIN
      /* iOS */
#     define OS_TYPE "DARWIN"
#     define DARWIN_DONT_PARSE_STACK 1
#     define DYNAMIC_LOADING
#     define DATASTART ((ptr_t)get_etext())
#     define DATAEND   ((ptr_t)get_end())
#     define STACKBOTTOM ((ptr_t)0x30000000)
#     define USE_MMAP_ANON
#     define MPROTECT_VDB
      EXTERN_C_END
#     include <unistd.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)getpagesize()
      /* FIXME: There seems to be some issues with trylock hanging on   */
      /* darwin. This should be looked into some more.                  */
#     define NO_PTHREAD_TRYLOCK
#     if TARGET_OS_IPHONE && !defined(NO_DYLD_BIND_FULLY_IMAGE)
#       define NO_DYLD_BIND_FULLY_IMAGE
#     endif
#   endif
#   ifdef OPENBSD
#     define OS_TYPE "OPENBSD"
#     ifndef GC_OPENBSD_THREADS
        EXTERN_C_END
#       include <sys/param.h>
#       include <uvm/uvm_extern.h>
        EXTERN_C_BEGIN
#       ifdef USRSTACK
#         define STACKBOTTOM ((ptr_t)USRSTACK)
#       else
#         define HEURISTIC2
#       endif
#     endif
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
      extern int _end[];
#     define DATAEND ((ptr_t)(&_end))
#     define DYNAMIC_LOADING
#   endif
#   ifdef SN_TARGET_PSP2
#     define NO_HANDLE_FORK 1
#     define DATASTART (ptr_t)ALIGNMENT
#     define DATAEND (ptr_t)ALIGNMENT
      void *psp2_get_stack_bottom(void);
#     define STACKBOTTOM ((ptr_t)psp2_get_stack_bottom())
#   endif
#   ifdef NN_PLATFORM_CTR
      extern unsigned char Image$$ZI$$ZI$$Base[];
#     define DATASTART (ptr_t)(Image$$ZI$$ZI$$Base)
      extern unsigned char Image$$ZI$$ZI$$Limit[];
#     define DATAEND (ptr_t)(Image$$ZI$$ZI$$Limit)
      void *n3ds_get_stack_bottom(void);
#     define STACKBOTTOM ((ptr_t)n3ds_get_stack_bottom())
#   endif
#   ifdef NOSYS
      /* __data_start is usually defined in the target linker script.  */
      extern int __data_start[];
#     define DATASTART ((ptr_t)(__data_start))
      /* __stack_base__ is set in newlib/libc/sys/arm/crt0.S  */
      extern void *__stack_base__;
#     define STACKBOTTOM ((ptr_t)(__stack_base__))
#   endif
#endif









# if defined(SH) && !defined(SH4)
#   define MACH_TYPE "SH"
#   define ALIGNMENT 4
#   ifdef MSWINCE
#     define OS_TYPE "MSWINCE"
#     define DATAEND /* not needed */
#   endif
#   ifdef LINUX
#     define OS_TYPE "LINUX"
#     define LINUX_STACKBOTTOM
#     define DYNAMIC_LOADING
#     define SEARCH_FOR_DATA_START
      extern int _end[];
#     define DATAEND ((ptr_t)(_end))
#   endif
#   ifdef NETBSD
#      define OS_TYPE "NETBSD"
#      define HEURISTIC2
       extern ptr_t GC_data_start;
#      define DATASTART GC_data_start
#      define DYNAMIC_LOADING
#   endif
#   ifdef OPENBSD
#     define OS_TYPE "OPENBSD"
#     ifndef GC_OPENBSD_THREADS
        EXTERN_C_END
#       include <sys/param.h>
#       include <uvm/uvm_extern.h>
        EXTERN_C_BEGIN
#       ifdef USRSTACK
#         define STACKBOTTOM ((ptr_t)USRSTACK)
#       else
#         define HEURISTIC2
#       endif
#     endif
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
      extern int _end[];
#     define DATAEND ((ptr_t)(&_end))
#     define DYNAMIC_LOADING
#   endif
# endif

# ifdef SH4
#   define MACH_TYPE "SH4"
#   define ALIGNMENT 4
#   ifdef MSWINCE
#     define OS_TYPE "MSWINCE"
#     define DATAEND /* not needed */
#   endif
# endif

# ifdef AVR32
#   define MACH_TYPE "AVR32"
#   define CPP_WORDSZ 32
#   define ALIGNMENT 4
#   ifdef LINUX
#     define OS_TYPE "LINUX"
#     define DYNAMIC_LOADING
#     define LINUX_STACKBOTTOM
#     define SEARCH_FOR_DATA_START
      extern int _end[];
#     define DATAEND ((ptr_t)(_end))
#   endif
# endif

# ifdef M32R
#   define CPP_WORDSZ 32
#   define MACH_TYPE "M32R"
#   define ALIGNMENT 4
#   ifdef LINUX
#     define OS_TYPE "LINUX"
#     define LINUX_STACKBOTTOM
#     define DYNAMIC_LOADING
#     define SEARCH_FOR_DATA_START
      extern int _end[];
#     define DATAEND ((ptr_t)(_end))
#   endif
# endif


# ifdef X86_64
#   define MACH_TYPE "X86_64"
#   ifdef __ILP32__
#     define ALIGNMENT 4
#     define CPP_WORDSZ 32
#   else
#     define ALIGNMENT 8
#     define CPP_WORDSZ 64
#   endif
#   ifndef HBLKSIZE
#     define HBLKSIZE 4096
#   endif
#   ifndef CACHE_LINE_SIZE
#     define CACHE_LINE_SIZE 64
#   endif
#   ifdef SN_TARGET_ORBIS
#     define DATASTART (ptr_t)ALIGNMENT
#     define DATAEND (ptr_t)ALIGNMENT
      void *ps4_get_stack_bottom(void);
#     define STACKBOTTOM ((ptr_t)ps4_get_stack_bottom())
#   endif
#   ifdef OPENBSD
#       define OS_TYPE "OPENBSD"
#       define ELF_CLASS ELFCLASS64
#       ifndef GC_OPENBSD_THREADS
          EXTERN_C_END
#         include <sys/param.h>
#         include <uvm/uvm_extern.h>
          EXTERN_C_BEGIN
#         ifdef USRSTACK
#           define STACKBOTTOM ((ptr_t)USRSTACK)
#         else
#           define HEURISTIC2
#         endif
#       endif
        extern int __data_start[];
        extern int _end[];
#       define DATASTART ((ptr_t)__data_start)
#       define DATAEND ((ptr_t)(&_end))
#       define DYNAMIC_LOADING
#   endif
#   ifdef LINUX
#       define OS_TYPE "LINUX"
#       define LINUX_STACKBOTTOM
#       if !defined(GC_LINUX_THREADS) || !defined(REDIRECT_MALLOC)
#           define MPROTECT_VDB
#       else
            /* We seem to get random errors in incremental mode,        */
            /* possibly because Linux threads is itself a malloc client */
            /* and can't deal with the signals.                         */
#       endif
#       ifdef __ELF__
#           define DYNAMIC_LOADING
            EXTERN_C_END
#           include <features.h>
            EXTERN_C_BEGIN
#           define SEARCH_FOR_DATA_START
            extern int _end[];
#           define DATAEND ((ptr_t)(_end))
#       else
             extern int etext[];
#            define DATASTART ((ptr_t)((((word)(etext)) + 0xfff) & ~0xfff))
#       endif
#       if defined(__GLIBC__) && !defined(__UCLIBC__)
          /* A workaround for GCF (Google Cloud Function) which does    */
          /* not support mmap() for "/dev/zero".  Should not cause any  */
          /* harm to other targets.                                     */
#         define USE_MMAP_ANON
          /* At present, there's a bug in GLibc getcontext() on         */
          /* Linux/x64 (it clears FPU exception mask).  We define this  */
          /* macro to workaround it.                                    */
          /* TODO: This seems to be fixed in GLibc v2.14.               */
#         define GETCONTEXT_FPU_EXCMASK_BUG
          /* Workaround lock elision implementation for some glibc.     */
#         define GLIBC_2_19_TSX_BUG
          EXTERN_C_END
#         include <gnu/libc-version.h> /* for gnu_get_libc_version() */
          EXTERN_C_BEGIN
#       endif
#   endif
#   ifdef DARWIN
#     include <sys/vm.h>
#     define OS_TYPE "DARWIN"
#     define DARWIN_DONT_PARSE_STACK 1
#     define DYNAMIC_LOADING
      /* XXX: see get_end(3), get_etext() and get_end() should not be used. */
      /* These aren't used when dyld support is enabled (it is by default)  */
#     define DATASTART ((ptr_t)get_etext())
#     define DATAEND   ((ptr_t)get_end())
#     define STACKBOTTOM ((ptr_t)0x7fff5fc00000)
#     define USE_MMAP_ANON
#     define MPROTECT_VDB
      EXTERN_C_END
#     include <unistd.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)getpagesize()
      /* There seems to be some issues with trylock hanging on darwin.  */
      /* This should be looked into some more.                          */
#     define NO_PTHREAD_TRYLOCK
#     if TARGET_OS_IPHONE && !defined(NO_DYLD_BIND_FULLY_IMAGE)
        /* iPhone/iPad simulator */
#       define NO_DYLD_BIND_FULLY_IMAGE
#     endif
#   endif
#   ifdef FREEBSD
#       define OS_TYPE "FREEBSD"
#       ifndef GC_FREEBSD_THREADS
#           define MPROTECT_VDB
#       endif
#       ifdef __GLIBC__
#           define SIG_SUSPEND          (32+6)
#           define SIG_THR_RESTART      (32+5)
            extern int _end[];
#           define DATAEND ((ptr_t)(_end))
#       else
#           define SIG_SUSPEND SIGUSR1
#           define SIG_THR_RESTART SIGUSR2
                /* SIGTSTP and SIGCONT could be used alternatively.     */
#       endif
#       define FREEBSD_STACKBOTTOM
#       ifdef __ELF__
#           define DYNAMIC_LOADING
#       endif
        extern char etext[];
#       define DATASTART GC_FreeBSDGetDataStart(0x1000, (ptr_t)etext)
#       define DATASTART_USES_BSDGETDATASTART
#   endif
#   ifdef NETBSD
#       define OS_TYPE "NETBSD"
#       define HEURISTIC2
#       ifdef __ELF__
            extern ptr_t GC_data_start;
#           define DATASTART GC_data_start
#           define DYNAMIC_LOADING
#       else
#           define SEARCH_FOR_DATA_START
#       endif
#   endif
#   ifdef HAIKU
#     define OS_TYPE "HAIKU"
      EXTERN_C_END
#     include <OS.h>
      EXTERN_C_BEGIN
#     define GETPAGESIZE() (unsigned)B_PAGE_SIZE
#     define HEURISTIC2
#     define SEARCH_FOR_DATA_START
#     define DYNAMIC_LOADING
#     define MPROTECT_VDB
#   endif
#   ifdef SOLARIS
#       define OS_TYPE "SOLARIS"
#       define ELF_CLASS ELFCLASS64
        extern int _etext[], _end[];
        ptr_t GC_SysVGetDataStart(size_t, ptr_t);
#       define DATASTART GC_SysVGetDataStart(0x1000, (ptr_t)_etext)
#       define DATASTART_IS_FUNC
#       define DATAEND ((ptr_t)(_end))
        /* # define STACKBOTTOM ((ptr_t)(_start)) worked through 2.7,   */
        /* but reportedly breaks under 2.8.  It appears that the stack  */
        /* base is a property of the executable, so this should not     */
        /* break old executables.                                       */
        /* HEURISTIC2 probably works, but this appears to be preferable.*/
        /* Apparently USRSTACK is defined to be USERLIMIT, but in some  */
        /* installations that's undefined.  We work around this with a  */
        /* gross hack:                                                  */
        EXTERN_C_END
#       include <sys/vmparam.h>
        EXTERN_C_BEGIN
#       ifdef USERLIMIT
          /* This should work everywhere, but doesn't.  */
#         define STACKBOTTOM ((ptr_t)USRSTACK)
#       else
#         define HEURISTIC2
#       endif
/* At least in Solaris 2.5, PROC_VDB gives wrong values for dirty bits. */
/* It appears to be fixed in 2.8 and 2.9.                               */
#       ifdef SOLARIS25_PROC_VDB_BUG_FIXED
#         define PROC_VDB
#       endif
#       ifndef GC_THREADS
#         define MPROTECT_VDB
#       endif
#       define DYNAMIC_LOADING
#       if !defined(USE_MMAP) && defined(REDIRECT_MALLOC)
#         define USE_MMAP 1
            /* Otherwise we now use calloc.  Mmap may result in the     */
            /* heap interleaved with thread stacks, which can result in */
            /* excessive blacklisting.  Sbrk is unusable since it       */
            /* doesn't interact correctly with the system malloc.       */
#       endif
#       ifdef USE_MMAP
#         define HEAP_START (ptr_t)0x40000000
#       else
#         define HEAP_START DATAEND
#       endif
#   endif
#   ifdef CYGWIN32
#       define OS_TYPE "CYGWIN32"
#       ifdef USE_WINALLOC
#         define GWW_VDB
#       else
#         define MPROTECT_VDB
#         ifdef USE_MMAP
#           define USE_MMAP_ANON
#         endif
#       endif
#   endif
#   ifdef MSWIN_XBOX1
#     define NO_GETENV
#     define DATASTART (ptr_t)ALIGNMENT
#     define DATAEND (ptr_t)ALIGNMENT
      LONG64 durango_get_stack_bottom(void);
#     define STACKBOTTOM ((ptr_t)durango_get_stack_bottom())
#     define GETPAGESIZE() 4096
#     ifndef USE_MMAP
#       define USE_MMAP 1
#     endif
      /* The following is from sys/mman.h:  */
#     define PROT_NONE  0
#     define PROT_READ  1
#     define PROT_WRITE 2
#     define PROT_EXEC  4
#     define MAP_PRIVATE 2
#     define MAP_FIXED  0x10
#     define MAP_FAILED ((void *)-1)
#   endif
#   ifdef MSWIN32
#       define OS_TYPE "MSWIN32"
                /* STACKBOTTOM and DATASTART are handled specially in   */
                /* os_dep.c.                                            */
#       if !defined(__GNUC__) || defined(__INTEL_COMPILER) \
           || GC_GNUC_PREREQ(4, 7)
          /* Older GCC has not supported SetUnhandledExceptionFilter    */
          /* properly on x64 (e.g. SEH unwinding information missed).   */
#         define MPROTECT_VDB
#       endif
#       define GWW_VDB
#       ifndef DATAEND
#         define DATAEND    /* not needed */
#       endif
#   endif
# endif /* X86_64 */

# ifdef HEXAGON
#   define CPP_WORDSZ 32
#   define MACH_TYPE "HEXAGON"
#   define ALIGNMENT 4
#   ifdef LINUX
#       define OS_TYPE "LINUX"
#       define LINUX_STACKBOTTOM
#       define MPROTECT_VDB
#       ifdef __ELF__
#           define DYNAMIC_LOADING
            EXTERN_C_END
#           include <features.h>
            EXTERN_C_BEGIN
#           if defined(__GLIBC__) && __GLIBC__ >= 2
#               define SEARCH_FOR_DATA_START
#           else
#               error Unknown Hexagon libc configuration
#           endif
            extern int _end[];
#           define DATAEND ((ptr_t)(_end))
#       elif !defined(CPPCHECK)
#           error Bad Hexagon Linux configuration
#       endif
#   else
#       error Unknown Hexagon OS configuration
#   endif
# endif

# ifdef TILEPRO
#   define CPP_WORDSZ 32
#   define MACH_TYPE "TILEPro"
#   define ALIGNMENT 4
#   define PREFETCH(x) __insn_prefetch(x)
#   define CACHE_LINE_SIZE 64
#   ifdef LINUX
#     define OS_TYPE "LINUX"
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
#     define LINUX_STACKBOTTOM
#     define DYNAMIC_LOADING
#   endif
# endif

# ifdef TILEGX
#   define CPP_WORDSZ (__SIZEOF_POINTER__ * 8)
#   define MACH_TYPE "TILE-Gx"
#   define ALIGNMENT __SIZEOF_POINTER__
#   if CPP_WORDSZ < 64
#     define CLEAR_DOUBLE(x) (*(long long *)(x) = 0)
#   endif
#   define PREFETCH(x) __insn_prefetch_l1(x)
#   define CACHE_LINE_SIZE 64
#   ifdef LINUX
#     define OS_TYPE "LINUX"
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
#     define LINUX_STACKBOTTOM
#     define DYNAMIC_LOADING
#   endif
# endif

# ifdef RISCV
#   define MACH_TYPE "RISC-V"
#   define CPP_WORDSZ __riscv_xlen /* 32 or 64 */
#   define ALIGNMENT (CPP_WORDSZ/8)
#   ifdef LINUX
#     define OS_TYPE "LINUX"
      extern int __data_start[];
#     define DATASTART ((ptr_t)__data_start)
#     define LINUX_STACKBOTTOM
#     define DYNAMIC_LOADING
#   endif
# endif /* RISCV */

#if defined(__GLIBC__) && !defined(DONT_USE_LIBC_PRIVATES)
  /* Use glibc's stack-end marker. */
# define USE_LIBC_PRIVATES
#endif

#if defined(LINUX_STACKBOTTOM) && defined(NO_PROC_STAT) \
    && !defined(USE_LIBC_PRIVATES)
    /* This combination will fail, since we have no way to get  */
    /* the stack base.  Use HEURISTIC2 instead.                 */
#   undef LINUX_STACKBOTTOM
#   define HEURISTIC2
    /* This may still fail on some architectures like IA64.     */
    /* We tried ...                                             */
#endif

#if defined(USE_MMAP_ANON) && !defined(USE_MMAP)
#   define USE_MMAP 1
#elif defined(LINUX) && defined(USE_MMAP)
    /* The kernel may do a somewhat better job merging mappings etc.    */
    /* with anonymous mappings.                                         */
#   define USE_MMAP_ANON
#endif

#if defined(GC_LINUX_THREADS) && defined(REDIRECT_MALLOC) \
    && !defined(USE_PROC_FOR_LIBRARIES)
    /* Nptl allocates thread stacks with mmap, which is fine.  But it   */
    /* keeps a cache of thread stacks.  Thread stacks contain the       */
    /* thread control blocks.  These in turn contain a pointer to       */
    /* (sizeof (void *) from the beginning of) the dtv for thread-local */
    /* storage, which is calloc allocated.  If we don't scan the cached */
    /* thread stacks, we appear to lose the dtv.  This tends to         */
    /* result in something that looks like a bogus dtv count, which     */
    /* tends to result in a memset call on a block that is way too      */
    /* large.  Sometimes we're lucky and the process just dies ...      */
    /* There seems to be a similar issue with some other memory         */
    /* allocated by the dynamic loader.                                 */
    /* This should be avoidable by either:                              */
    /* - Defining USE_PROC_FOR_LIBRARIES here.                          */
    /*   That performs very poorly, precisely because we end up         */
    /*   scanning cached stacks.                                        */
    /* - Have calloc look at its callers.                               */
    /*   In spite of the fact that it is gross and disgusting.          */
    /* In fact neither seems to suffice, probably in part because       */
    /* even with USE_PROC_FOR_LIBRARIES, we don't scan parts of stack   */
    /* segments that appear to be out of bounds.  Thus we actually      */
    /* do both, which seems to yield the best results.                  */
#   define USE_PROC_FOR_LIBRARIES
#endif

#ifndef STACK_GROWS_UP
# define STACK_GROWS_DOWN
#endif

#ifndef CPP_WORDSZ
# define CPP_WORDSZ 32
#endif

#ifndef OS_TYPE
# define OS_TYPE ""
#endif

#ifndef DATAEND
# if !defined(CPPCHECK)
    extern int end[];
# endif
# define DATAEND ((ptr_t)(end))
#endif

/* Workaround for Android NDK clang 3.5+ (as of NDK r10e) which does    */
/* not provide correct _end symbol.  Unfortunately, alternate __end__   */
/* symbol is provided only by NDK "bfd" linker.                         */
#if defined(HOST_ANDROID) && defined(__clang__) \
    && !defined(BROKEN_UUENDUU_SYM)
# undef DATAEND
# pragma weak __end__
  extern int __end__[];
# define DATAEND (__end__ != 0 ? (ptr_t)__end__ : (ptr_t)_end)
#endif

#if (defined(SVR4) || defined(HOST_ANDROID) || defined(HOST_TIZEN)) \
    && !defined(GETPAGESIZE)
  EXTERN_C_END
# include <unistd.h>
  EXTERN_C_BEGIN
# define GETPAGESIZE() (unsigned)sysconf(_SC_PAGESIZE)
#endif

#ifndef GETPAGESIZE
# if defined(SOLARIS) || defined(IRIX5) || defined(LINUX) \
     || defined(NETBSD) || defined(FREEBSD) || defined(HPUX)
    EXTERN_C_END
#   include <unistd.h>
    EXTERN_C_BEGIN
# endif
# define GETPAGESIZE() (unsigned)getpagesize()
#endif

#if defined(HOST_ANDROID) && !(__ANDROID_API__ >= 23) \
    && ((defined(MIPS) && (CPP_WORDSZ == 32)) \
        || defined(ARM32) || defined(I386) /* but not x32 */)
  /* tkill() exists only on arm32/mips(32)/x86. */
  /* NDK r11+ deprecates tkill() but keeps it for Mono clients. */
# define USE_TKILL_ON_ANDROID
#endif

#if defined(SOLARIS) || defined(DRSNX) || defined(UTS4)
        /* OS has SVR4 generic features.        */
        /* Probably others also qualify.        */
# define SVR4
#endif

#if defined(SOLARIS) || defined(DRSNX)
        /* OS has SOLARIS style semi-undocumented interface     */
        /* to dynamic loader.                                   */
# define SOLARISDL
        /* OS has SOLARIS style signal handlers.        */
# define SUNOS5SIGS
#endif

#if defined(HPUX)
# define SUNOS5SIGS
#endif

#if defined(FREEBSD) && (defined(__DragonFly__) || __FreeBSD__ >= 4 \
                         || (__FreeBSD_kernel__ >= 4))
# define SUNOS5SIGS
#endif

#if !defined(GC_EXPLICIT_SIGNALS_UNBLOCK) && defined(SUNOS5SIGS) \
    && !defined(GC_NO_PTHREAD_SIGMASK)
# define GC_EXPLICIT_SIGNALS_UNBLOCK
#endif

#if !defined(NO_SIGNALS_UNBLOCK_IN_MAIN) && defined(GC_NO_PTHREAD_SIGMASK)
# define NO_SIGNALS_UNBLOCK_IN_MAIN
#endif

#if !defined(NO_MARKER_SPECIAL_SIGMASK) \
    && (defined(NACL) || defined(GC_WIN32_PTHREADS))
  /* Either there is no pthread_sigmask(), or GC marker thread cannot   */
  /* steal and drop user signal calls.                                  */
# define NO_MARKER_SPECIAL_SIGMASK
#endif

#ifdef GC_NETBSD_THREADS
# define SIGRTMIN 33
# define SIGRTMAX 63
  /* It seems to be necessary to wait until threads have restarted.     */
  /* But it is unclear why that is the case.                            */
# define GC_NETBSD_THREADS_WORKAROUND
#endif

#ifdef GC_OPENBSD_THREADS
  EXTERN_C_END
# include <sys/param.h>
  EXTERN_C_BEGIN
  /* Prior to 5.2 release, OpenBSD had user threads and required        */
  /* special handling.                                                  */
# if OpenBSD < 201211
#   define GC_OPENBSD_UTHREADS 1
# endif
#endif /* GC_OPENBSD_THREADS */

#if defined(SVR4) || defined(LINUX) || defined(IRIX5) || defined(HPUX) \
    || defined(OPENBSD) || defined(NETBSD) || defined(FREEBSD) \
    || defined(DGUX) || defined(BSD) || defined(HAIKU) || defined(HURD) \
    || defined(AIX) || defined(DARWIN) || defined(OSF1)
# define UNIX_LIKE      /* Basic Unix-like system calls work.   */
#endif

#if defined(CPPCHECK)
# undef CPP_WORDSZ
# define CPP_WORDSZ (__SIZEOF_POINTER__ * 8)
#elif CPP_WORDSZ != 32 && CPP_WORDSZ != 64
#   error Bad word size
#endif


#ifdef PCR
# undef DYNAMIC_LOADING
# undef STACKBOTTOM
# undef HEURISTIC1
# undef HEURISTIC2
# undef PROC_VDB
# undef MPROTECT_VDB
# define PCR_VDB
#endif

#if !defined(STACKBOTTOM) && (defined(ECOS) || defined(NOSYS)) \
    && !defined(CPPCHECK)
# error Undefined STACKBOTTOM
#endif

#ifdef IGNORE_DYNAMIC_LOADING
# undef DYNAMIC_LOADING
#endif

#if defined(SMALL_CONFIG) && !defined(GC_DISABLE_INCREMENTAL)
  /* Presumably not worth the space it takes.   */
# define GC_DISABLE_INCREMENTAL
#endif

#if (defined(MSWIN32) || defined(MSWINCE)) && !defined(USE_WINALLOC)
  /* USE_WINALLOC is only an option for Cygwin. */
# define USE_WINALLOC 1
#endif

#ifdef USE_WINALLOC
# undef USE_MMAP
#endif

#if defined(DARWIN) || defined(FREEBSD) || defined(HAIKU) \
    || defined(IRIX5) || defined(LINUX) || defined(NETBSD) \
    || defined(OPENBSD) || defined(SOLARIS) \
    || ((defined(CYGWIN32) || defined(USE_MMAP) || defined(USE_MUNMAP)) \
        && !defined(USE_WINALLOC))
  /* Try both sbrk and mmap, in that order.     */
# define MMAP_SUPPORTED
#endif

/* Xbox One (DURANGO) may not need to be this aggressive, but the       */
/* default is likely too lax under heavy allocation pressure.           */
/* The platform does not have a virtual paging system, so it does not   */
/* have a large virtual address space that a standard x64 platform has. */
#if defined(USE_MUNMAP) && !defined(MUNMAP_THRESHOLD) \
    && (defined(SN_TARGET_ORBIS) || defined(SN_TARGET_PS3) \
        || defined(SN_TARGET_PSP2) || defined(MSWIN_XBOX1))
# define MUNMAP_THRESHOLD 2
#endif

#if defined(GC_DISABLE_INCREMENTAL) || defined(DEFAULT_VDB)
# undef GWW_VDB
# undef MPROTECT_VDB
# undef PCR_VDB
# undef PROC_VDB
#endif

#ifdef GC_DISABLE_INCREMENTAL
# undef CHECKSUMS
#endif

#ifdef USE_GLOBAL_ALLOC
  /* Cannot pass MEM_WRITE_WATCH to GlobalAlloc().      */
# undef GWW_VDB
#endif

#if defined(BASE_ATOMIC_OPS_EMULATED)
  /* GC_write_fault_handler() cannot use lock-based atomic primitives   */
  /* as this could lead to a deadlock.                                  */
# undef MPROTECT_VDB
#endif

#if defined(MPROTECT_VDB) && defined(GC_PREFER_MPROTECT_VDB)
  /* Choose MPROTECT_VDB manually (if multiple strategies available).   */
# undef PCR_VDB
# undef PROC_VDB
  /* #undef GWW_VDB - handled in os_dep.c       */
#endif

#ifdef PROC_VDB
  /* Multi-VDB mode is not implemented. */
# undef MPROTECT_VDB
#endif

#if defined(MPROTECT_VDB) && !defined(MSWIN32) && !defined(MSWINCE)
# include <signal.h> /* for SA_SIGINFO, SIGBUS */
#endif

#if defined(SIGBUS) && !defined(HAVE_SIGBUS) && !defined(CPPCHECK)
# define HAVE_SIGBUS
#endif

#ifndef SA_SIGINFO
# define NO_SA_SIGACTION
#endif

#if defined(NO_SA_SIGACTION) && defined(MPROTECT_VDB) && !defined(DARWIN) \
    && !defined(MSWIN32) && !defined(MSWINCE)
# undef MPROTECT_VDB
#endif

#if !defined(PCR_VDB) && !defined(PROC_VDB) && !defined(MPROTECT_VDB) \
    && !defined(GWW_VDB) && !defined(DEFAULT_VDB) \
    && !defined(GC_DISABLE_INCREMENTAL)
# define DEFAULT_VDB
#endif

#if ((defined(UNIX_LIKE) && (defined(DARWIN) || defined(HAIKU) \
                             || defined(HURD) || defined(OPENBSD) \
                             || defined(ARM32) \
                             || defined(AVR32) || defined(MIPS) \
                             || defined(NIOS2) || defined(OR1K))) \
     || (defined(LINUX) && !defined(__gnu_linux__)) \
     || (defined(RTEMS) && defined(I386)) || defined(HOST_ANDROID)) \
    && !defined(NO_GETCONTEXT)
# define NO_GETCONTEXT 1
#endif


#ifndef CACHE_LINE_SIZE
# define CACHE_LINE_SIZE 32     /* Wild guess   */
#endif

#ifndef STATIC
# ifndef NO_DEBUGGING
#   define STATIC /* ignore to aid profiling and possibly debugging     */
# else
#   define STATIC static
# endif
#endif

#if defined(LINUX) && (defined(USE_PROC_FOR_LIBRARIES) || defined(IA64) \
                       || !defined(SMALL_CONFIG))
# define NEED_PROC_MAPS
#endif

#if defined(LINUX) || defined(HURD) || defined(__GLIBC__)
# define REGISTER_LIBRARIES_EARLY
  /* We sometimes use dl_iterate_phdr, which may acquire an internal    */
  /* lock.  This isn't safe after the world has stopped.  So we must    */
  /* call GC_register_dynamic_libraries before stopping the world.      */
  /* For performance reasons, this may be beneficial on other           */
  /* platforms as well, though it should be avoided in win32.           */
#endif /* LINUX */

#if defined(SEARCH_FOR_DATA_START)
  extern ptr_t GC_data_start;
# define DATASTART GC_data_start
#endif

#ifndef HEAP_START
# define HEAP_START ((ptr_t)0)
#endif

#ifndef CLEAR_DOUBLE
# define CLEAR_DOUBLE(x) (((word*)(x))[0] = 0, ((word*)(x))[1] = 0)
#endif

#if defined(GC_LINUX_THREADS) && defined(REDIRECT_MALLOC) \
    && !defined(INCLUDE_LINUX_THREAD_DESCR)
  /* Will not work, since libc and the dynamic loader use thread        */
  /* locals, sometimes as the only reference.                           */
# define INCLUDE_LINUX_THREAD_DESCR
#endif

#if !defined(CPPCHECK)
# if defined(GC_IRIX_THREADS) && !defined(IRIX5)
#   error Inconsistent configuration
# endif
# if defined(GC_LINUX_THREADS) && !defined(LINUX) && !defined(NACL)
#   error Inconsistent configuration
# endif
# if defined(GC_NETBSD_THREADS) && !defined(NETBSD)
#   error Inconsistent configuration
# endif
# if defined(GC_FREEBSD_THREADS) && !defined(FREEBSD)
#   error Inconsistent configuration
# endif
# if defined(GC_SOLARIS_THREADS) && !defined(SOLARIS)
#   error Inconsistent configuration
# endif
# if defined(GC_HPUX_THREADS) && !defined(HPUX)
#   error Inconsistent configuration
# endif
# if defined(GC_AIX_THREADS) && !defined(_AIX)
#   error Inconsistent configuration
# endif
# if defined(GC_WIN32_THREADS) && !defined(CYGWIN32) && !defined(MSWIN32) \
     && !defined(MSWINCE) && !defined(MSWIN_XBOX1)
#   error Inconsistent configuration
# endif
# if defined(GC_WIN32_PTHREADS) && defined(CYGWIN32)
#   error Inconsistent configuration
# endif
#endif /* !CPPCHECK */

#if defined(PCR) || defined(GC_WIN32_THREADS) || defined(GC_PTHREADS) \
    || ((defined(NN_PLATFORM_CTR) || defined(NINTENDO_SWITCH) \
         || defined(SN_TARGET_ORBIS) || defined(SN_TARGET_PS3) \
         || defined(SN_TARGET_PSP2)) && defined(GC_THREADS))
# define THREADS
#endif

#if defined(PARALLEL_MARK) && !defined(THREADS) && !defined(CPPCHECK)
# error Invalid config: PARALLEL_MARK requires GC_THREADS
#endif

#if defined(GWW_VDB) && !defined(USE_WINALLOC) && !defined(CPPCHECK)
# error Invalid config: GWW_VDB requires USE_WINALLOC
#endif

#if (((defined(MSWIN32) || defined(MSWINCE)) && !defined(__GNUC__)) \
        || (defined(MSWIN32) && defined(I386)) /* for Win98 */ \
        || (defined(USE_PROC_FOR_LIBRARIES) && defined(THREADS))) \
    && !defined(NO_CRT) && !defined(NO_WRAP_MARK_SOME)
  /* Under rare conditions, we may end up marking from nonexistent      */
  /* memory.  Hence we need to be prepared to recover by running        */
  /* GC_mark_some with a suitable handler in place.                     */
  /* TODO: Probably replace __GNUC__ above with ndef GC_PTHREADS.       */
  /* FIXME: Should we really need it for WinCE?  If yes then            */
  /* WRAP_MARK_SOME should be also defined for CeGCC which requires     */
  /* CPU/OS-specific code in mark_ex_handler and GC_mark_some (for      */
  /* manual stack unwinding and exception handler installation).        */
# define WRAP_MARK_SOME
#endif

#if defined(PARALLEL_MARK) && !defined(DEFAULT_STACK_MAYBE_SMALL) \
    && (defined(HPUX) || defined(GC_DGUX386_THREADS) \
        || defined(NO_GETCONTEXT) /* e.g. musl */)
    /* TODO: Test default stack size in configure. */
# define DEFAULT_STACK_MAYBE_SMALL
#endif

#ifdef PARALLEL_MARK
  /* The minimum stack size for a marker thread. */
# define MIN_STACK_SIZE (8 * HBLKSIZE * sizeof(word))
#endif

#if defined(HOST_ANDROID) && !defined(THREADS) \
    && !defined(USE_GET_STACKBASE_FOR_MAIN)
  /* Always use pthread_attr_getstack on Android ("-lpthread" option is  */
  /* not needed to be specified manually) since GC_linux_main_stack_base */
  /* causes app crash if invoked inside Dalvik VM.                       */
# define USE_GET_STACKBASE_FOR_MAIN
#endif

/* Outline pthread primitives to use in GC_get_[main_]stack_base.       */
#if ((defined(FREEBSD) && defined(__GLIBC__)) /* kFreeBSD */ \
     || defined(LINUX) || defined(NETBSD) || defined(HOST_ANDROID)) \
    && !defined(NO_PTHREAD_GETATTR_NP)
# define HAVE_PTHREAD_GETATTR_NP 1
#elif defined(FREEBSD) && !defined(__GLIBC__) \
      && !defined(NO_PTHREAD_ATTR_GET_NP)
# define HAVE_PTHREAD_NP_H 1 /* requires include pthread_np.h */
# define HAVE_PTHREAD_ATTR_GET_NP 1
#endif

#if defined(UNIX_LIKE) && defined(THREADS) && !defined(NO_CANCEL_SAFE) \
    && !defined(HOST_ANDROID)
  /* Make the code cancellation-safe.  This basically means that we     */
  /* ensure that cancellation requests are ignored while we are in      */
  /* the collector.  This applies only to Posix deferred cancellation;  */
  /* we don't handle Posix asynchronous cancellation.                   */
  /* Note that this only works if pthread_setcancelstate is             */
  /* async-signal-safe, at least in the absence of asynchronous         */
  /* cancellation.  This appears to be true for the glibc version,      */
  /* though it is not documented.  Without that assumption, there       */
  /* seems to be no way to safely wait in a signal handler, which       */
  /* we need to do for thread suspension.                               */
  /* Also note that little other code appears to be cancellation-safe.  */
  /* Hence it may make sense to turn this off for performance.          */
# define CANCEL_SAFE
#endif

#ifdef CANCEL_SAFE
# define IF_CANCEL(x) x
#else
# define IF_CANCEL(x) /* empty */
#endif

#if !defined(CAN_HANDLE_FORK) && !defined(NO_HANDLE_FORK) \
    && !defined(HAVE_NO_FORK) \
    && ((defined(GC_PTHREADS) && !defined(NACL) \
         && !defined(GC_WIN32_PTHREADS) && !defined(USE_WINALLOC)) \
        || (defined(DARWIN) && defined(MPROTECT_VDB)) || defined(HANDLE_FORK))
  /* Attempts (where supported and requested) to make GC_malloc work in */
  /* a child process fork'ed from a multi-threaded parent.              */
# define CAN_HANDLE_FORK
#endif

#if defined(CAN_HANDLE_FORK) && !defined(CAN_CALL_ATFORK) \
    && !defined(HURD) && !defined(SN_TARGET_ORBIS) && !defined(HOST_TIZEN) \
    && (!defined(HOST_ANDROID) || __ANDROID_API__ >= 21)
  /* Have working pthread_atfork().     */
# define CAN_CALL_ATFORK
#endif

#if !defined(CAN_HANDLE_FORK) && !defined(HAVE_NO_FORK) \
    && (defined(MSWIN32) || defined(MSWINCE) || defined(DOS4GW) \
        || defined(OS2) || defined(SYMBIAN) /* and probably others ... */)
# define HAVE_NO_FORK
#endif

#if !defined(USE_MARK_BITS) && !defined(USE_MARK_BYTES) \
    && defined(PARALLEL_MARK)
   /* Minimize compare-and-swap usage.  */
#  define USE_MARK_BYTES
#endif

#if (defined(MSWINCE) && !defined(__CEGCC__) || defined(MSWINRT_FLAVOR)) \
    && !defined(NO_GETENV)
# define NO_GETENV
#endif

#if (defined(NO_GETENV) || defined(MSWINCE)) && !defined(NO_GETENV_WIN32)
# define NO_GETENV_WIN32
#endif

#ifndef STRTOULL
# if defined(_WIN64) && !defined(__GNUC__)
#   define STRTOULL _strtoui64
# elif defined(_LLP64) || defined(__LLP64__) || defined(_WIN64)
#   define STRTOULL strtoull
# else
    /* strtoul() fits since sizeof(long) >= sizeof(word).       */
#   define STRTOULL strtoul
# endif
#endif /* !STRTOULL */

#ifndef GC_WORD_C
# if defined(_WIN64) && !defined(__GNUC__)
#   define GC_WORD_C(val) val##ui64
# elif defined(_LLP64) || defined(__LLP64__) || defined(_WIN64)
#   define GC_WORD_C(val) val##ULL
# else
#   define GC_WORD_C(val) ((word)val##UL)
# endif
#endif /* !GC_WORD_C */

#if defined(__has_feature)
  /* __has_feature() is supported.      */
# if __has_feature(address_sanitizer) && !defined(ADDRESS_SANITIZER)
#   define ADDRESS_SANITIZER
# endif
# if __has_feature(memory_sanitizer) && !defined(MEMORY_SANITIZER)
#   define MEMORY_SANITIZER
# endif
# if __has_feature(thread_sanitizer) && !defined(THREAD_SANITIZER)
#   define THREAD_SANITIZER
# endif
#else
# ifdef __SANITIZE_ADDRESS__
    /* GCC v4.8+ */
#   define ADDRESS_SANITIZER
# endif
#endif /* !__has_feature */

#if defined(SPARC)
# define ASM_CLEAR_CODE /* Stack clearing is crucial, and we    */
                        /* include assembly code to do it well. */
#endif

/* Can we save call chain in objects for debugging?                     */
/* SET NFRAMES (# of saved frames) and NARGS (#of args for each         */
/* frame) to reasonable values for the platform.                        */
/* Set SAVE_CALL_CHAIN if we can.  SAVE_CALL_COUNT can be specified     */
/* at build time, though we feel free to adjust it slightly.            */
/* Define NEED_CALLINFO if we either save the call stack or             */
/* GC_ADD_CALLER is defined.                                            */
/* GC_CAN_SAVE_CALL_STACKS is set in gc.h.                              */
#if defined(SPARC)
# define CAN_SAVE_CALL_ARGS
#endif
#if (defined(I386) || defined(X86_64)) \
    && (defined(LINUX) || defined(__GLIBC__))
  /* SAVE_CALL_CHAIN is supported if the code is compiled to save       */
  /* frame pointers by default, i.e. no -fomit-frame-pointer flag.      */
# define CAN_SAVE_CALL_ARGS
#endif

#if defined(SAVE_CALL_COUNT) && !defined(GC_ADD_CALLER) \
    && defined(GC_CAN_SAVE_CALL_STACKS)
# define SAVE_CALL_CHAIN
#endif
#ifdef SAVE_CALL_CHAIN
# if defined(SAVE_CALL_NARGS) && defined(CAN_SAVE_CALL_ARGS)
#   define NARGS SAVE_CALL_NARGS
# else
#   define NARGS 0      /* Number of arguments to save for each call.   */
# endif
#endif
#ifdef SAVE_CALL_CHAIN
# if !defined(SAVE_CALL_COUNT) || defined(CPPCHECK)
#   define NFRAMES 6    /* Number of frames to save. Even for   */
                        /* alignment reasons.                   */
# else
#   define NFRAMES ((SAVE_CALL_COUNT + 1) & ~1)
# endif
# define NEED_CALLINFO
#endif /* SAVE_CALL_CHAIN */
#ifdef GC_ADD_CALLER
# define NFRAMES 1
# define NARGS 0
# define NEED_CALLINFO
#endif

#if (defined(FREEBSD) || (defined(DARWIN) && !defined(_POSIX_C_SOURCE)) \
        || (defined(SOLARIS) && (!defined(_XOPEN_SOURCE) \
                                 || defined(__EXTENSIONS__))) \
        || defined(LINUX)) && !defined(HAVE_DLADDR)
# define HAVE_DLADDR 1
#endif

#if defined(MAKE_BACK_GRAPH) && !defined(DBG_HDRS_ALL)
# define DBG_HDRS_ALL 1
#endif

#if defined(POINTER_MASK) && !defined(POINTER_SHIFT)
# define POINTER_SHIFT 0
#endif

#if defined(POINTER_SHIFT) && !defined(POINTER_MASK)
# define POINTER_MASK ((word)(-1))
#endif

#if !defined(FIXUP_POINTER) && defined(POINTER_MASK)
# define FIXUP_POINTER(p) (p = ((p) & POINTER_MASK) << POINTER_SHIFT)
#endif

#if defined(FIXUP_POINTER)
# define NEED_FIXUP_POINTER
#else
# define FIXUP_POINTER(p)
#endif

#if !defined(MARK_BIT_PER_GRANULE) && !defined(MARK_BIT_PER_OBJ)
# define MARK_BIT_PER_GRANULE   /* Usually faster       */
#endif

/* Some static sanity tests.    */
#if !defined(CPPCHECK)
# if defined(MARK_BIT_PER_GRANULE) && defined(MARK_BIT_PER_OBJ)
#   error Define only one of MARK_BIT_PER_GRANULE and MARK_BIT_PER_OBJ
# endif
# if defined(STACK_GROWS_UP) && defined(STACK_GROWS_DOWN)
#   error Only one of STACK_GROWS_UP and STACK_GROWS_DOWN should be defined
# endif
# if !defined(STACK_GROWS_UP) && !defined(STACK_GROWS_DOWN)
#   error One of STACK_GROWS_UP and STACK_GROWS_DOWN should be defined
# endif
# if defined(REDIRECT_MALLOC) && defined(THREADS) && !defined(LINUX) \
     && !defined(REDIRECT_MALLOC_IN_HEADER)
#   error REDIRECT_MALLOC with THREADS works at most on Linux
# endif
#endif /* !CPPCHECK */

#ifdef GC_PRIVATE_H
        /* This relies on some type definitions from gc_priv.h, from    */
        /* where it's normally included.                                */
        /*                                                              */
        /* How to get heap memory from the OS:                          */
        /* Note that sbrk()-like allocation is preferred, since it      */
        /* usually makes it possible to merge consecutively allocated   */
        /* chunks.  It also avoids unintended recursion with            */
        /* REDIRECT_MALLOC macro defined.                               */
        /* GET_MEM() argument should be of size_t type and have         */
        /* no side-effect.  GET_MEM() returns HBLKSIZE-aligned chunk;   */
        /* 0 is taken to mean failure.                                  */
        /* In case of MMAP_SUPPORTED, the argument must also be         */
        /* a multiple of a physical page size.                          */
        /* GET_MEM is currently not assumed to retrieve 0 filled space, */
        /* though we should perhaps take advantage of the case in which */
        /* does.                                                        */
        struct hblk;    /* See gc_priv.h.       */
# if defined(PCR)
    char * real_malloc(size_t bytes);
#   define GET_MEM(bytes) HBLKPTR(real_malloc(SIZET_SAT_ADD(bytes, \
                                                            GC_page_size)) \
                                          + GC_page_size-1)
# elif defined(OS2)
    void * os2_alloc(size_t bytes);
#   define GET_MEM(bytes) HBLKPTR((ptr_t)os2_alloc( \
                                            SIZET_SAT_ADD(bytes, \
                                                          GC_page_size)) \
                                  + GC_page_size-1)
# elif defined(NEXT) || defined(DOS4GW) || defined(NONSTOP) \
        || (defined(AMIGA) && !defined(GC_AMIGA_FASTALLOC)) \
        || (defined(SOLARIS) && !defined(USE_MMAP)) || defined(RTEMS) \
        || defined(__CC_ARM)
#   define GET_MEM(bytes) HBLKPTR((size_t)calloc(1, \
                                            SIZET_SAT_ADD(bytes, \
                                                          GC_page_size)) \
                                  + GC_page_size - 1)
# elif defined(MSWIN_XBOX1)
    ptr_t GC_durango_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk *)GC_durango_get_mem(bytes)
# elif defined(MSWIN32) || defined(CYGWIN32)
    ptr_t GC_win32_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk *)GC_win32_get_mem(bytes)
# elif defined(MACOS)
#   if defined(USE_TEMPORARY_MEMORY)
      Ptr GC_MacTemporaryNewPtr(size_t size, Boolean clearMemory);
#     define GET_MEM(bytes) HBLKPTR(GC_MacTemporaryNewPtr( \
                                        SIZET_SAT_ADD(bytes, \
                                                      GC_page_size), true) \
                        + GC_page_size-1)
#   else
#     define GET_MEM(bytes) HBLKPTR(NewPtrClear(SIZET_SAT_ADD(bytes, \
                                                              GC_page_size)) \
                                    + GC_page_size-1)
#   endif
# elif defined(MSWINCE)
    ptr_t GC_wince_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk *)GC_wince_get_mem(bytes)
# elif defined(AMIGA) && defined(GC_AMIGA_FASTALLOC)
    void *GC_amiga_get_mem(size_t bytes);
#   define GET_MEM(bytes) HBLKPTR((size_t)GC_amiga_get_mem( \
                                            SIZET_SAT_ADD(bytes, \
                                                          GC_page_size)) \
                          + GC_page_size-1)
# elif defined(SN_TARGET_ORBIS)
    void *ps4_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk*)ps4_get_mem(bytes)
# elif defined(SN_TARGET_PS3)
    void *ps3_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk*)ps3_get_mem(bytes)
# elif defined(SN_TARGET_PSP2)
    void *psp2_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk*)psp2_get_mem(bytes)
# elif defined(NINTENDO_SWITCH)
    void *switch_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk*)switch_get_mem(bytes)
# elif defined(HAIKU)
    ptr_t GC_haiku_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk*)GC_haiku_get_mem(bytes)
# else
    ptr_t GC_unix_get_mem(size_t bytes);
#   define GET_MEM(bytes) (struct hblk *)GC_unix_get_mem(bytes)
# endif
#endif /* GC_PRIVATE_H */


#endif /* GCCONFIG_H */
