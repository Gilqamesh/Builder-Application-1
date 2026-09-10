# Raylib in Builder

This module publishes Raylib's window, input, graphics and audio API together with
its math and low-level rendering headers. [builder.cpp](builder.cpp) pins upstream
**5.5**, downloading `https://codeload.github.com/raysan5/raylib/tar.gz/refs/tags/5.5`
with SHA-256 `aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e`.
The upstream source is acquired during the source phase; it is not checked into
this module. The builder configures a shared library with upstream examples and
games disabled.

The interface phase publishes:

| Include | Responsibility |
| --- | --- |
| `<m03gagbht17w4tser1fescqxye_raylib/raylib.h>` | Window, input, drawing, resource and audio API. |
| `<m03gagbht17w4tser1fescqxye_raylib/raymath.h>` | Vector, matrix and quaternion operations. |
| `<m03gagbht17w4tser1fescqxye_raylib/rlgl.h>` | Low-level rendering API used by integrations such as rlImGui. |

Use the published library with these headers; the headers do not replace linking
Raylib. The pinned [raylib.h](https://github.com/raysan5/raylib/blob/5.5/src/raylib.h),
[raymath.h](https://github.com/raysan5/raylib/blob/5.5/src/raymath.h) and
[rlgl.h](https://github.com/raysan5/raylib/blob/5.5/src/rlgl.h) own the upstream API
contracts.

This standalone caller owns the window and its graphics context:

```cpp
#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>

int main() {
    InitWindow(800, 450, "Raylib example");
    if (!IsWindowReady()) {
        return 1;
    }
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Close the window or press Escape", 20, 20, 20, DARKGRAY);
        EndDrawing();
    }
    CloseWindow();
}
```

Run desktop window setup and drawing on the main thread. Execution requires a
working graphical session and the display/OpenGL support selected by the Raylib
build; compiling this example does not verify those runtime requirements.
`EndDrawing()` presents the frame and, in the default configuration, handles
timing and input polling. `WindowShouldClose()` includes the default Escape-key
exit request.

Load GPU resources after window creation and unload caller-owned textures,
render textures and shaders before `CloseWindow()`. Window creation does not
initialize the audio device. Callers using audio must pair `InitAudioDevice()`
with `CloseAudioDevice()` and manage their audio resources separately.
For ImGui drawing, use the [rlImGui integration](../m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/INTEGRATION.md)
inside this window lifecycle.
