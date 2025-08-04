// #pragma once

// #include "window.h"
// #include <libplacebo/opengl.h>
// #define IMPL_NAME "GLFW (opengl)"
// #define IMPL_TAG "glfw-gl"

// class GLFWwindow;

// struct window_pos
// {
//     int x;
//     int y;
//     int w;
//     int h;
// };

// struct priv
// {
//     struct window w;
//     GLFWwindow *win;
//     pl_opengl gl;
//     float scroll_dx, scroll_dy;
//     char **files;
//     size_t files_num;
//     size_t files_size;
//     bool file_seen;
//     struct window_pos windowed_pos;
// };

// class GlfwWindow
// {
// public:
//     static void err_cb(int code, const char *desc);
//     static void close_cb(GLFWwindow *win);
//     static void resize_cb(GLFWwindow *win, int width, int height);
//     static void scroll_cb(GLFWwindow *win, double dx, double dy);
//     static void drop_cb(GLFWwindow *win, int num, const char *files[]);

//     static bool make_current(void *priv);
//     static void release_current(void *priv);

// private:
//     static struct window *glfw_create(pl_log log, const struct window_params *params);
//     static void glfw_destroy(struct window **window);
//     static void glfw_poll(struct window *window, bool block);
//     static void glfw_get_cursor(const struct window *window, int *x, int *y);
//     static bool glfw_get_button(const struct window *window, enum button btn);
//     static bool glfw_get_key(const struct window *window, enum key key);
//     static void glfw_get_scroll(const struct window *window, float *dx, float *dy);
//     static char *glfw_get_file(const struct window *window);
//     static bool glfw_is_fullscreen(const struct window *window);
//     static bool glfw_toggle_fullscreen(const struct window *window, bool fullscreen);
//     static const char *glfw_get_clipboard(const struct window *window);
//     static void glfw_set_clipboard(const struct window *window, const char *text);
//     static void initIMPL();

// private:
//     const window_impl IMPL = {
//         .name = IMPL_NAME,
//         .tag = IMPL_TAG,
//         .create = glfw_create,
//         .destroy = glfw_destroy,
//         .poll = glfw_poll,
//         .get_cursor = glfw_get_cursor,
//         .get_scroll = glfw_get_scroll,
//         .get_button = glfw_get_button,
//         .get_key = glfw_get_key,
//         .get_file = glfw_get_file,
//         .toggle_fullscreen = glfw_toggle_fullscreen,
//         .is_fullscreen = glfw_is_fullscreen,
//         .get_clipboard = glfw_get_clipboard,
//         .set_clipboard = glfw_set_clipboard,
//     };

// protected:
// };