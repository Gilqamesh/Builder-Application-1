# rlImGui in Builder

This module feeds Raylib input into Dear ImGui and renders ImGui draw data into
the current Raylib render target. [rlImGui.h](rlImGui.h) exposes the high-level
lifecycle and image helpers; [imgui_impl_raylib.h](imgui_impl_raylib.h) exposes the
backend for callers that own an ImGui context. [builder.cpp](builder.cpp) builds
`rlImGui.cpp` and publishes those headers, `rlImGuiColors.h`,
`extras/IconsFontAwesome6.h` and `extras/FA6FreeSolidFontData.h`, all beneath the
`m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/` include prefix.

Use the workspace's [Raylib](../m03gagbht17w4tser1fescqxye_raylib/INTEGRATION.md) and
[Dear ImGui](../../ws1/m03ge9ij4gm20btwzykayjx3pl_imgui/INTEGRATION.md) modules with
this backend. Its implementation uses ImGui's texture-update API from the bundled
1.92 series. All window/context operations belong on the main thread of the
graphical application; the backend shares global input and context state and
does not provide concurrent or independent multi-window sessions.

For a single application, the high-level API owns its ImGui context:

```cpp
#include <m03ge9ij4gm20btwzykayjx3pl_imgui/imgui.h>
#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
#include <m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/rlImGui.h>

int main() {
    InitWindow(800, 450, "rlImGui example");
    if (!IsWindowReady()) {
        return 1;
    }
    IMGUI_CHECKVERSION();
    rlImGuiSetup(true);
    ImGui::GetIO().IniFilename = nullptr;
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        rlImGuiBegin();
        if (ImGui::Begin("Tools")) {
            ImGui::TextUnformatted("Ready");
        }
        ImGui::End();
        rlImGuiEnd();
        EndDrawing();
    }
    rlImGuiShutdown();
    CloseWindow();
}
```

`rlImGuiSetup(true)` selects the dark theme; pass `false` for the light theme.
Setup creates the backend's global ImGui context and adds the default font and,
unless built with `NO_FONT_AWESOME`, Font Awesome. Do not separately create or
destroy that context. Pair each `rlImGuiBegin()` with `rlImGuiEnd()`;
these perform event processing, `ImGui::NewFrame()`, `ImGui::Render()` and backend
submission. Keep ImGui widgets between them. Always pair `ImGui::Begin()` with
`ImGui::End()`, even when the window is collapsed. Shutdown releases backend
textures and destroys the owned context while the Raylib window is still alive.

For custom font/configuration setup, replace `rlImGuiSetup()` with
`rlImGuiBeginInitImGui()`, your ImGui configuration, then
`rlImGuiEndInitImGui()`. This still uses the high-level API's owned context.
Use `rlImGuiBegin()` for Raylib frame timing. If supplying `rlImGuiBeginDelta()`,
pass a positive delta in seconds: despite the header's negative-value fallback
comment, the current implementation clamps nonpositive values to `0.001f`
instead of reading Raylib's frame time.

Choose the low-level API when the caller must own and keep its context current:

```cpp
#include <m03ge9ij4gm20btwzykayjx3pl_imgui/imgui.h>
#include <m03gagbht17w4tser1fescqxye_raylib/raylib.h>
#include <m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/imgui_impl_raylib.h>

int main() {
    InitWindow(800, 450, "ImGui backend example");
    if (!IsWindowReady()) {
        return 1;
    }
    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    if (!ImGui_ImplRaylib_Init()) {
        ImGui::DestroyContext(context);
        CloseWindow();
        return 1;
    }
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        ImGui_ImplRaylib_NewFrame();
        ImGui_ImplRaylib_ProcessEvents();
        ImGui::NewFrame();
        if (ImGui::Begin("Tools")) {
            ImGui::TextUnformatted("Caller-owned context");
        }
        ImGui::End();
        ImGui::Render();
        ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
        EndDrawing();
    }
    ImGui_ImplRaylib_Shutdown();
    ImGui::DestroyContext(context);
    CloseWindow();
}
```

Low-level initialization configures the current context and currently returns
`true` without creating a context or checking window readiness. It does not
perform the high-level theme/Font Awesome setup. Low-level shutdown frees backend
resources but leaves context destruction to the caller. Do not mix high-level
frame calls into this sequence: they select the backend's global context.
The examples disable automatic `imgui.ini` I/O; the default behavior is described
by the ImGui module.

Image helpers borrow Raylib texture descriptors during the call; the caller keeps
the underlying GPU textures alive through frame submission and remains responsible
for unloading them. Null image pointers draw nothing (image buttons return
`false`). Sizes and source rectangles use pixels; render-texture helpers flip the
Y axis. For direct ImGui image calls, the current implementation uses
`ImTextureID(texture.id)`, **not** a pointer to the Raylib `Texture` struct as the
vendored header's introductory image comment suggests.
