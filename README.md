![ENNOBLE LOGO](Images/Logo.png "ENNOBLE LOGO")

<br>

<p align="center">
  <img alt="license" src="https://img.shields.io/badge/License-MIT-green.svg"/>
  <img alt="license" src="https://img.shields.io/badge/Platform-Windows-green.svg"/>
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

<br>
<hr>

<p align="center">
  <img alt="license" src="https://github.com/paskalian/Ennoble/blob/master/Images/EnnobleMainframe.png"/>
</p>

<hr>
