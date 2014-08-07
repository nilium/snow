Snow Engine
===============================================================================

The Snow Engine is a personal project of mine to build a game/rendering engine.
It is currently licensed under a simplified BSD license, found under COPYING 
and provided below for reference.


Building
-------------------------------------------------------------------------------

The Snow engine is currently only running on Mac OS X. To build it, you'll need
Premake 4.4, a C++11 compiler and standard library as well as the following
other libraries:

- [libsnow-common](https://github.com/nilium/libsnow-common)
- [Enet](http://enet.bespin.org)
- [FLTK](http://www.fltk.org/index.php)
- [GLFW 3](https://github.com/glfw/glfw)
- [PhysicsFS](http://icculus.org/physfs/)
- [ZeroMQ](http://www.zeromq.org)

On Mac OS X, the libc++ standard library is used in place of libstdc++ due to
the latter still being incompatible with C++11 in odd ways. As such, building
this on non-Apple platforms might be tricky. Time should iron out that issue.

Once built, the engine requires a 'base' directory in one of two places,
depending on the platform:

- On OS X when built as an app bundle, the 'base' directory goes under the app
  bundle's 'Resources' directory.
- On other platforms, the 'base' directory is placed alongside the executable.

If using resource archives, the engine expects them to be zip files with a
.snowball extension. Otherwise, it will attempt to load resources from either
the base directory or a platform-specific user-writable directory.


License
-------------------------------------------------------------------------------

        Copyright (c) 2013 - 2014, Noel Raymond Cower <ncower@gmail.com>
        All rights reserved.

        Redistribution and use in source and binary forms, with or without
        modification, are permitted provided that the following conditions are
        met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.

        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
        "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
        LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
        PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
        HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
        SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
        TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
        PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
        LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
        NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
        SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

