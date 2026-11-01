# usbnotify plan

usbnotify shows a desktop notification each time you plug in or remove a USB device. It reads events from the kernel and sends them to dunst over D-Bus. The program has three files: a header, a source file, and a CMake file.

This plan says what to build and where to learn it. It does not say how to write it.

## Data flow

```
kernel --netlink socket--> raw bytes --parse--> your event struct
                                                     |
                                    sysfs files --> name
                                                     |
dunst  <--------------- D-Bus method call <-- title and body
```

## Files

| File | Job |
| --- | --- |
| Header | Your event struct, and the declarations of your functions |
| Source file | The function bodies and `main` |
| `CMakeLists.txt` | The build instructions |

Plan one function for each job in the data flow: open the socket, receive one message, parse one message, find a device name, and send a notification. Decide the parameters and return types yourself.

Read about header guards and `#pragma once` before you write the header.

## How a C++ program is built

Two programs turn your source into an executable. CMake only tells them what to do.

1. The compiler turns each source file into an object file. It reads your header and the system headers. A system header only declares functions: it gives their names and types, but not their code.
2. The linker joins the object files into one executable. For each function that your code calls, it must find the code. Your own functions are in your object file. The sd-bus functions are in a library.

A library is a file of compiled code that many programs share. sd-bus is in `libsystemd`, a shared library in `/usr/lib`. Linking a library means that you tell the linker which library file to search.

Learn to tell two errors apart. A compiler error about a missing file means a missing header or include path. A linker error with "undefined reference" means a missing library or a missing source file.

A shared library is not copied into your program. The program only records its name, and the dynamic loader loads it at each start. Find the tool that lists the shared libraries of a program, and use it on usbnotify.

pkg-config is a tool that tells build tools which flags a library needs. Each library describes itself in a `.pc` file. Find the file for `libsystemd`, read it, and ask pkg-config for its flags.

The netlink socket and the sysfs files need no extra library. Find out which library gives you `socket`, `bind`, and `recv`, and why you never link it by hand.

## The CMake file

Your CMake file must do these things:

1. State the minimum CMake version.
2. Name the project and its language.
3. Require C++20, with no compiler extensions.
4. Optional: export the compile commands, so clangd understands your includes.
5. Load CMake's pkg-config support, and find `libsystemd` through it as an imported target.
6. Create the executable target from your source file.
7. Link the imported target into your executable.
8. Turn on compiler warnings for your target.

A target is a named thing that CMake builds or uses. It carries its own sources, flags, and libraries. Learn the difference between a normal target and an imported target.

Leave items 5 and 7 out until step 5 of the program. Then add them together with the notification code.

Learn the two CMake commands: one configures (reads the CMake file and writes build files into a build folder), and one builds. Find the option that shows the real compiler and linker commands, and look for your library on the link line.

Read:

- CMake tutorial: https://cmake.org/cmake/help/latest/guide/tutorial/
- `cmake_minimum_required`, `project`, `add_executable`, `target_link_libraries`, and `target_compile_options` in the CMake command reference.
- The `CMAKE_CXX_STANDARD` and `CMAKE_EXPORT_COMPILE_COMMANDS` variables.
- The FindPkgConfig module, especially `pkg_check_modules` and `IMPORTED_TARGET`: https://cmake.org/cmake/help/latest/module/FindPkgConfig.html
- The meaning of `PRIVATE`, `PUBLIC`, and `INTERFACE` on target commands.

## Steps

Each step ends with a program that you can run.

1. Build setup. Write the CMake file and an empty header. Print one line from `main`.
2. Raw events. Open a netlink socket for kernel uevents and print every message. Plug in a USB stick.
3. Parsing. Turn each message into your event struct. Print one line for each plug and one for each removal.
4. Names. Find a readable device name in sysfs, and show it for both the plug and the removal.
5. Notifications. Send each event to dunst as a desktop notification.
6. Optional extras. Exit cleanly on Ctrl+C, update one popup instead of stacking two, and start usbnotify at login.

## Step 2: raw events

A netlink socket connects a program to the kernel, not to another computer. The kernel sends a "uevent" message each time a device appears, changes, or goes away.

What to find out:

- Which address family, socket type, and netlink protocol receive kernel uevents.
- How to fill the netlink address struct before you bind, and what its multicast group field means.
- Which multicast group carries the raw kernel messages, and which one carries udev's processed copies.
- How many bytes one message needs, and whether one receive call gives a whole message.
- What to do after a system call is interrupted by a signal.

Read: `man 7 netlink`, `man 2 socket`, `man 2 bind`, `man 2 recv`, `man 7 signal` (the part about interrupted system calls).

Watch out: the message contains zero bytes in the middle. Pick a C++ type that can hold them, and print it in a readable way.

Test: plug in a USB stick and compare your output with `udevadm monitor --kernel --property`.

## Step 3: parsing

Study your step 2 output first. Find the separator between fields, the header line, and the shape of the other fields.

Design a struct with only the fields you need. You need at least the action, the device path, and some device id from the message. The device path is the same for the plug and the removal of one device, so it works as a key in step 4.

Keep only the plug and the removal of a whole USB device. Find out which fields and values identify these events.

Watch out:

- One USB stick sends several messages: one for the device, more for its interfaces, and more for the disk under it. Your filter must leave exactly one event for each plug and one for each removal.
- The kernel also sends actions other than plug and removal. Ignore them.
- A view into a string is valid only while that string exists. Decide where you copy the values.

Read on cppreference.com: `std::string_view` (`find`, `substr`, `starts_with`) and `std::optional`.

Test: one plug and one removal print exactly two lines.

## Step 4: names

The message has only numbers. sysfs is a folder of files under `/sys` that the kernel creates for each device. Explore it by hand, and find the files that hold the manufacturer and the product name of a USB device.

Problems to solve:

- Some cheap devices have no name files. Choose a fallback text.
- On removal, the sysfs folder of the device is already gone. Remember each name at the plug, and use it again at the removal. Choose a container for this, and clean up the entry after the removal.

Read: https://www.kernel.org/doc/Documentation/ABI/stable/sysfs-bus-usb, and `std::ifstream`, `std::getline`, and `std::unordered_map` on cppreference.com.

Test: a plug and a removal show the same readable name.

## Step 5: notifications

D-Bus is a message bus that desktop programs use to call methods on each other. dunst listens on your session bus. sd-bus is the D-Bus client library in `libsystemd`.

What to find out:

- The bus name, object path, interface, and method of the notification service. Find them with `busctl --user list` and `busctl --user introspect`.
- The arguments of the method, their order, and their D-Bus types, from the Desktop Notifications specification.
- How sd-bus opens a connection to the session bus, calls a method, reports errors, and frees what it gave you.
- How sd-bus takes an empty array as an argument.

Call the method once from the shell with `busctl --user call` before you write any C++.

Read:

- Desktop Notifications specification: https://specifications.freedesktop.org/notification-spec/latest/
- `man 3 sd_bus_open_user`, `man 3 sd_bus_call_method`, `man 3 sd_bus_message_append` (its table explains each type letter), `man 3 sd_bus_error`.
- Introduction to sd-bus: https://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html
- The freedesktop icon naming specification, to choose an icon.

Watch out: sd-bus takes its arguments as C varargs. The compiler does not convert or check their types for you.

Test: a plug and a removal each show a dunst popup with the device name.

## Step 6: optional extras

Clean exit: make Ctrl+C end the loop, so the program closes the socket and the bus connection. Read `man 2 signalfd` and `man 2 poll`.

One popup for each device: the notification method returns an id. Find the argument that replaces an existing notification, and use it at the removal.

Start at login: install the binary into a folder on your `PATH`, and start it from the `hyprland.start` block in `~/.config/hypr/hyprland.lua`.
