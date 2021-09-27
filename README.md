wasl
====

A cross-platform event library where publishers and subscribers can span processes, platforms, devices, and networks.

If you need a C API for interprocess communication without all the IPC portability wrangling, checkout the ubiquitous [libevent](https://libevent.org/). It is probably already running on your machine. If you want a reactor-based implementation in C++, Boost ASIO is good too.

If you need simple IPC with many:many publishers:subscribers and familiar C++ syntax, `wasl` can help. It abstracts away the low-level socket details and does its best to stay out of your way.


installation
------------

```bash

# build it

cmake -G Ninja -B path-to-build

# install it

sudo ninja install


```

datatypes
---------

...todo

integrating with CMake
----------------------

Use CMake's `find_package` after installing the library above.
Edit your CMakeLists.txt:

```cmake
find_package(Wasl 0.2.0 REQUIRED)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Wasl::Wasl
  )
```

docs
----

Build with sphinx:

```bash
make sphinx-doc
```



