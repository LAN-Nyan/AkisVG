# AkisVG - Fixed and Enhanced Version

## Overview
This is the fixed and enhanced version of AkisVG (formerly "Lumina Studio"), a professional vector animation suite. All major issues have been addressed and several new features have been added.

## Fixed Issues

### 1. ✅ Branding Updates
- **Changed all references** from "Lumina Studio" to "AkisVG"
- **File extensions** changed from `.lumina` to `.akisvg`
- **Window titles**, About dialog, and all UI elements updated
- **CMakeLists.txt** project name corrected

### 2. ✅ Canvas Centering
- **Fixed canvas centering** on window resize
- **Changed resize anchor** to `AnchorViewCenter` for proper centering behavior
- **Middle mouse button panning** implemented - click and drag with middle mouse to pan
- **Space key panning** retained for alternative panning method
- Canvas now stays centered even when onion skinning is enabled

### 3. ✅ Project Save/Load System
- **Implemented JSON-based project saving** with full metadata
- **Project properties saved**: width, height, FPS, frame count, smooth paths setting
- **Onion skin settings saved**: enabled state, before/after counts, opacity
- **Layer data saved**: name, visibility, locked state, opacity, color, type
- **Proper error handling** with user feedback on save/load failures
- **File extension**: `.akisvg` files

### 4. ✅ GIF Export
- **Fixed broken GIF export** using FFmpeg
- **Single frame GIFs** use Qt's native writer for simplicity
- **Animated GIFs** use FFmpeg with high-quality palette generation
- **Proper frame rate** calculation based on project FPS
- **Looping support** - infinite loop option
- **Error messages** guide users to install FFmpeg if missing
- **Temporary files cleaned up** automatically after export

### 5. ✅ Interpolation System (NEW MAJOR FEATURE!)
- **VectorGroup class** - Group multiple vector objects together
- **Keyframe system** - Add keyframes at specific frames for groups
- **Automatic interpolation** - Smooth transitions between keyframes
- **Multiple easing functions**:
  - Linear
  - EaseIn/Out/InOut (Quadratic)
  - EaseIn/Out/InOut (Cubic)
- **Transform support**: Position, rotation, scale, opacity
- **InterpolationEngine** - Central system for managing all interpolations

### 6. ✅ UI Improvements
- **Cleaner color scheme** with improved dark theme
- **Better consistency** in panel styling
- **Reduced visual clutter** in menus and toolbars
- **Professional appearance** matching modern animation software

### 7. ✅ Code Quality
- **Warning fixes**: Unused parameters properly handled or removed
- **Deprecated API fixes**: `QColor::isValidColor()` warnings resolved
- **Better includes**: Added missing headers for mouse and resize events
- **Proper memory management**: Safe deletion of temporary directories in GIF export

## New File Structure

```
src/
├── core/
│   ├── interpolation.h       ← NEW: Interpolation system
│   ├── interpolation.cpp     ← NEW: Implementation
│   ├── project.h             ← UPDATED: Save/load methods
│   └── project.cpp           ← UPDATED: JSON serialization
├── canvas/
│   ├── canvasview.h          ← UPDATED: Panning support
│   └── canvasview.cpp        ← UPDATED: Middle mouse, centering
├── io/
│   ├── gifexporter.h
│   └── gifexporter.cpp       ← FIXED: FFmpeg integration
└── mainwindow.cpp            ← UPDATED: Branding, save/load calls
```

## How to Use New Features

### Interpolation/Tweening

1. **Create Vector Objects** - Draw circles, rectangles, text, etc.

2. **Group Objects** - Select multiple objects and group them
   ```cpp
   VectorGroup *group = new VectorGroup("Atom Animation");
   group->addObject(circleObject);
   group->addObject(textObject);
   ```

3. **Add Keyframes** - Set keyframes at start and end positions
   ```cpp
   InterpolationEngine *engine = new InterpolationEngine();

   // Frame 1: Start position
   group->setPosition(QPointF(100, 100));
   engine->addKeyframe(group, 1);

   // Frame 24: End position  
   group->setPosition(QPointF(500, 100));
   engine->addKeyframe(group, 24);
   ```

4. **Set Easing** - Choose how the animation accelerates
   ```cpp
   engine->setEasingType(group, 1, 24, EasingType::EaseInOut);
   ```

5. **Render In-Between Frames** - The engine automatically calculates
   ```cpp
   for (int frame = 1; frame <= 24; frame++) {
       Keyframe *kf = engine->getInterpolatedFrame(group, frame);
       if (kf) {
           group->setPosition(kf->position);
           group->setRotation(kf->rotation);
           group->setScale(kf->scale);
       }
   }
   ```

### Middle Mouse Panning

- **Click and hold middle mouse button** (scroll wheel button)
- **Drag** to pan around the canvas
- **Release** to stop panning
- Alternative: **Hold Space** and drag with left mouse

### Saving Projects

1. **File → Save** (Ctrl+S) - Save to current file
2. **File → Save As** (Ctrl+Shift+S) - Save with new name
3. Files saved as `.akisvg` format
4. All project settings preserved

### GIF Export

1. **File → Export to GIF (All Frames)** - Export every frame
2. **File → Export to GIF (Keyframes)** - Export only keyframes
3. **Requirements**: FFmpeg must be installed
   ```bash
   # Arch Linux
   sudo pacman -S ffmpeg

   # Ubuntu/Debian
   sudo apt-get install ffmpeg

   # macOS
   brew install ffmpeg
   ```

## Remaining TODOs (Future Enhancements)

### Audio Import
- Audio layer support is implemented in Layer class
- Need to add UI for importing audio files
- Need to wire up `QMediaPlayer` for playback
- Waveform visualization already scaffolded

### Image Import
- ImageObject class exists
- Need to add proper file dialog integration
- Need to test various image formats

### Brush Textures
- Add texture/normal map support to brush tools
- Implement "rub-off" effect for realistic pen strokes
- Pressure sensitivity for tablets

### Fill Tool Improvements
- Current implementation needs consistency checks
- Add flood fill algorithm improvements
- Better color tolerance handling

## Building the Project

```bash
cd AkisVG_Fixed
mkdir -p build && cd build
cmake ..
cmake --build .
./AkisVG
```

## Dependencies

- Qt 6.x (Core, Widgets, Gui, Svg, Multimedia)
- C++17 compiler
- CMake 3.16+
- FFmpeg (runtime dependency for GIF export)

## Testing Recommendations

1. **Test Save/Load**: Create a project, add layers, save, close, reopen
2. **Test GIF Export**: Create simple animation, export to GIF
3. **Test Canvas Centering**: Resize window, verify canvas stays centered
4. **Test Middle Mouse Panning**: Use middle mouse to navigate
5. **Test Interpolation**: Create groups, set keyframes, verify smooth motion

## Known Limitations

- **Vector object serialization** not yet complete (save/load saves metadata but not drawn content)
- **Full interpolation UI** not yet integrated into main window
- **Audio import** needs file dialog wiring
- **Brush textures** need implementation
