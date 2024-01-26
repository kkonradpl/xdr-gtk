XDR-GTK
=======

User interface for XDR-F1HD tuner with XDR-I2C modification.

![Screenshot](/xdr-gtk.png?raw=true)

Copyright (C) 2012-2024  Konrad Kosmatka

https://fmdx.pl/xdr-gtk/

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

# Build
In order to build XDR-GTK you will need:
- CMake
- C compiler
- GTK+ 3 & dependencies
- librdsparser (https://github.com/kkonradpl/librdsparser)

The librdsparser is available as submodule.

Clone the repository with:
```sh
$ git clone --recurse-submodules https://github.com/kkonradpl/xdr-gtk
```

Once you have all the necessary dependencies, you can use scripts available in the `build` directory.

# Installation
After a successful build, just use:
```sh
$ sudo make install
```
in the `build` directory.
