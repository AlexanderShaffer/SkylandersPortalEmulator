# Contact Information

**Alex Shaffer:** alexander.shaffer.623@gmail.com

If you have any questions, feature/improvement suggestions, or wish to report a bug, please do not hesitate to email me.

___

# Credits

|      **Usage**      |                                                                                                                                                                                                                                        **Links**                                                                                                                                                                                                                                        |         **Author**          |  **License**  |
|:-------------------:|:---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------:|:---------------------------:|:-------------:|
|      GUI Font       |                                                                                                                                                 [Techna Sans Font](https://www.fontspace.com/techna-sans-font-f34640) embedded in [Font.cppm](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/gui/src/Font.cppm)                                                                                                                                                 |         Carl Enlund         | SIL Open Font |
| Configuration Files | [pico_sdk_import.cmake](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/pico/pico_sdk_import.cmake), [FreeRTOSConfig.h](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/pico/config/FreeRTOSConfig.h), [lwipopts.h](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/pico/config/lwipopts.h), and [tusb_config.h](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/pico/config/tusb_config.h) | Raspberry Pi (Trading) Ltd. | BSD-3-Clause  |

___

# Background

Skylanders is a video game franchise that features six games. Players must purchase figurines and a USB device called a portal. Each game introduces a new portal and many new figures. The portal is a circular platform that contains an NFC reader.

When a figure is placed on the portal, it is digitally transferred into the game. While on the portal, the game periodically writes data to the figure. For example, the number of gold coins collected or upgrades purchased. When the figure is removed from the portal, it is also removed from the game. Each game contains levels for the player to progress through. Each level contains enemies that the player must defeat using their figures. There are six types of figures:

1. **Skylanders:** a creature that the player controls in game to defeat enemies. Each skylander has unique attacks and upgrades. If a skylander is defeated in game, the player must choose a different skylander to place on the portal or restart the level. All skylanders are revived after a level ends or is reset.
2. **Swappers:** a skylander composed of a top half and a bottom half. The player may interchange the top and bottom halves of swappers, partially combining their attacks and upgrades.
3. **Traps:** a crystal that allows the player to trap specific enemies. For a limited amount of time, the player may control trapped enemies in game to defeat enemies.
4. **Vehicles:** used by skylanders to help defeat enemies and explore levels. There are three vehicle types: land, sea, and sky.
5. **Creation Crystals:** allow the player to create their own skylander using parts found in levels. The skylander is stored inside the physical figure and may be modified by the player at any time while on the portal. 
6. **Magic items:** some grant the player a unique advantage and have exactly one use per level. Other magic items unlock new levels or racing tracks.

The fourth game in the franchise, _Skylanders: Trap Team_, introduced a portal with an embedded speaker. When a trap containing an enemy is placed on the portal, the enemy occasionally speaks to the player through this speaker. Also, the portal plays audio while an enemy is being trapped and music in specific level areas. _Skylanders: Trap Team_ is the only game that plays audio through the portal.

___

# Project Description

This project emulates Skylanders portals using a GUI and a Raspberry Pi Pico W. The Pico is programmed to be compatible with all six games and the PS3, PS4, Wii, and Wii U consoles. Additionally, the GUI and Pico support all six figure types. The figures are digitally stored on the computer running the GUI. Then, users may command the GUI to wirelessly send the figures to the Pico, which sends them to the game running on a console. In game, the emulated figures function and behave exactly the same as the original figures. Lastly, the GUI will play the same audio as the original portal.

Note that this project is in early development and therefore has insufficient instructions for how to build the source code. Please read the **Planned Improvements and Features** section for more information.

___

# How to Use the Emulator

## Initial Setup

Currently, the GUI and Pico source code must be recompiled for each Wi-Fi network. For the GUI, the _PICO_IPV4_ADDRESS_ CMake cache variable must be defined. For the Pico, the _WIFI_SSID_, _WIFI_PASSWORD_, and _HOST_IPV4_ADDRESS_ CMake cache variables must be defined.

## Configuring the GUI 

Figures may be added to or removed from the GUI at any time. The GUI periodically searches through a directory named _Figures_ in the current working directory, which is typically the same directory as the executable. Within the _Figures_ directory, the GUI expects subdirectories with any name, which each contain _Icons_ and _Dumps_ directories. These subdirectories allow the user to organize figures into named groups, making the GUI easier to navigate.

The _Dumps_ directories are expected to contain _.dump_ files that each store the data of one figure. The _Icons_ directories are expected to contain _.jpg_ images.

The GUI recognizes _.dump_ and _.jpg_ files with the same name as the same figure. All figures found by the GUI are displayed as image buttons and organized into groups according to the subdirectories in the _Figures_ directory.

Within a subdirectory in the _Figures_ directory, the GUI will search through any _Bottom Halves_ or _Top Halves_ directories that each contain _Dumps_ and _Icons_ directories. Pairs of _.dump_ and _.jpg_ files with the same and located in _Bottom Halves_ and _Top Halves_ directories are recognized as a swapper. Swappers recognized by the GUI are drawn with their top and bottom halves connected together.

Figure _.dump_ files and images can easily be found through a Google Search. Alternatively, there exists software that can extract _.dump_ files from figures placed on a NFC reader connected to a PC. A comprehensive list of figure _.dump_ files from each game can be downloaded from [this GitHub repository](https://github.com/skylandersNFC/Skylanders-Ultimate-NFC-Pack). 

## Playing the Games with the Emulator

After booting the game, connect the Pico into the video game console. If the GUI is open, the Pico will automatically connect to it. Ensure the location of the Pico does not disrupt the connection by maximizing the line of sight to the internet modem. The Wi-Fi antenna on the Pico W is weak enough that simply wrapping your hands around the chip will compromise the connection.

After the GUI-Pico connection is established, any figures displayed by the GUI should light up. If the figures appear dark, then no connection is established.

Audio data sent by the game to the Pico is relayed to GUI and played.

If there are too many figures to display all at once, use the scroll wheel to scroll up and down.

Left-clicking a figure causes the GUI to transfer it to the Pico and into the game. The GUI displays all active figures in a gold tint. Left or right-clicking the figure again will remove it from the game, and any modifications to the figure by the game are saved. Like the original portals, at most 16 figures can be active at a time.

Right-clicking a figure causes it to display in a red tint. Because the game never notifies the portal when a figure is defeated or used, the red tint is intended to help the user keep track of used and defeated figures. However, the user may interpret the red tint in any way they wish. Marking a figure red is purely visual and serves no functional purpose. Left or right-clicking the figure again will remove the red tint.   

Pressing the middle mouse button removes all figures from the game and red tints. This action is helpful after all figures are revived when a level in the game ends or is reset.

## Demonstration Video

[This YouTube video](https://www.youtube.com/watch?v=lvt3XxA4824) demonstrates how to use the portal emulator after the initial setup. In other words, it is a summary of the previous two subsections. The video begins by showing how to configure the GUI. Then, the video summarizes how the emulator functions while playing the game. All six PS3 versions of the games and all six figure types are showcased. Finally, the video ends by proving that skylander upgrades purchased and gold coins remaining are retained even after restarting the GUI.

___

# How the Emulator Works

## GUI Dependencies

- **Dear ImGui:** GUI library used to display the figure buttons in a grid with appropriate padding.
- **GLEW and OpenGL:** Backends for Dear ImGui, and used by this project to load figure images into GPU memory.
- **SDL3:** Backend for Dear ImGui, and used by this project to play audio sent by the Pico.
- **stb_image:** Used to load figure images from secondary storage to main memory.
- **Winsock2:** Windows networking API used to allow the GUI to communicate with the Pico.

## Pico Dependencies

- **PicoSDK and other related dependencies:** Libraries required to write C or C++ programs for the Pico.
- **Picotool:** Required by PicoSDK to translate _.elf_ to _.uf2_ binaries.
- **LWIP:** Networking API used to allow the Pico to communicate with the GUI.
- **FreeRTOS:** Operating system used to enable the LWIP socket API and schedule threads.
- **TinyUSB:** Used to allow the Pico to communicate with the video game console.

## Reverse Engineering the Original Portals

Wireshark was used to obtain the device descriptor, configuration descriptor, hid report descriptor, and string descriptors of the portals. This information was copied directly into [UsbDescriptors.cpp](https://github.com/AlexanderShaffer/SkylandersPortalEmulator/blob/main/pico/config/UsbDescriptors.cpp), causing the video game console to recognize the Pico as a portal.

Then, _libusb_ was used to relay all USB packets sent by a portal connected to a computer to the Pico. After that, the Pico relayed the same packets to the video game console. The same process was repeated in reverse for USB packets sent by the console.

With this setup, figures were placed and removed from the portal, and the USB packets sent by the portal and console were captured. These packets were then analyzed and reverse engineered.

## Adding and Removing Figures from the Game

To minimize connectivity issues and data corruption, figure data is sent by the GUI in one TCP packet and stored in the Pico's main memory. Both the GUI and Pico store arrays of length 16 to hold active figures. The number 16 was chosen because the original portals also support up to 16 active figures. Each element in these arrays are referred to as portal slots in this project's source code. The Pico notifies the console when a new figure has been received, and the console responds by requesting 16 byte blocks of the figure data at a time. After all the figure data is transferred to the console, the console occasionally demands the Pico to write data to the figure. When the user wishes to remove the figure from the game, the Pico sends the modified figure data to the GUI in one TCP packet. Then, the GUI writes the figure data to its corresponding _.dump_ file.

If the user quickly requests multiple figures to be added or removed, then the oldest requests are completed first. This feature allows the GUI and Pico to recover if a disconnection occurs. However, it introduces a thread synchronization problem between the TCP sender, TCP receiver, and renderer threads in the GUI. To synchronize these threads, locks and condition variables are used.

## Adding and Removing Figures from the GUI

Figure _.jpg_ files are loaded from secondary storage to main memory by a loader thread rather than the renderer thread. This decision maximizes the GUI response time, but introduces a thread synchronization problem between the renderer and loader threads. OpenGL functions must be called on threads with an OpenGL context. It is often discouraged to have multiple threads with OpenGL contexts since exactly one context can be active at a time. Therefore, the loader thread sends the figure image data to the renderer thread. Then, the renderer thread transfers the image data into GPU memory, creating a texture. When the user removes a figure from the GUI, the loader thread notifies the renderer thread to unload the figure's texture from the GPU.

## Disconnection Recovery

When a disconnection between the GUI and Pico occurs, a data recovery process is initiated. This process compares the 16 pairs of portal slots between the GUI and Pico. If the Pico has an active figure at an index, but the GUI does not have an active figure at the same index, then the figure data is lost. Without the ability to decrypt figure data, it is impossible to recover the figure data in this case. If the GUI has an active figure at an index, but the Pico does not have an active figure at the same index, then the GUI sends the figure data to the Pico. In all other cases, no recovery is necessary.  

## Portal Audio

Audio data sent by the console to the Pico is relayed at a variable rate to the GUI using UDP packets. When the GUI receives the audio data, it plays it immediately and periodically notifies the Pico how many bytes are in its audio stream. Because the portal audio data has a sample rate of 8000 Hz and sample size of 2 bytes, the transfer rate must be at least 16,000 bytes per second. However, connectivity issues and the use of UDP packets can cause many bytes to be lost or delayed, which may temporarily cause the GUI's audio buffer to become empty. When this buffer empties, an audible stutter occurs. Therefore, the Pico requests audio data from the console at a higher rate when the GUI's audio buffer is low and vice versa. Originally, the Pico requested the audio data from the console as fast as possible to prevent stuttering. Unfortunately, this strategy causes some portal audio to play too early if multiple sounds are played in succession. Thus, the variable request rate strategy is the most effective strategy for maximizing audio quality.

## Pico Threads

The Pico has four threads:

- The LWIP main thread initialized by the LWIP library.
- A USB thread for communicating with the console, sending audio data to the GUI, and periodically pinging the GUI.
- A TCP sender/receiver thread for receiving and sending figure data to the GUI.
- A UDP receiver thread for receiving audio stream size notifications from the GUI.

These threads are managed by the FreeRTOS operating system. Also, the USB thread runs on a separate core from the other three threads. This decision was made because the USB thread has the greatest workload. This thread is constantly polling for USB packets sent by the console and must send UDP packets to the GUI. On the other hand, the other three threads are required to communicate with the GUI only, and communication with the GUI happens less often compared to the console.

___

# Planned Improvements and Features

- Distribute precompiled binaries for the GUI and Pico that do not require manual configuration.
- Configure CMake to automatically fetch external dependencies from GitHub.
- Add sufficient build instructions for the GUI and Pico source code that include the toolchains required.
- Switch the type of communication between the GUI and Pico from internet to Bluetooth. Then, the Pico would require line of sight with the computer running the GUI rather than the internet modem.
- Complete _TODO_ comments within the source code.
- Allow the user to change the size of the figure buttons in the GUI.
- Convert the implementation of the PortalEmulator and FigureLoader classes into modules. Resources used by these classes will be managed by RAII objects.
- Use object-oriented programing to improve the readability of the PortalEmulator implementation and Pico source code.
- Replace poor practice _std::println_ error logging with C++ exceptions.
- Join the threads started by the PortalEmulator and FigureLoader objects after the GUI window has finished closing. Currently, the window freezes for about one second before closing because the threads are joined too early.
- Split the Pico source code into multiple modules and eliminate all global variables with external linkage.
- Add a section within the GUI that allows users to add and remove figures. A comprehensive list of all figures would be distributed with the GUI binary, removing the need for users to source _.dump_ and _.jpg_ files themselves.
- Use _libusb_ to allow users to extract _.dump_ files from their physical figures through the NFC readers in the original portals. This feature will make it easier for users to transition from physical to digital figures.
- Periodically autosave figure data stored in the Pico's main memory to prevent accidental loss of data when the Pico is unplugged or the GUI is closed.