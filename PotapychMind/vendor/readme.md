# Third-patry libraries

## P7 logger [link][P7_website]

- GNU Lesser General Public License (LGPL)

The source code is available [here][P7_download_source], but it is already contained in the vendor's catalog.

How to compile:
- Install it with CMake
- Open configured project in Visual Studio
- Launch BUILD and INSTALL solutions in Debug mode, it produces file '_p7.lib_'
- Rename it to '_p7d.lib_'
- Launch BUILD and INSTALL solutions in Release mode, it produces file '_p7.lib_'

How to use:
- Copy _lib_ catalog to _vendor/libp7_
- Open your project, go to 'Project'->'Properties'->'Configuration Properties'->'VC++ Directories'
-- In 'Include Directories' add `$(ProjectDir)vendor\libp7\Headers;`
- Open your project, go to 'Project'->'Properties'->'Configuration Properties'->'Linker'
-- In 'General' add `$(ProjectDir)vendor\libp7\lib` to 'Additional Library Directories'
-- In 'Input' add `ws2_32.lib;p7d.lib;` to 'Additional Dependencies'
- Find documentation in sources
- Enjoy!


[//]: #links

[P7_website]: https://baical.net/p7.html
[P7_download_source]: https://baical.net/files/libP7Client_v5.6.zip