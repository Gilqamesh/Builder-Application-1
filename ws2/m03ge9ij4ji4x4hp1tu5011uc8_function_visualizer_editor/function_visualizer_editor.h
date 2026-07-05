#ifndef M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H
# define M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H

# include <functional>
# include <string>

namespace m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor {

class function_visualizer_editor_t {
public:
    function_visualizer_editor_t();

    void init();
    void deinit();

    void create_text_editor(std::function<void(std::string)> on_text_complete);
    void draw();

    bool open();
    bool is_captured_mouse();
    bool is_captured_keyboard();

private:
    char m_buffer[128];
    bool m_open;
    bool m_just_opened;
    std::function<void(std::string)> m_on_complete;
};

} // namespace m03ge9ij4ji4x4hp1tu5011uc8_function_visualizer_editor

#endif // M03GE9IJ4JI4X4HP1TU5011UC8_FUNCTION_VISUALIZER_EDITOR_FUNCTION_VISUALIZER_EDITOR_H
