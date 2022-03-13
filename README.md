# Zoom floating window hider
#### Use this tool if the floating windows that keep appearing while sharing your screen do annoy you. If you are unaware of the implications: while sharing a window, any popup/floating window cause a black rectangle to show up on top of the shared area, which is annoying.

### How to use
1. Create a folder in AppData/Roaming/ZoomFloatingWindowHider

3. Move all the files from ./Release to ./ZoomFloatingWindowHider

5. Create a shortcut for the ./start.exe

#### You can compile the source files yourself

When you want to start Zoom without floating windows call the start.exe which will patch the app in memory. It is a simple runtime patch that detours the Win32 classical window registration functions.