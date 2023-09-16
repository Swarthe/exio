# exio

**Extended IO**

Simple C99 library for POSIX systems containing various IO utilities intended to
extend standard C facilities. It includes convenience functions (with optional
colour support) for console messages, signal handling utilities, a function for
reading passwords in a secure manner, and more.

## Installation

```
git clone https://github.com/Swarthe/exio
```

Include the header files `src/*.h` where they are needed, and compile the source
files `src/*.c` together with your project.

You may optionally define the `EXIO_USE_COLOUR` macro before inclusion to enable
support for coloured text output as so:

```c
#define EXIO_USE_COLOUR
#include "exio.h"
```

## License

This library is free software and subject to the MIT license. See `LICENSE.txt`
for more information.
