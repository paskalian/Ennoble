![ENNOBLE LOGO](Images/Logo.png "ENNOBLE LOGO")

<br>

<p align="center">
  <img alt="license" src="https://img.shields.io/badge/License-MIT-green.svg"/>
  <img alt="platform" src="https://img.shields.io/badge/Platform-Windows-green.svg"/>
</p>

## Introduction

Ennoble as it states is an easy to use dynamic memory searcher and offset dumper with very flexible configuration options.

The project was meant to be an external application but turned out to be an internal dynamic link library (.dll) allowing it to have more control over the application you are offset dumping.

<br>

## Usage

1. Compile the given solution.
2. Inject the resulting Dynamic Link Library (.dll) into your target application.
3. Enter the configuration file (.json) path which is specifically made for your target application.
> **_EXTRA:_**  If Runtime Type Information (RTTI) is used for finding class offsets, you will be asked to enter the base class instance address.

<br>

## Configuration

Configuration files are accepted as in **.json** format.

There are two main arrays which **must** be in all configurations.
- classes
- functions
> **_NOTE:_** The arrays can be empty but must be present.

<br>

Classes and Functions are as their name also states is the two base arrays used for checking which classes and functions you are planning to dump.

<hr>

<p align="center">
  <img alt="mainframe" src="https://github.com/paskalian/Ennoble/blob/master/Images/EnnobleMainframe.png"/>
</p>

<hr>

There are two important things to keep in mind:
1. If an object's enabled member is set to false, the following members are not required to be set. (can be set but will be ignored eventually)
2. Unlike classes, functions doesn't have the read member on it's object, this is because it will not read anything from the code to get the offset.

<br>

#### Objects Explained
<pre>
classes ➜ Base array for which classes to dump.
   |
   ↳ class_name ➜ The class name.
   |
   ↳ module ➜ The module which the offset will be searched on.
   |
   ↳ offsets ➜ An array of offset objects to search and dump.
        |
        ↳ name ➜ Offset name. (If Runtime Type Information (RTTI) is used for finding this offset
        |         then this name must be the exact class name used)
        |
        ↳ rtti_search ➜ An object controlling rtti search.
        |     |
        |     ↳ enabled ➜ true/false, is rtti search enabled or not.
        |     |
        |     ↳ limit ➜ As stated above, since rtti will be used you will be asked an base class
        |                instance address, this member sets the max distance to take from the base class.
        |
        ↳ stringref_search ➜ An object controlling string reference search.
        |     |
        |     ↳ enabled ➜ true/false, is string reference search enabled or not.
        |     |
        |     ↳ string ➜ The string to search it's references and use that reference as a base.
        |     |
        |     ↳ pattern ➜ The byte pattern to search for using the string reference as the base address.
        |     |
        |     ↳ limit ➜ Max distance to take from the base address, if it's negative then it will subtract
        |     |          that from the base address and use the result as the new base address, which will
        |     |          search until the old base address.
        |     |          
        |     |
        |     ↳ read ➜ Amount of bytes to read after finding the pattern and adding
        |     |         or subtracting the extra from the found pattern address.
        |     |
        |     ↳ extra ➜ Amount of bytes to add or subtract from the found pattern address.
        |
        ↳ signatures ➜ An object controlling signature search.
              |
              ↳ pattern ➜ The byte pattern to search for using the module base as the base address.
              |
              ↳ extra ➜ Amount of bytes to add or subtract from the found pattern address.
              |
              ↳ read ➜ Amount of bytes to read after finding the pattern and adding
                        or subtracting the extra from the found pattern address.

functions ➜ Base array for which functions to dump.
   |
   ↳ function_name ➜ The function name.
   |
   ↳ module ➜ The module which the function will be searched on.
   |
   ↳ stringref_search ➜ An object controlling string reference search.
   |     |
   |     ↳ enabled ➜ true/false, is string reference search enabled or not.
   |     |
   |     ↳ string ➜ The string to search it's references and use that reference as a base.
   |     |
   |     ↳ pattern ➜ The byte pattern to search for using the string reference as the base address.
   |     |
   |     ↳ limit ➜ Max distance to take from the base address, if it's negative then it will subtract
   |     |          that from the base address and use the result as the new base address, which will
   |     |          search until the old base address.
   |     |
   |     ↳ extra ➜ Amount of bytes to add or subtract from the found pattern address.
   |
   ↳ signatures ➜ An object controlling signature search.
         |
         ↳ pattern ➜ The byte pattern to search for using the module base as the base address.
         |
         ↳ extra ➜ Amount of bytes to add or subtract from the found pattern address.
</pre>
