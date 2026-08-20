# Mark_1_DX3D Project Documentation

## Project Overview

**Mark_1_DX3D** is a DirectX 11 graphics application for Windows 10. The project is built using the Rastertek DirectX 11 tutorial series as a foundation and reference implementation.

- **Technology**: DirectX 11 (Direct3D)
- **Platform**: Windows 10+
- **Build System**: Visual Studio (C++)
- **Purpose**: Real-time 3D graphics rendering with post-processing effects (color grading)

## Project Structure

```
Mark_1_DX3D/
├── CLAUDE.md                           # This file
├── Mark_1_DX3D/
│   ├── Application.cpp                 # Main application logic
│   ├── Application.h                   # Application class header
│   ├── Mark_1_DX3D.vcxproj            # Visual Studio project file
│   └── Mark_1_DX3D.vcxproj.filters    # Visual Studio filter configuration
└── Resource/
    └── dx11win10tut61_src/             # Rastertek Tutorial #61 Reference Code
        ├── source/                     # C++ source files
        └── data/                       # Assets (textures, meshes, lookup tables)
```

## Resource Folder - Rastertek Tutorial Reference

The **Resource/dx11win10tut61_src** folder contains reference code from **Rastertek's DirectX 11 Windows 10 Tutorial Series, Tutorial 61**.

**Source**: https://www.rastertek.com/tutdx11win10.html

### Key Components in Resource Folder:

#### Core System
- **SystemClass** - Windows application framework, window management, message loop
- **ApplicationClass** - Main application orchestrator

#### Graphics & Rendering
- **D3DClass** - DirectX 11 device and context management
- **ModelClass** - 3D model loading and rendering
- **CameraClass** - Camera/view matrix management
- **TextureClass** - Texture loading and binding

#### Lighting & Effects
- **LightClass** - Light source definitions
- **LightShaderClass** - HLSL shaders for light calculations
- **RenderTextureClass** - Off-screen rendering to texture (for post-processing)

#### Post-Processing
- **ColorGradeShaderClass** - Color grading effect shader
- **ClutClass** - Color Lookup Table (CLUT) for color correction
- **OrthoWindowClass** - Orthographic rendering for UI overlays

#### Assets
- **Shaders**: `light.vs`, `light.ps`, `colorgrade.vs`, `colorgrade.ps`
- **Textures**: `stone01.tga`
- **Data**: `clut.dat` (color lookup table), `cube.txt` (mesh data)

### Purpose
The Resource folder serves as:
1. **Educational reference** - Study how each component is implemented
2. **Code templates** - Copy patterns and structures into Application.cpp/h
3. **Asset base** - Reference textures and shaders for testing

## Architecture Overview

### Rendering Pipeline
1. **SystemClass** initializes Windows and manages the main loop
2. **ApplicationClass** orchestrates the rendering:
   - Renders 3D scene to an off-screen texture
   - Applies color grading post-process effect
   - Displays final result to screen
3. **D3DClass** manages DirectX device and rendering contexts
4. **Shaders** (HLSL) execute lighting and color correction on GPU

### Key Workflow
- **Input** → InputClass detects keyboard/mouse input
- **Update** → Camera and model positions updated based on input
- **Render** → Scene rendered to texture, then post-processed
- **Display** → Final frame presented to window

## Build & Setup

### Requirements
- Visual Studio 2015 or later
- Windows 10 SDK
- DirectX 11 capable GPU

### Build Steps
1. Open `Mark_1_DX3D.vcxproj` in Visual Studio
2. Configure DirectX SDK paths if needed
3. Build solution (Ctrl+Shift+B)
4. Run the application

### Project Configuration
- **Platform**: Windows (x86/x64)
- **Language Standard**: C++11 or later
- **DirectX Version**: Direct3D 11

## Coding Conventions

### Class Naming
- Classes use `*Class` suffix (e.g., `SystemClass`, `D3DClass`)
- Member variables prefixed with `m_` (e.g., `m_Direct3D`)
- Static members prefixed with `s_` (e.g., `s_Instance`)

### File Organization
- Header files (.h) in source folder
- Implementation files (.cpp) in source folder
- Shaders (.vs, .ps) stored alongside classes that use them

### Shader Language
- Vertex shaders: `.vs` files
- Pixel shaders: `.ps` files
- HLSL (High-Level Shader Language)

## Integration Notes

When adapting Rastertek tutorial code into `Application.cpp/h`:
1. Replace class names if needed to fit project conventions
2. Remove hardcoded window size constants; make configurable
3. Adapt shader file paths to your project structure
4. Test each component independently before integration
5. The tutorial code uses `D3D_FEATURE_LEVEL_11_0` - ensure GPU supports it

## Important Conventions

- **Memory Management**: Manual new/delete with null checks
- **Error Handling**: Functions return `bool` for success/failure
- **Initialization Order**: Always Initialize() before use, Shutdown() before delete
- **Global Handles**: ApplicationHandle global pointer used for Windows message routing

## References

- **Rastertek Tutorials**: https://www.rastertek.com/tutdx11win10.html
- **DirectX Documentation**: https://learn.microsoft.com/en-us/windows/win32/direct3d11/atl-using-direct3d-11
- **HLSL Shader Syntax**: https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl
