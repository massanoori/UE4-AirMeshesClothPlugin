# UE4-AirMeshesClothPlugin

Prototype of UE4 plugin for cloth with multiple layers by AirMeshes.
This plugin cannot import meshes from FBX and support only rectangular shapes.

This is based on the paper "[Air Meshes for Robust Collision Handling (Direct link to pdf)](http://matthias-mueller-fischer.ch/publications/airMeshesPreprint.pdf)" by Matthias Muller.
[This article](https://www.unrealengine.com/blog/cable-component-plugin-for-ue4) on Epic's blog has provided me a valuable clue to implement this plugin.
This plugin uses [TetGen 1.5.0](http://wias-berlin.de/software/tetgen/) to tessellate the air between layers.

# Install

## Windows

1. Copy and Paste to your game project directory.
2. Right click the uproject file and "Generate Visual Studio project file".
3. Build your game project on Visual Studio.
