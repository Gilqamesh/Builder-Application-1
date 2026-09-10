# Dear ImGui in Builder

This module supplies Dear ImGui's widget, layout and draw-data APIs plus its
`std::string` input helpers. The vendored [imgui.h](imgui.h) identifies this copy as
`1.92.4 WIP` (`IMGUI_VERSION_NUM == 19236`); its comments own the upstream API
contract. Use headers and compiled sources from the same copy and configuration.

[builder.cpp](builder.cpp) publishes these module-qualified includes:

```text
m03ge9ij4gm20btwzykayjx3pl_imgui/imconfig.h
m03ge9ij4gm20btwzykayjx3pl_imgui/imgui.h
m03ge9ij4gm20btwzykayjx3pl_imgui/imgui_internal.h
m03ge9ij4gm20btwzykayjx3pl_imgui/imstb_rectpack.h
m03ge9ij4gm20btwzykayjx3pl_imgui/imstb_textedit.h
m03ge9ij4gm20btwzykayjx3pl_imgui/imstb_truetype.h
m03ge9ij4gm20btwzykayjx3pl_imgui/misc/cpp/imgui_stdlib.h
```

Ordinary callers use `imgui.h` and, for `std::string` inputs,
`misc/cpp/imgui_stdlib.h`. The internal and embedded stb headers support the
vendored implementation; their publication does not make them stable application
interfaces. The interface phase also installs unprefixed copies for upstream
includes; application includes should retain the complete module prefix.

The library builds exactly `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`,
`imgui_widgets.cpp` and `misc/cpp/imgui_stdlib.cpp`. It does **not** build
`imgui_demo.cpp` or any `backends/` source, and does not publish backend headers.
Consequently, the header's recommendation to call `ImGui::ShowDemoWindow()` needs
the matching `imgui_demo.cpp` to be compiled and linked separately. Other functions
defined there, including `ShowStyleEditor()` and `ShowUserGuide()`, have the same
limitation. No demo or backend is added by including its declaration.

A core-only caller can create and destroy a context without opening a window:

```cpp
#include <m03ge9ij4gm20btwzykayjx3pl_imgui/imgui.h>

int main() {
    IMGUI_CHECKVERSION();
    ImGuiContext* context = ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGui::DestroyContext(context);
}
```

This example does not start frames or render. A graphical caller must also supply
platform input, frame timing, display dimensions and a renderer, including font
texture handling. For the matching Raylib integration, follow the complete
[rlImGui lifecycle example](../../ws2/m03ge9ij4hzaaehqgc59cj8f9q_rl_imgui/INTEGRATION.md).
That high-level API creates and destroys its own ImGui context, so do not combine
its setup with the core-only context example above.

Keep a context current while calling its ImGui APIs. References from `GetIO()` and
`GetStyle()` borrow that context and expire when it is destroyed. This build uses
ImGui's global current-context state and is not thread-safe. By default, ImGui
loads and saves `imgui.ini` relative to the process working directory; setting
`IniFilename` to `nullptr` before the first frame disables that automatic I/O.
