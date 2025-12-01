# HighDPI and UI Scaling Support Research Report

**Project:** RocketEditor
**Date:** 2025-11-30
**Objective:** Investigate changes required to support HighDPI displays and internal UI scaling across all platforms (macOS, Windows, Linux)

---

## Executive Summary

RocketEditor is a sync tracker application using a custom EMGUI library for UI rendering with OpenGL. Currently, the application does not properly support HighDPI displays (Retina, 4K, scaled Windows displays), resulting in blurry or incorrectly-sized UI elements on modern high-resolution monitors.

**Key Findings:**
- The application currently has hardcoded pixel dimensions throughout the codebase
- macOS explicitly disables Retina support (`setWantsBestResolutionOpenGLSurface:NO`)
- Windows lacks DPI awareness declarations
- Linux/SDL2 does not query display scale factors
- OpenGL viewport and orthographic projection need backing store resolution handling
- Font rendering requires re-rasterization at different scales
- Mouse coordinates need transformation from physical to logical pixels

**Recommended Approach:** Implement a scale-aware rendering system with:
1. Platform-specific DPI detection
2. Centralized scale factor management in EMGUI
3. Scaled font re-rasterization
4. Viewport/projection matrix adjustments
5. User preference for custom UI scaling (100%, 125%, 150%, 200%)

**Estimated Complexity:** Medium-High

---

## 1. Current State Analysis

### 1.1 Hardcoded Dimensions

The codebase contains numerous hardcoded pixel values:

**TrackView.c (lines 27-32):**
```c
#define font_size 8
int track_size_folded = 20;
int min_track_size = 100;
int name_adjust = font_size * 2;
int font_size_half = font_size / 2;
int colorbar_adjust = ((font_size * 3) + 2);
```

**Key Issues:**
- Font size is hardcoded to 8 pixels
- Track sizes are fixed integers
- All widget dimensions are pixel-based
- No scale factor is applied anywhere in the rendering pipeline

### 1.2 Font Handling

**Current Implementation:**
- Fonts loaded at fixed pixel heights via `Emgui_loadFontTTF(const char* ttfFontname, float fontHeight)`
- Default bitmap font "Microknight" at 8x8 pixels
- TrueType fonts rasterized once at load time using `stbtt_BakeFontBitmap()`
- Font textures created at fixed dimensions (512x512 in `emgui/src/Emgui.c:717`)

**Problem:** Fonts cannot be dynamically re-rasterized at different scales

### 1.3 OpenGL Rendering

**Current viewport setup (OpenGLBackend.c:62-68):**
```c
void EMGFXBackend_updateViewPort(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
}
```

**Issues:**
- `glViewport` uses window size directly (should use framebuffer/backing store size)
- `glOrtho` projection maps 1:1 to window coordinates (needs logical coordinate space)
- No distinction between window size (logical pixels) and framebuffer size (physical pixels)

### 1.4 Mouse Coordinate Handling

Mouse coordinates are set directly from platform events:
- **macOS:** `Emgui_setMousePos((int)location.x, (int)adjustFrame.size.height - (int)location.y)`
- **Windows:** `Emgui_setMousePos(pos_x, pos_y)`
- **Linux/SDL2:** `Emgui_setMousePos(ev->x, ev->y)`

**Problem:** On HighDPI displays, mouse coordinates are in physical pixels but UI is positioned in logical pixels, causing misalignment.

### 1.5 Current Behavior on HighDPI Displays

**macOS (Retina):**
- Explicitly disabled via `[self setWantsBestResolutionOpenGLSurface:NO]` (RocketView.m:55)
- Window renders at 1x scale, appears blurry on Retina displays
- macOS performs bitmap scaling to fill the display

**Windows:**
- No DPI awareness declaration
- Likely running in "System DPI Aware" mode by default
- UI may be bitmap-scaled by Windows on high-DPI monitors
- Text and controls appear blurry

**Linux:**
- SDL2 window created without `SDL_WINDOW_ALLOW_HIGHDPI` flag
- No DPI scale factor queries
- Relies on compositor scaling, resulting in blurry appearance

---

## 2. Platform-Specific Requirements

### 2.1 macOS HighDPI (Retina) Support

#### Key APIs and Properties

1. **Enable Retina Support:**
   ```objective-c
   [self setWantsBestResolutionOpenGLSurface:YES];
   ```
   - Currently set to `NO` in `src/macosx/RocketView.m:55`
   - Must be enabled before context creation

2. **Query Scale Factor:**
   ```objective-c
   CGFloat scaleFactor = [[self window] backingScaleFactor];
   ```
   - Returns 1.0 on non-Retina, 2.0 on Retina, potentially other values on newer displays
   - Should be queried whenever the window changes displays

3. **Detect Display Changes:**
   ```objective-c
   - (void)viewDidChangeBackingProperties {
       [super viewDidChangeBackingProperties];
       CGFloat newScale = [[self window] backingScaleFactor];
       // Update rendering with new scale
   }
   ```
   - Called when window moves between displays with different scale factors
   - Called when user changes display preferences

4. **Get Framebuffer Size:**
   ```objective-c
   NSRect backingBounds = [self convertRectToBacking:[self bounds]];
   int framebufferWidth = (int)backingBounds.size.width;
   int framebufferHeight = (int)backingBounds.size.height;
   ```

#### Implementation Pattern

```objective-c
// In RocketView.m initialization
- (id)initWithFrame:(NSRect)frame {
    // ...existing code...
    [self setWantsBestResolutionOpenGLSurface:YES]; // CHANGED FROM NO
    // ...
}

// Add method to detect display changes
- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];
    CGFloat scale = [[self window] backingScaleFactor];
    NSRect backingBounds = [self convertRectToBacking:[self bounds]];

    EMGFXBackend_updateViewPort((int)backingBounds.size.width,
                                (int)backingBounds.size.height);
    Emgui_setDisplayScale(scale);
    Editor_update();
}

// Update drawRect to use backing bounds
- (void)drawRect:(NSRect)frameRect {
    [oglContext update];
    g_window = [self window];

    NSRect backingBounds = [self convertRectToBacking:frameRect];
    CGFloat scale = [[self window] backingScaleFactor];

    EMGFXBackend_updateViewPort((int)backingBounds.size.width,
                                (int)backingBounds.size.height);
    Editor_setWindowSize((int)frameRect.size.width,
                        (int)frameRect.size.height);
    Emgui_setDisplayScale(scale);
    Editor_update();
}
```

**Sources:**
- [HiDPI / backingScaleFactor on MacOS - Stack Overflow](https://stackoverflow.com/questions/54429178/hidpi-backingscalefactor-on-macos-how-to-get-actual-value)
- [APIs for Supporting High Resolution - Apple Developer](https://developer.apple.com/library/archive/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/APIs/APIs.html)
- [Advanced NSView Setup with OpenGL and Metal on macOS](https://metashapes.com/blog/advanced-nsview-setup-opengl-metal-macos/)

### 2.2 Windows HighDPI Support

#### DPI Awareness Levels

Windows supports multiple DPI awareness modes:

1. **DPI Unaware** - Application bitmap-scaled by Windows (blurry)
2. **System DPI Aware** - Application uses system DPI, set at login
3. **Per-Monitor DPI Aware (V1)** - Application adapts to each monitor's DPI
4. **Per-Monitor DPI Aware V2** - Enhanced mode with automatic child window scaling (Windows 10 1703+)

**Recommended:** Per-Monitor DPI Aware V2

#### Declaration Methods

**Option 1: Application Manifest** (recommended)
```xml
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3">
  <asmv3:application>
    <asmv3:windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/PM</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2</dpiAwareness>
    </asmv3:windowsSettings>
  </asmv3:application>
</assembly>
```

**Option 2: Runtime API**
```c
#include <ShellScalingAPI.h>

// In WinMain, before creating window
SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
```

#### Implementation for RocketEditor

**Step 1: Create manifest file** `data/windows/RocketEditor.manifest`:
```xml
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
  <application xmlns="urn:schemas-microsoft-com:asm.v3">
    <windowsSettings>
      <dpiAware xmlns="http://schemas.microsoft.com/SMI/2005/WindowsSettings">true/pm</dpiAware>
      <dpiAwareness xmlns="http://schemas.microsoft.com/SMI/2016/WindowsSettings">PerMonitorV2</dpiAwareness>
    </windowsSettings>
  </application>
</assembly>
```

**Step 2: Embed manifest in** `data/windows/editor.rc`:
```rc
// Add at the end of the file
1 RT_MANIFEST "RocketEditor.manifest"
```

**What this fixes immediately (no code changes needed):**
- System menus render crisp (not bitmap-scaled)
- Window chrome (title bar, borders) render at native resolution
- Dialogs (Set Rows, Bias Selection) render at correct size
- Common dialogs (File Open/Save) render correctly

**What still needs code changes:**
- OpenGL content (viewport/font scaling as described in this document)
- Handling `WM_DPICHANGED` when window is dragged between monitors with different DPI

#### Key APIs

1. **Get Window DPI:**
   ```c
   UINT dpi = GetDpiForWindow(hwnd);  // Windows 10 1607+
   float scale = dpi / 96.0f;  // 96 is 100% scale
   ```

2. **Handle DPI Changes:**
   ```c
   case WM_DPICHANGED: {
       UINT newDpi = HIWORD(wParam);
       float scale = newDpi / 96.0f;

       // Suggested new window rectangle
       RECT* suggestedRect = (RECT*)lParam;

       SetWindowPos(hwnd, NULL,
                   suggestedRect->left,
                   suggestedRect->top,
                   suggestedRect->right - suggestedRect->left,
                   suggestedRect->bottom - suggestedRect->top,
                   SWP_NOZORDER | SWP_NOACTIVATE);

       Emgui_setDisplayScale(scale);
       Editor_update();
       return 0;
   }
   ```

3. **DPI-Aware Window Sizing:**
   ```c
   // Replace AdjustWindowRectEx with:
   AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
   ```

4. **DPI-Aware System Metrics:**
   ```c
   // Instead of GetSystemMetrics(SM_CXSCREEN)
   int width = GetSystemMetricsForDpi(SM_CXSCREEN, dpi);
   ```

#### Implementation Pattern

```c
// In createWindow() function (RocketWindow.c)
bool createWindow(const wchar_t* title, int width, int height) {
    // Set DPI awareness FIRST
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // ...existing window creation code...

    // After window creation, get initial DPI
    UINT dpi = GetDpiForWindow(s_window);
    float scale = dpi / 96.0f;
    Emgui_setDisplayScale(scale);

    // ...rest of initialization...
}

// In WndProc(), add WM_DPICHANGED handling
LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DPICHANGED: {
            UINT newDpi = HIWORD(wParam);
            float scale = newDpi / 96.0f;
            RECT* rect = (RECT*)lParam;

            SetWindowPos(window, NULL, rect->left, rect->top,
                        rect->right - rect->left, rect->bottom - rect->top,
                        SWP_NOZORDER | SWP_NOACTIVATE);

            Emgui_setDisplayScale(scale);
            Editor_setWindowSize(rect->right - rect->left,
                                rect->bottom - rect->top);
            Editor_update();
            return 0;
        }
        // ...existing cases...
    }
}
```

**Sources:**
- [High DPI Desktop Application Development on Windows - Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
- [Mixed-Mode DPI Scaling and DPI-aware APIs - Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-improvements-for-desktop-applications)
- [How to build high DPI aware native Windows desktop applications](https://mariusbancila.ro/blog/2021/05/19/how-to-build-high-dpi-aware-native-desktop-applications/)

### 2.3 Linux HighDPI Support

#### SDL2 HighDPI APIs

1. **Enable HighDPI Support:**
   ```c
   // Update window creation in src/linux/main.c:435
   window = SDL_CreateWindow("Rocket",
                             SDL_WINDOWPOS_CENTERED,
                             SDL_WINDOWPOS_CENTERED,
                             800, 600,
                             SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
   ```

2. **Query Display Scale (SDL 2.0.22+):**
   ```c
   float displayScale = 1.0f;

   #if SDL_VERSION_ATLEAST(2, 26, 0)
   // SDL 2.26+ has direct API
   displayScale = SDL_GetWindowDisplayScale(window);
   #elif SDL_VERSION_ATLEAST(2, 0, 22)
   // SDL 2.0.22+ can use DPI method
   float ddpi, hdpi, vdpi;
   int displayIndex = SDL_GetWindowDisplayIndex(window);
   if (SDL_GetDisplayDPI(displayIndex, &ddpi, &hdpi, &vdpi) == 0) {
       displayScale = ddpi / 96.0f;  // 96 is baseline DPI
   }
   #endif
   ```

3. **Get Framebuffer Size:**
   ```c
   int windowWidth, windowHeight;
   int framebufferWidth, framebufferHeight;

   SDL_GetWindowSize(window, &windowWidth, &windowHeight);
   SDL_GL_GetDrawableSize(window, &framebufferWidth, &framebufferHeight);

   float scaleX = (float)framebufferWidth / windowWidth;
   float scaleY = (float)framebufferHeight / windowHeight;
   ```

#### Platform-Specific Considerations

**X11:**
- Scale factor typically integer (1.0, 2.0)
- Controlled by `Xft.dpi` setting or desktop environment
- `SDL_GetDisplayDPI()` reads X11 DPI settings
- Generally more straightforward than Wayland

**Wayland:**
- Supports fractional scaling (1.25, 1.5, 1.75, etc.)
- Better native support in newer SDL2 versions
- May need `SDL_VIDEODRIVER=wayland` environment variable
- Compositor handles scaling automatically

**GTK File Dialogs:**
If using GTK for file dialogs (current implementation uses `#ifdef HAVE_GTK`):
```c
// GTK respects GDK_SCALE environment variable
// Can also query programmatically:
int gtkScale = gtk_widget_get_scale_factor(widget);
```

#### Implementation Pattern

```c
// In main() function (src/linux/main.c)
int main(int argc, char* argv[]) {
    // ...GTK init...

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Add SDL_WINDOW_ALLOW_HIGHDPI flag
    window = SDL_CreateWindow("Rocket",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              800, 600,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    // ...context creation...

    // Query initial scale
    float displayScale = 1.0f;
    int windowW, windowH;
    int drawableW, drawableH;

    SDL_GetWindowSize(window, &windowW, &windowH);
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);

    if (windowW > 0) {
        displayScale = (float)drawableW / windowW;
    }

    EMGFXBackend_create();
    Editor_create();
    Emgui_setDisplayScale(displayScale);
    EMGFXBackend_updateViewPort(drawableW, drawableH);
    Editor_setWindowSize(windowW, windowH);

    // ...
}

// Update resize handling
void resize(int w, int h) {
    int drawableW, drawableH;

    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowFullscreen(window, 0);
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);

    float scale = (float)drawableW / w;
    Emgui_setDisplayScale(scale);

    EMGFXBackend_updateViewPort(drawableW, drawableH);
    Editor_setWindowSize(w, h);
}
```

**Environment Variables to Consider:**
- `GDK_SCALE` - GTK/GNOME scale factor
- `QT_SCALE_FACTOR` - Qt application scaling (not directly relevant but users may set)
- `SDL_VIDEODRIVER=wayland` - Force Wayland backend

**Sources:**
- [SDL/docs/README-highdpi.md - GitHub](https://github.com/libsdl-org/SDL/blob/main/docs/README-highdpi.md)
- [HiDPI Scaling - steamtinkerlaunch Wiki](https://github.com/sonic2kk/steamtinkerlaunch/wiki/HiDPI-Scaling)
- [SDL2 Adds Wayland HiDPI Support - Phoronix](https://www.phoronix.com/news/SDL2-Wayland-HiDPI-Support)

---

## 3. OpenGL Rendering Pipeline Changes

### 3.1 Viewport vs Projection Separation

**Current Problem:**
The current implementation conflates window size (logical pixels) with framebuffer size (physical pixels).

**Solution:**
Separate viewport (physical pixels) from projection (logical coordinates).

```c
// In OpenGLBackend.c
static int s_framebufferWidth = 0;
static int s_framebufferHeight = 0;
static int s_windowWidth = 0;
static int s_windowHeight = 0;

void EMGFXBackend_updateViewPort(int framebufferWidth, int framebufferHeight) {
    s_framebufferWidth = framebufferWidth;
    s_framebufferHeight = framebufferHeight;

    // Viewport uses actual framebuffer size (physical pixels)
    glViewport(0, 0, framebufferWidth, framebufferHeight);
}

void EMGFXBackend_updateProjection(int windowWidth, int windowHeight) {
    s_windowWidth = windowWidth;
    s_windowHeight = windowHeight;

    // Projection uses logical window size
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, 0, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
```

### 3.2 Scissor Rectangle Scaling

The scissor test uses viewport coordinates (physical pixels), not logical coordinates:

```c
// Current code in OpenGLBackend.c:410-412
glScissor(layer->scissor.x, layer->scissor.y,
          layer->scissor.width, layer->scissor.height);
```

**Fix Required:**
```c
// Add scale factor tracking
static float s_displayScale = 1.0f;

void EMGFXBackend_setDisplayScale(float scale) {
    s_displayScale = scale;
}

// Update scissor call
glScissor((int)(layer->scissor.x * s_displayScale),
          (int)(layer->scissor.y * s_displayScale),
          (int)(layer->scissor.width * s_displayScale),
          (int)(layer->scissor.height * s_displayScale));
```

### 3.3 Line Width and Point Size

OpenGL line widths and point sizes are in physical pixels:

```c
// May need to scale line widths
glLineWidth(1.0f * s_displayScale);
```

However, for RocketEditor's current rendering (mostly quads and textured quads), this is less critical.

### 3.4 Texture Considerations

**Font Textures:**
- Currently rasterized at fixed size (512x512)
- Should be regenerated at higher resolution for HighDPI displays
- See section 4 for font scaling details

**Image Textures:**
- Loaded images should remain at their native resolution
- May want to provide 2x assets for HighDPI (e.g., `icon.png` and `icon@2x.png`)

**Sources:**
- [How can I set up a pixel-perfect camera using OpenGL and orthographic projection? - Game Development Stack Exchange](https://gamedev.stackexchange.com/questions/103334/how-can-i-set-up-a-pixel-perfect-camera-using-opengl-and-orthographic-projection)
- [OpenGL: Resizing Display and glOrtho/glViewport - Game Development Stack Exchange](https://gamedev.stackexchange.com/questions/49674/opengl-resizing-display-and-glortho-glviewport)

---

## 4. EMGUI Architecture Changes

### 4.1 Scale Factor Storage

Add global scale factor to EMGUI state:

```c
// In Emgui.c
static float g_displayScale = 1.0f;
static float g_userScale = 1.0f;  // User preference (100%, 125%, 150%, 200%)
static float g_effectiveScale = 1.0f;  // displayScale * userScale

void Emgui_setDisplayScale(float scale) {
    g_displayScale = scale;
    g_effectiveScale = g_displayScale * g_userScale;
    // Trigger font re-rasterization if needed
}

void Emgui_setUserScale(float scale) {
    g_userScale = scale;
    g_effectiveScale = g_displayScale * g_userScale;
    // Trigger font re-rasterization
}

float Emgui_getEffectiveScale(void) {
    return g_effectiveScale;
}
```

### 4.2 Mouse Coordinate Transformation

Mouse coordinates from platform layer are in physical pixels, but UI layout is in logical pixels:

```c
// In Emgui.c
void Emgui_setMousePos(int posX, int posY) {
    // Transform physical pixel coordinates to logical coordinates
    g_emguiGuiState.mouse.x = (int)(posX / g_effectiveScale);
    g_emguiGuiState.mouse.y = (int)(posY / g_effectiveScale);
}
```

Alternatively, have platform layer pass both window and framebuffer coordinates:

```c
void Emgui_setMousePosScaled(int windowX, int windowY, int framebufferX, int framebufferY) {
    // Use window coordinates for UI interaction (already in logical pixels)
    g_emguiGuiState.mouse.x = windowX;
    g_emguiGuiState.mouse.y = windowY;
}
```

**Recommendation:** Second approach is cleaner - have platform layer provide window coordinates directly.

### 4.3 Font Rendering at Different Scales

Current implementation uses `stbtt_BakeFontBitmap()` to rasterize fonts once at load time.

**Option 1: Re-rasterize on Scale Change**
```c
typedef struct LoadedFont {
    // ...existing fields...
    char* ttfFontPath;           // Store original TTF path
    void* ttfFontBuffer;         // Or keep font data in memory
    int ttfFontBufferSize;
    float originalFontHeight;    // Base height before scaling
} LoadedFont;

void Emgui_regenerateFontsForScale(float scale) {
    for (int i = 0; i < g_loadedFontsCount; i++) {
        LoadedFont* font = &g_loadedFonts[i];

        if (font->ttfFontBuffer) {
            // Calculate scaled height
            float scaledHeight = font->originalFontHeight * scale;

            // Re-rasterize
            uint8_t* fontBitmap = malloc(EM_FONTBITMAP_WIDTH * EM_FONTBITMAP_HEIGHT);
            stbtt_BakeFontBitmap(font->ttfFontBuffer, 0, scaledHeight,
                                fontBitmap, EM_FONTBITMAP_WIDTH, EM_FONTBITMAP_HEIGHT,
                                32, 96, font->cData);

            // Update texture
            EMGFXBackend_updateTexture(font->handle, EM_FONTBITMAP_WIDTH,
                                      EM_FONTBITMAP_HEIGHT, fontBitmap);
            free(fontBitmap);
        }
    }
}
```

**Option 2: Render Fonts at Higher Resolution, Scale Down**
Pre-render fonts at maximum expected scale (e.g., 2x or 4x) and scale down when rendering. This is simpler but uses more VRAM.

**Option 3: Signed Distance Field Fonts**
More complex but resolution-independent. Likely overkill for this project.

**Recommendation:** Option 1 (re-rasterize on demand) provides best quality-to-complexity ratio.

### 4.4 Widget Sizing API Changes

Currently, all EMGUI functions take pixel coordinates directly:

```c
void Emgui_fill(uint32_t color, int x, int y, int w, int h);
```

**Options:**

**Option A: Keep API in Logical Pixels (Recommended)**
- API remains unchanged from caller's perspective
- Internally, rendering commands scale coordinates before drawing
- Callers continue to use logical coordinate space (cleaner)

**Option B: Add Scaled Variants**
- Provide both logical and physical pixel APIs
- More complex, not recommended

**Recommendation:** Option A. Keep the EMGUI API in logical pixels, apply scaling internally during rendering.

### 4.5 Rendering Command Scaling

Scale all rendering commands before OpenGL calls:

```c
// In OpenGLBackend.c drawFill function
static void drawFill(struct DrawFillCommand* command) {
    glEnable(GL_BLEND);
    glBegin(GL_QUADS);

    while (command) {
        // Scale coordinates to physical pixels
        const float x = (float)(command->x * s_displayScale);
        const float y = (float)(command->y * s_displayScale);
        const float w = (float)(command->width * s_displayScale);
        const float h = (float)(command->height * s_displayScale);

        // ...rest of rendering...
    }
}
```

**Alternative:** Apply scale in projection matrix instead of per-command.

```c
// Simpler: scale projection matrix
glOrtho(0, windowWidth / scale, windowHeight / scale, 0, 0, 1);
```

**Recommendation:** Use projection matrix scaling (simpler, better performance).

---

## 5. Architecture Proposal

### 5.1 Scale Factor Flow

```
Platform Layer (macOS/Windows/Linux)
    ↓
    Query Display Scale Factor (1.0, 1.5, 2.0, etc.)
    ↓
    Call: Emgui_setDisplayScale(displayScale)
    ↓
EMGUI Core
    ↓
    Combine with User Scale Preference (stored in config)
    effectiveScale = displayScale * userScale
    ↓
    Trigger font re-rasterization if scale changed
    ↓
    Update projection matrix: glOrtho(0, width/scale, height/scale, 0, 0, 1)
    ↓
OpenGL Backend
    ↓
    Render at framebuffer resolution
    ↓
Display (pixel-perfect at native resolution)
```

### 5.2 Configuration Storage

**User scale preference storage:**

**macOS:** NSUserDefaults
```objective-c
[[NSUserDefaults standardUserDefaults] setFloat:1.5f forKey:@"UIScale"];
float userScale = [[NSUserDefaults standardUserDefaults] floatForKey:@"UIScale"];
```

**Windows:** Registry (already used for recent files)
```c
// In RocketWindow.c, add to saveRecentFileList/getRecentFiles pattern
setRegString(s_regConfigKey, L"UIScale", L"1.5", 3);
float userScale = 1.0f;
wchar_t buffer[32];
if (getRegString(s_regConfigKey, buffer, L"UIScale")) {
    userScale = (float)_wtof(buffer);
}
```

**Linux:** Config file in `~/.config/rocket/config`
```c
// New config.c module
void Config_save(const char* key, float value);
float Config_load(const char* key, float defaultValue);
```

### 5.3 Menu/UI for Scale Selection

Add menu items under View menu:

```c
// In Menu.h
enum {
    // ...existing items...
    EDITOR_MENU_UI_SCALE_100,
    EDITOR_MENU_UI_SCALE_125,
    EDITOR_MENU_UI_SCALE_150,
    EDITOR_MENU_UI_SCALE_175,
    EDITOR_MENU_UI_SCALE_200,
};

// In menu definitions
{L"100%", EMGUI_KEY_CTRL, '1', 0, EDITOR_MENU_UI_SCALE_100},
{L"125%", EMGUI_KEY_CTRL, '2', 0, EDITOR_MENU_UI_SCALE_125},
{L"150%", EMGUI_KEY_CTRL, '3', 0, EDITOR_MENU_UI_SCALE_150},
{L"175%", EMGUI_KEY_CTRL, '4', 0, EDITOR_MENU_UI_SCALE_175},
{L"200%", EMGUI_KEY_CTRL, '5', 0, EDITOR_MENU_UI_SCALE_200},
```

Handler in `Editor_menuEvent()`:
```c
case EDITOR_MENU_UI_SCALE_100: Emgui_setUserScale(1.00f); break;
case EDITOR_MENU_UI_SCALE_125: Emgui_setUserScale(1.25f); break;
case EDITOR_MENU_UI_SCALE_150: Emgui_setUserScale(1.50f); break;
case EDITOR_MENU_UI_SCALE_175: Emgui_setUserScale(1.75f); break;
case EDITOR_MENU_UI_SCALE_200: Emgui_setUserScale(2.00f); break;
```

---

## 6. Code Change Inventory

### 6.1 EMGUI Core (`emgui/`)

**emgui/include/emgui/Emgui.h**
- [ ] Add: `void Emgui_setDisplayScale(float scale)`
- [ ] Add: `void Emgui_setUserScale(float scale)`
- [ ] Add: `float Emgui_getEffectiveScale(void)`

**emgui/src/Emgui.c**
- [ ] Add: `static float g_displayScale = 1.0f`
- [ ] Add: `static float g_userScale = 1.0f`
- [ ] Add: `static float g_effectiveScale = 1.0f`
- [ ] Modify: `Emgui_setMousePos()` - apply scale transform
- [ ] Modify: `Emgui_loadFontTTF()` - store original font data/path
- [ ] Add: `Emgui_regenerateFontsForScale(float scale)`
- [ ] Modify: `createDefaultFont()` - make scale-aware
- [ ] Add: Implementation of new scale functions

**emgui/include/emgui/GFXBackend.h**
- [ ] Modify: `EMGFXBackend_updateViewPort(int framebufferWidth, int framebufferHeight)` signature
- [ ] Add: `void EMGFXBackend_updateProjection(int windowWidth, int windowHeight)`
- [ ] Add: `void EMGFXBackend_setDisplayScale(float scale)`

**emgui/src/GFXBackends/OpenGLBackend.c**
- [ ] Add: `static float s_displayScale = 1.0f`
- [ ] Add: `static int s_windowWidth, s_windowHeight`
- [ ] Add: `static int s_framebufferWidth, s_framebufferHeight`
- [ ] Modify: `EMGFXBackend_updateViewPort()` - use framebuffer dimensions
- [ ] Add: `EMGFXBackend_updateProjection()` - set up scaled ortho projection
- [ ] Add: `EMGFXBackend_setDisplayScale()` implementation
- [ ] Modify: `drawFill()` - apply scissor scaling
- [ ] Modify: All OpenGL rendering to account for scale

**emgui/src/Emgui_internal.h**
- [ ] Add: `extern float g_displayScale, g_userScale, g_effectiveScale`
- [ ] Modify: `LoadedFont` structure to store TTF data

### 6.2 Platform Layer

**src/macosx/RocketView.m**
- [ ] Modify: Line 55 - Change `setWantsBestResolutionOpenGLSurface:NO` to `YES`
- [ ] Add: `viewDidChangeBackingProperties` method
- [ ] Modify: `drawRect:` - use `convertRectToBacking:` for framebuffer size
- [ ] Modify: `initWithFrame:` - query initial scale factor
- [ ] Modify: Mouse event handlers - use window coordinates, not backing coordinates

**src/windows/RocketWindow.c**
- [ ] Add: `SetProcessDpiAwarenessContext()` call at startup
- [ ] Add: `WM_DPICHANGED` case in `WndProc()`
- [ ] Modify: `createWindow()` - query initial DPI with `GetDpiForWindow()`
- [ ] Modify: `WM_SIZE` handler - distinguish window size from client area
- [ ] Add: Include `ShellScalingAPI.h` header
- [ ] Add: Configuration save/load for user scale preference

**src/linux/main.c**
- [ ] Modify: Line 435 - Add `SDL_WINDOW_ALLOW_HIGHDPI` flag to `SDL_CreateWindow()`
- [ ] Modify: `main()` - query scale using `SDL_GL_GetDrawableSize()`
- [ ] Modify: `resize()` - query new scale factor on resize
- [ ] Add: Configuration save/load module for user scale preference
- [ ] Modify: Mouse coordinates - ensure using window coords not drawable coords

### 6.3 Application Layer

**src/Editor.c**
- [ ] Modify: `Editor_create()` - load user scale preference from config
- [ ] Modify: `Editor_setWindowSize()` - update to handle separate window/framebuffer sizes
- [ ] Modify: `Editor_menuEvent()` - add handlers for UI scale menu items
- [ ] Add: `Editor_setUserScale(float scale)` function
- [ ] Modify: `Editor_scroll()` - ensure scroll deltas are scale-independent

**src/TrackView.c**
- [ ] Evaluate: All hardcoded pixel constants (lines 27-32)
- [ ] Modify: `font_size`, `track_size_folded`, `min_track_size` - make scale-aware or remove
- [ ] Review: All rendering functions for scale independence
- [ ] Note: Most coordinates are already logical, main concern is font sizes

**src/Menu.h / src/Menu.c**
- [ ] Add: Menu item IDs for UI scale options
- [ ] Add: Menu descriptors for View → UI Scale submenu

### 6.4 New Files

**src/Config.c / src/Config.h** (Linux)
- [ ] Create: Configuration file I/O for Linux
- [ ] Implement: `Config_save(const char* key, float value)`
- [ ] Implement: `Config_load(const char* key, float defaultValue)`

### 6.5 Build System

**data/windows/RocketEditor.manifest** (NEW FILE)
- [ ] Create: DPI awareness manifest with PerMonitorV2 declaration

**data/windows/editor.rc**
- [ ] Add: `1 RT_MANIFEST "RocketEditor.manifest"` to embed manifest

**CMakeLists.txt**
- [ ] Ensure: SDL2 version 2.0.22+ for Linux HighDPI support
- [ ] Verify: OpenGL version compatibility

---

## 7. Complexity Assessment

### 7.1 Complexity Overview

| Component | Complexity | Risk Level |
|-----------|-----------|------------|
| **EMGUI Scale Factor Management** | Medium | Low |
| **OpenGL Backend Modifications** | Medium | Medium |
| **Font Re-rasterization System** | High | Medium |
| **macOS Retina Support** | Low | Low |
| **Windows DPI Awareness** | Medium | Medium |
| **Linux SDL2 HighDPI** | Low-Medium | Low |
| **Mouse Coordinate Transformation** | Low | Low |
| **Configuration System** | Low | Low |
| **UI Scale Menu/Preferences** | Low | Low |
| **Testing & Bug Fixes** | - | High |

### 7.2 Risk Factors

**High Risk:**
- Font rendering quality at various scales (aliasing, metrics)
- Mouse coordinate precision (off-by-one errors common)
- Platform-specific quirks (especially Linux with X11/Wayland)
- Regression testing across all platforms and configurations

**Medium Risk:**
- OpenGL viewport/projection coordinate system bugs
- Performance impact of font re-rasterization
- Configuration migration for existing users

**Low Risk:**
- Basic DPI detection on each platform (well-documented APIs)
- Menu/UI additions

### 7.3 Dependencies

- **SDL2 Version:** Minimum 2.0.22 recommended for Linux (better DPI support)
- **Windows SDK:** Windows 10 SDK version 1607+ for `GetDpiForWindow()`
- **macOS Version:** Retina APIs available since 10.7, stable since 10.9
- **stb_truetype:** Already integrated, no changes needed

---

## 8. Risk Analysis

### 8.1 Rendering Artifacts

**Risk:** Text or UI elements appear blurry, misaligned, or clipped at non-integer scales

**Mitigation:**
- Use floating-point coordinates internally, round only at final render
- Test thoroughly at common fractional scales (125%, 150%, 175%)
- Implement pixel-snapping for crisp lines and borders

### 8.2 Performance Degradation

**Risk:** Font re-rasterization causes stuttering when changing scales or moving between displays

**Mitigation:**
- Cache font textures at multiple scales
- Make re-rasterization asynchronous if needed
- Profile to ensure viewport/projection changes don't impact frame rate

### 8.3 Backward Compatibility

**Risk:** Older systems or configurations may not support HighDPI APIs

**Mitigation:**
- Graceful fallback to 1.0 scale on unsupported platforms
- Version checks for Windows/SDL2 APIs
- Test on both new and legacy systems

### 8.4 Platform-Specific Issues

**macOS Specific:**
- Users may have "Open in Low Resolution" enabled for the app
- Moving between Retina and non-Retina displays must be handled seamlessly

**Windows Specific:**
- Mixing DPI awareness levels in the process can cause issues
- Per-monitor v2 not available on Windows 7 (fallback needed)
- Users may override app DPI settings in compatibility properties

**Linux Specific:**
- X11 vs Wayland behavior differs significantly
- Desktop environment scale settings vary (GNOME, KDE, XFCE)
- Environment variables may conflict with detected settings
- SDL2 version fragmentation across distributions

### 8.5 User Experience

**Risk:** Sudden UI scale changes are jarring; users may not understand new scale options

**Mitigation:**
- Make scale changes smooth (perhaps require restart for major changes)
- Clear UI labels for scale options
- Provide "Reset to Default" option
- Documentation explaining HighDPI support

---

## 9. Recommendations

### 9.1 Implementation Approach

**Phase 1: Foundation**
1. Add scale factor infrastructure to EMGUI (storage, getters/setters)
2. Update OpenGL backend to separate viewport and projection
3. Modify mouse coordinate handling

**Phase 2: Platform Integration**
4. Implement macOS Retina support
5. Implement Windows DPI awareness
6. Implement Linux SDL2 HighDPI

**Phase 3: Font System**
7. Add font data retention to LoadedFont structure
8. Implement font re-rasterization on scale change
9. Test font quality at various scales

**Phase 4: User Controls**
10. Add configuration persistence
11. Implement UI scale menu
12. Testing and refinement across all platforms

### 9.2 Testing Strategy

**Test Matrix:**

| Platform | Display Type | Scale Factors | Window Ops |
|----------|-------------|---------------|------------|
| macOS | Retina | 1.0, 2.0 | Drag between displays |
| macOS | Non-Retina | 1.0 | - |
| Windows 10 | 4K | 1.0, 1.25, 1.5, 2.0 | Drag between displays |
| Windows 10 | 1080p | 1.0 | System DPI change |
| Linux (X11) | 1080p | 1.0, 2.0 | - |
| Linux (Wayland) | 4K | 1.0, 1.25, 1.5, 2.0 | - |

**Test Cases:**
- [ ] UI renders crisply at each scale factor
- [ ] Mouse clicks register at correct coordinates
- [ ] Text remains readable and properly sized
- [ ] Window resizing works smoothly
- [ ] Moving window between displays updates scale automatically
- [ ] User scale preference persists across sessions
- [ ] Changing user scale updates UI immediately
- [ ] Scrolling and zooming work correctly at all scales
- [ ] File dialogs appear at correct scale
- [ ] No crashes on scale changes

### 9.3 Quick Wins

If full implementation is too complex, prioritize these high-impact changes:

1. **macOS:** Enable Retina support (`setWantsBestResolutionOpenGLSurface:YES`)
   - Immediate improvement for Mac users

2. **Windows:** Add DPI awareness manifest
   - Create `data/windows/RocketEditor.manifest` with PerMonitorV2 declaration
   - Add `1 RT_MANIFEST "RocketEditor.manifest"` to `data/windows/editor.rc`
   - Fixes blurry system menus and window chrome immediately (no code changes)

3. **Linux:** Add `SDL_WINDOW_ALLOW_HIGHDPI` flag
   - Enables HighDPI on Wayland

These three changes alone would make the application HighDPI-aware on all platforms, even without user-configurable scaling.

### 9.4 Future Enhancements

**Out of Scope (but worth considering later):**
- Scalable vector graphics for icons/images
- Per-monitor user scale preferences
- Fractional scaling on Windows (requires more complex implementation)
- Dynamic font size adjustment based on window size
- Accessibility features (high contrast, larger text modes)

---

## 10. References

### Documentation

- [Apple - High Resolution Guidelines for OS X](https://developer.apple.com/library/archive/documentation/GraphicsAnimation/Conceptual/HighResolutionOSX/Introduction/Introduction.html)
- [Microsoft - High DPI Desktop Application Development on Windows](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
- [SDL2 - High DPI Support README](https://github.com/libsdl-org/SDL/blob/main/docs/README-highdpi.md)

### API References

- [NSView backingScaleFactor - Apple Developer](https://developer.apple.com/documentation/appkit/nsview/1483461-backingscalefactor)
- [GetDpiForWindow - Microsoft Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getdpiforwindow)
- [SDL_GL_GetDrawableSize - SDL2 Wiki](https://wiki.libsdl.org/SDL2/SDL_GL_GetDrawableSize)

### Community Resources

- [HiDPI / backingScaleFactor on MacOS - Stack Overflow](https://stackoverflow.com/questions/54429178/hidpi-backingscalefactor-on-macos-how-to-get-actual-value)
- [Writing Win32 apps like it's 2020: A DPI-aware resizable wizard - ENLYZE](https://building.enlyze.com/posts/writing-win32-apps-like-its-2020-part-3/)
- [Correct out-of-the-box HiDPI support in SDL - GitHub Issue](https://github.com/mosra/magnum/issues/243)

---

## Appendix A: Example Code Snippets

### A.1 Complete macOS Implementation

```objective-c
// In RocketView.m

@interface RocketView : NSOpenGLView {
    NSOpenGLContext* oglContext;
}
@end

@implementation RocketView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    // Enable Retina support
    [self setWantsBestResolutionOpenGLSurface:YES];

    NSOpenGLPixelFormatAttribute attributes[] = {
        NSOpenGLPFADoubleBuffer,
        0
    };

    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    oglContext = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
    [format release];
    [oglContext makeCurrentContext];

    oglContext.view = self;

    EMGFXBackend_create();
    Editor_create();

    // Set initial scale
    CGFloat scale = [[self window] backingScaleFactor];
    Emgui_setDisplayScale(scale);

    // Load user preference
    float userScale = [[NSUserDefaults standardUserDefaults] floatForKey:@"UIScale"];
    if (userScale == 0.0f) userScale = 1.0f;
    Emgui_setUserScale(userScale);

    Editor_update();

    const float framerate = 60;
    const float frequency = 1.0f/framerate;
    [NSTimer scheduledTimerWithTimeInterval:frequency
                                     target:self selector:@selector(updateEditor)
                                   userInfo:nil repeats:YES];

    return self;
}

- (void)viewDidChangeBackingProperties {
    [super viewDidChangeBackingProperties];

    CGFloat newScale = [[self window] backingScaleFactor];
    NSRect backingBounds = [self convertRectToBacking:[self bounds]];

    Emgui_setDisplayScale(newScale);
    EMGFXBackend_updateViewPort((int)backingBounds.size.width,
                                (int)backingBounds.size.height);

    NSRect bounds = [self bounds];
    EMGFXBackend_updateProjection((int)bounds.size.width,
                                  (int)bounds.size.height);
    Editor_setWindowSize((int)bounds.size.width, (int)bounds.size.height);
    Editor_update();
}

- (void)drawRect:(NSRect)frameRect {
    [oglContext update];

    NSRect backingBounds = [self convertRectToBacking:frameRect];
    CGFloat scale = [[self window] backingScaleFactor];

    Emgui_setDisplayScale(scale);
    EMGFXBackend_updateViewPort((int)backingBounds.size.width,
                                (int)backingBounds.size.height);
    EMGFXBackend_updateProjection((int)frameRect.size.width,
                                  (int)frameRect.size.height);
    Editor_setWindowSize((int)frameRect.size.width, (int)frameRect.size.height);
    Editor_update();
}

- (void)mouseMoved:(NSEvent *)event {
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    // Use window coordinates (already in logical pixels)
    Emgui_setMousePos((int)location.x, (int)([self bounds].size.height - location.y));
    Editor_update();
}

// Save scale preference
- (void)dealloc {
    float userScale = Emgui_getUserScale();
    [[NSUserDefaults standardUserDefaults] setFloat:userScale forKey:@"UIScale"];
    [super dealloc];
}

@end
```

### A.2 Complete Windows Implementation

```c
// In RocketWindow.c

#include <ShellScalingAPI.h>
#pragma comment(lib, "Shcore.lib")

bool createWindow(const wchar_t* title, int width, int height) {
    // Set DPI awareness context FIRST
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    // ...existing pixel format and window class setup...

    if (!(s_window = CreateWindowEx(exStyle, L"RocketEditor", title,
                                    style | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                                    0, 0, rect.right - rect.left,
                                    rect.bottom - rect.top,
                                    NULL, NULL, s_instance, NULL))) {
        closeWindow();
        return FALSE;
    }

    // ...existing DC and context creation...

    // Get initial DPI
    UINT dpi = GetDpiForWindow(s_window);
    float displayScale = dpi / 96.0f;
    Emgui_setDisplayScale(displayScale);

    // Load user scale from registry
    wchar_t scaleStr[32] = {0};
    float userScale = 1.0f;
    if (getRegString(s_regConfigKey, scaleStr, L"UIScale")) {
        userScale = (float)_wtof(scaleStr);
    }
    if (userScale < 0.5f || userScale > 4.0f) userScale = 1.0f;
    Emgui_setUserScale(userScale);

    Editor_create();
    // ...rest of initialization...

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DPICHANGED: {
            UINT newDpi = HIWORD(wParam);
            float displayScale = newDpi / 96.0f;
            RECT* rect = (RECT*)lParam;

            // Update window position and size as suggested by OS
            SetWindowPos(window, NULL, rect->left, rect->top,
                        rect->right - rect->left, rect->bottom - rect->top,
                        SWP_NOZORDER | SWP_NOACTIVATE);

            Emgui_setDisplayScale(displayScale);

            RECT clientRect;
            GetClientRect(window, &clientRect);
            EMGFXBackend_updateViewPort(clientRect.right, clientRect.bottom);
            EMGFXBackend_updateProjection(clientRect.right, clientRect.bottom);
            Editor_setWindowSize(clientRect.right, clientRect.bottom);
            Editor_update();
            return 0;
        }

        case WM_SIZE: {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            EMGFXBackend_updateViewPort(width, height);
            EMGFXBackend_updateProjection(width, height);
            Editor_setWindowSize(width, height);
            Editor_update();
            return 0;
        }

        case WM_CLOSE: {
            // Save user scale preference
            wchar_t scaleStr[32];
            float userScale = Emgui_getUserScale();
            swprintf_s(scaleStr, sizeof_array(scaleStr), L"%.2f", userScale);
            setRegString(s_regConfigKey, L"UIScale", scaleStr, (int)wcslen(scaleStr));

            // ...existing close logic...
        }

        // ...other cases...
    }
    return DefWindowProc(window, message, wParam, lParam);
}
```

### A.3 Complete Linux/SDL2 Implementation

```c
// In src/linux/main.c

int main(int argc, char* argv[]) {
#ifdef HAVE_GTK
    gtk_init(&argc, &argv);
#endif

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // Create window with HighDPI support
    window = SDL_CreateWindow("Rocket",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              800, 600,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    if (!window) {
        fprintf(stderr, "SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_VERSION(&window_info.version);
    SDL_GetWindowWMInfo(window, &window_info);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext(): %s\n", SDL_GetError());
        return 1;
    }

    loadRecents();

    EMGFXBackend_create();
    Editor_create();

    // Query display scale
    int windowW, windowH;
    int drawableW, drawableH;
    SDL_GetWindowSize(window, &windowW, &windowH);
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);

    float displayScale = (windowW > 0) ? ((float)drawableW / windowW) : 1.0f;
    Emgui_setDisplayScale(displayScale);

    // Load user scale from config
    float userScale = Config_load("UIScale", 1.0f);
    if (userScale < 0.5f || userScale > 4.0f) userScale = 1.0f;
    Emgui_setUserScale(userScale);

    EMGFXBackend_updateViewPort(drawableW, drawableH);
    EMGFXBackend_updateProjection(windowW, windowH);
    Editor_setWindowSize(windowW, windowH);

    run(window);

    // Save user scale preference
    Config_save("UIScale", Emgui_getUserScale());
    saveRecents();

    SDL_Quit();
    return 0;
}

void resize(int w, int h) {
    int drawableW, drawableH;

    SDL_SetWindowSize(window, w, h);
    SDL_SetWindowFullscreen(window, 0);
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);

    float displayScale = (w > 0) ? ((float)drawableW / w) : 1.0f;
    Emgui_setDisplayScale(displayScale);

    EMGFXBackend_updateViewPort(drawableW, drawableH);
    EMGFXBackend_updateProjection(w, h);
    Editor_setWindowSize(w, h);
}
```

---

## Appendix B: Configuration File Format (Linux)

### B.1 Config File Location

`~/.config/rocket/config`

### B.2 Format

Simple key=value format:

```ini
UIScale=1.5
WindowWidth=1200
WindowHeight=800
```

### B.3 Implementation

```c
// Config.h
#ifndef CONFIG_H
#define CONFIG_H

void Config_init(void);
void Config_save(const char* key, float value);
float Config_load(const char* key, float defaultValue);
void Config_shutdown(void);

#endif

// Config.c
#include "Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static char s_configPath[512] = {0};

void Config_init(void) {
    const char* home = getenv("HOME");
    if (!home) return;

    snprintf(s_configPath, sizeof(s_configPath), "%s/.config/rocket", home);
    mkdir(s_configPath, 0755);  // Create if doesn't exist

    snprintf(s_configPath, sizeof(s_configPath), "%s/.config/rocket/config", home);
}

void Config_save(const char* key, float value) {
    if (s_configPath[0] == 0) Config_init();

    char tempPath[512];
    snprintf(tempPath, sizeof(tempPath), "%s.tmp", s_configPath);

    FILE* in = fopen(s_configPath, "r");
    FILE* out = fopen(tempPath, "w");
    if (!out) return;

    bool found = false;
    char line[256];

    if (in) {
        while (fgets(line, sizeof(line), in)) {
            if (strncmp(line, key, strlen(key)) == 0 && line[strlen(key)] == '=') {
                fprintf(out, "%s=%.2f\n", key, value);
                found = true;
            } else {
                fputs(line, out);
            }
        }
        fclose(in);
    }

    if (!found) {
        fprintf(out, "%s=%.2f\n", key, value);
    }

    fclose(out);
    rename(tempPath, s_configPath);
}

float Config_load(const char* key, float defaultValue) {
    if (s_configPath[0] == 0) Config_init();

    FILE* f = fopen(s_configPath, "r");
    if (!f) return defaultValue;

    char line[256];
    float result = defaultValue;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, key, strlen(key)) == 0 && line[strlen(key)] == '=') {
            result = atof(line + strlen(key) + 1);
            break;
        }
    }

    fclose(f);
    return result;
}

void Config_shutdown(void) {
    // Nothing needed
}
```

---

**End of Research Report**
