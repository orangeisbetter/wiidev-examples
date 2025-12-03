# Wii Homebrew Development Examples

This repository contains some very basic example programs to get started with Wii Homebrew development, up to and including a bouncing DVD logo.

## Building

To build these projects, you will need to have devkitpro and devkitppc installed, and environment variables set accordingly. Please install those tools [here](https://devkitpro.org/wiki/Getting_Started).

Checkout to the desired branch and type `make` in the repository root. The resulting `.dol` and `.elf` files will be created in the root directory.

## Running on a Wii

There are two methods to getting your homebrew running on a real Wii. You can copy the files to the SD card and launch it like any other homebrew application, or you can use a tool called `wiiload`
(which comes bundled with devkitpro) to directly send the file to your Wii over the internet.

### Copying the Files Manually

Follow the steps outlined [here](https://wiibrew.org/wiki/Homebrew_Channel#Configuring_Applications).

### Using Wiiload (preferred)

Make sure you have `WIILOAD` set as an environment variable in your system, and then run `make run` from the root of the repository. The app will be sent over the network and started automatically.

For more information, check [here](https://wiibrew.org/wiki/Wiiload).
