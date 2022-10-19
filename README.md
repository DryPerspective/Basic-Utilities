# Basic-Utilities
A collection of utilities and tools I use in several projects.

In my own projects, I often find myself using much of the same functionality both for use in the final project, and as part of the process (e.g. utility timers for evaluating executing speed). As such, I gathered several of the methods and objects I use frequently into this static library to centralise them and allow optimisations to one piece of code to apply to all projects.

The file layout of this project is also a little different from the standard generated by VS. As headers for static libraries are required in using the library, they have been separated from the source files into their own directory for easy access. They are found at MyLib/Headers; with the source files at MyLib/Source Files; and the pre-compiled lib file in the MyLib directory.

This code is built in C++17 and requires at least that standard to compile.

## Library Contents.

This section will document a brief overview of each part of the library and what it contains.

- **PhysicsVector** - A size-templated class to serve as a base vector object for physics simulations, with some optimisation targeting 2D and 3D vectors.

- **ConfigReader** - A class to read configuration files and copy the valued contained within into the program at runtime.

- **SimpleTimer and MultiTimer** - Classes which use the chrono header to track a single point in time, or multiple (respectively) and compare how much time has elapsed since.

- **IOFunctions** - Some basic boilerplate IO functions to read in data through the console with input validation.

- **BigInt** - A class to represent an arbitrarily sized (signed) integer, complete with arithmetic, comparison, and bitwise operators. Allows up to `std::numeric_limits<std::size_t>::max()`-bit integers before functionality breaks down. On the author's machine this corresponds to being able to represent ~5.55 x 10<sup>18</sup> decimal digits, or a range of ± ~10<sup>10<sup>18.74</sup></sup>.
