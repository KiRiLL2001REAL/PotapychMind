# Third-patry libraries

## P7 logger [link][P7_website]

- GNU Lesser General Public License (LGPL)

The source code is available [here][P7_download_source], but it is already contained in the vendor's catalog.

How to compile:
- Install it with CMake
- Open configured project in Visual Studio
- Launch BUILD and INSTALL solutions in Debug mode, it produces file **p7.lib**
- Rename it to **p7d.lib**
- Launch BUILD and INSTALL solutions in Release mode, it produces file **p7.lib**

How to link:
- Copy `lib` catalog to `vendor/libp7`
- Open your project, go to 'Project'->'Properties'->'Configuration Properties'->'VC++ Directories'
-- In 'Include Directories' add `$(ProjectDir)vendor\libp7\Headers;`
- Go to 'Project'->'Properties'->'Configuration Properties'->'Linker'
-- In 'General' add `$(ProjectDir)vendor\libp7\lib;` to 'Additional Library Directories'
-- In 'Input' add `ws2_32.lib;p7d.lib;` to 'Additional Dependencies'
- Find documentation in sources
- Enjoy!


[//]: #links
[P7_website]: https://baical.net/p7.html
[P7_download_source]: https://baical.net/files/libP7Client_v5.6.zip

## ImGui

- The MIT License (MIT)

This library is downloaded from [github][ImGui_github] as a submodule.

Make sure you use _docking_ branch, version tag _v1.90.6-docking_.
```bash
cd PotapychMind/vendor/ImGui
git checkout tags/v1.90.6-docking
```

How to link:
- Open your project, go to 'Project'->'Properties'->'Configuration Properties'->'VC++ Directories'
-- In 'Include Directories' add `$(ProjectDir)vendor\ImGui;$(ProjectDir)vendor\ImGui\backends;`
- Go to 'Project'->'Properties'->'Configuration Properties'->'Linker'
-- In 'Input' add `opengl32.lib;` to 'Additional Dependencies'
- Include these files into your project:
-- All **\*.h** and **\*.cpp** files from `vendor/ImGui`, except of **"imgui_demo.cpp"** (totally 10)
-- **"imgui_impl_opengl3.cpp / .h"**, **"imgui_impl_opengl3_loader.h"** and **"imgui_impl_win32.cpp / .h"** files from `vendor/ImGui/backends` (totally 5)

[//]: #links
[ImGui_github]: https://github.com/ocornut/imgui.git

## serial

-  GNU Lesser General Public License

Library files were taken from [this][cserial_files] site.
An article on how to use the library is available at [this][cserial_how-to-use] link.

How to link:
- Open your project, go to 'Project'->'Properties'->'Configuration Properties'->'VC++ Directories'
-- In 'Include Directories' add `$(ProjectDir)vendor\serial`

To avoid any errors when compiling the program, use these lines below to include the `serial.h` library in your code:
```c++
#define STRICT
#include <tchar.h>
#include <windows.h>
#include "serial.h"
```

[//]: #links
[cserial_how-to-use]: https://www.codeproject.com/Articles/992/Serial-library-for-C
[cserial_files]: https://www.codeproject.com/script/Articles/ViewDownloads.aspx?aid=992