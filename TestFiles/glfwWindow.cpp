// #include "common.h"
// #include "glfwWindow.h"
// #include <iostream>
// #include <vector>
// #include <string>
// #include <math.h>

// #define GLFW_EXPOSE_NATIVE_EGL
// #include <GLFW/glfw3.h>
// #include <GLFW/glfw3native.h>

// #define IMPL win_impl_glfw_gl
// #define PL_ARRAY_SIZE(s) (sizeof(s) / sizeof((s)[0]))

// void GlfwWindow::err_cb(int code, const char *desc)
// {
//     fprintf(stderr, "GLFW err %d: %s\n", code, desc);
// }

// void GlfwWindow::close_cb(GLFWwindow *win)
// {
//     struct priv *p = (priv *)glfwGetWindowUserPointer(win);
//     p->w.window_lost = true;
// }

// void GlfwWindow::resize_cb(GLFWwindow *win, int width, int height)
// {
//     struct priv *p = (priv *)glfwGetWindowUserPointer(win);
//     if (!pl_swapchain_resize(p->w.swapchain, &width, &height))
//     {
//         fprintf(stderr, "libplacebo: Failed resizing swapchain? Exiting...\n");
//         p->w.window_lost = true;
//     }
// }

// void GlfwWindow::scroll_cb(GLFWwindow *win, double dx, double dy)
// {
//     struct priv *p = (priv *)glfwGetWindowUserPointer(win);
//     p->scroll_dx += dx;
//     p->scroll_dy += dy;
// }

// void GlfwWindow::drop_cb(GLFWwindow *win, int num, const char *files[])
// {
//     struct priv *p = (priv *)glfwGetWindowUserPointer(win);

//     for (int i = 0; i < num; i++)
//     {
//         if (p->files_num == p->files_size)
//         {
//             size_t new_size = p->files_size ? p->files_size * 2 : 16;
//             char **new_files = (char **)realloc(p->files, new_size * sizeof(char *));
//             if (!new_files)
//                 return;
//             p->files = new_files;
//             p->files_size = new_size;
//         }

//         char *file = strdup(files[i]);
//         if (!file)
//             return;

//         p->files[p->files_num++] = file;
//     }
// }

// bool GlfwWindow::make_current(void *priv)
// {
//     GLFWwindow *win = (GLFWwindow *)priv;
//     glfwMakeContextCurrent(win);
//     return true;
// }

// void GlfwWindow::release_current(void *priv)
// {
//     glfwMakeContextCurrent(NULL);
// }

// window *GlfwWindow::glfw_create(pl_log log, const window_params *params)
// {
//     struct priv *p = (priv *)calloc(1, sizeof(struct priv));
//     if (!p)
//         return NULL;
//     // window_impl impl;
//     // // initIMPL();

//     // impl.name = IMPL_NAME;
//     // impl.tag = IMPL_TAG;
//     // impl.create = glfw_create;
//     // impl.destroy = glfw_destroy;
//     // impl.poll = glfw_poll;
//     // impl.get_cursor = glfw_get_cursor;
//     // impl.get_button = glfw_get_button;
//     // impl.get_key = glfw_get_key;
//     // impl.get_scroll = glfw_get_scroll;
//     // impl.get_file = glfw_get_file;
//     // impl.toggle_fullscreen = glfw_toggle_fullscreen;
//     // impl.is_fullscreen = glfw_is_fullscreen;
//     // impl.get_clipboard = glfw_get_clipboard;
//     // impl.set_clipboard = glfw_set_clipboard;

//     p->w.impl = &IMPL;
//     if (!glfwInit())
//     {
//         fprintf(stderr, "GLFW: Failed initializing?\n");
//         // goto error;
//     }

//     glfwSetErrorCallback(&err_cb);
//     struct
//     {
//         int api;
//         int major, minor;
//         int glsl_ver;
//         int profile;
//     } gl_vers[] = {
//         {GLFW_OPENGL_API, 4, 6, 460, GLFW_OPENGL_CORE_PROFILE},
//         {GLFW_OPENGL_API, 4, 5, 450, GLFW_OPENGL_CORE_PROFILE},
//         {GLFW_OPENGL_API, 4, 4, 440, GLFW_OPENGL_CORE_PROFILE},
//         {GLFW_OPENGL_API, 4, 0, 400, GLFW_OPENGL_CORE_PROFILE},
//         {GLFW_OPENGL_API, 3, 3, 330, GLFW_OPENGL_CORE_PROFILE},
//         {GLFW_OPENGL_API, 3, 2, 150, GLFW_OPENGL_CORE_PROFILE},
//         {
//             GLFW_OPENGL_ES_API,
//             3,
//             2,
//             320,
//         },
//         {
//             GLFW_OPENGL_API,
//             3,
//             1,
//             140,
//         },
//         {
//             GLFW_OPENGL_ES_API,
//             3,
//             1,
//             310,
//         },
//         {
//             GLFW_OPENGL_API,
//             3,
//             0,
//             130,
//         },
//         {
//             GLFW_OPENGL_ES_API,
//             3,
//             0,
//             300,
//         },
//         {
//             GLFW_OPENGL_ES_API,
//             2,
//             0,
//             100,
//         },
//         {
//             GLFW_OPENGL_API,
//             2,
//             1,
//             120,
//         },
//         {
//             GLFW_OPENGL_API,
//             2,
//             0,
//             110,
//         },
//     };

//     for (int i = 0; i < PL_ARRAY_SIZE(gl_vers); i++)
//     {
//         glfwWindowHint(GLFW_CLIENT_API, gl_vers[i].api);
// #ifdef HAVE_EGL
//         glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
// #endif
//         glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_vers[i].major);
//         glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_vers[i].minor);
//         glfwWindowHint(GLFW_OPENGL_PROFILE, gl_vers[i].profile);
// #ifdef __APPLE__
//         if (gl_vers[i].profile == GLFW_OPENGL_CORE_PROFILE)
//             glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
// #endif if (params->alpha)
//         glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);

//         printf("Creating %dx%d window%s...\n", params->width, params->height,
//                params->alpha ? " (with alpha)" : "");

//         p->win = glfwCreateWindow(params->width, params->height, params->title, NULL, NULL);

//         if (p->win)
//             break;
//     }
//     if (!p->win)
//     {
//         fprintf(stderr, "GLFW: Failed creating window\n");
//         goto error;
//     }

//     // Set up GLFW event callbacks
//     glfwSetWindowUserPointer(p->win, p);
//     glfwSetFramebufferSizeCallback(p->win, resize_cb);
//     glfwSetWindowCloseCallback(p->win, close_cb);
//     glfwSetScrollCallback(p->win, scroll_cb);
//     glfwSetDropCallback(p->win, drop_cb);
//     pl_opengl_params plparams;
//     plparams.allow_software = true;
//     plparams.debug = false;
// #ifdef HAVE_EGL
//     plparams.egl_display = glfwGetEGLDisplay();
//     plparams.egl_context = glfwGetEGLContext(p->win);
// #endif
//     plparams.make_current = make_current;
//     plparams.release_current = release_current;
//     plparams.get_proc_addr = glfwGetProcAddress;
//     plparams.priv = p->win;
//     p->gl = pl_opengl_create(log, &plparams);
//     if (!p->gl)
//     {
//         fprintf(stderr, "libplacebo: Failed creating opengl device\n");
//         goto error;
//     }

//     pl_opengl_swapchain_params scparams;
//     scparams.swap_buffers = (void (*)(void *))glfwSwapBuffers;
//     scparams.priv = p->win;
//     p->w.swapchain = pl_opengl_create_swapchain(p->gl, &scparams);

//     if (!p->w.swapchain)
//     {
//         fprintf(stderr, "libplacebo: Failed creating opengl swapchain\n");
//         goto error;
//     }

//     p->w.gpu = p->gl->gpu;
//     glfwGetWindowSize(p->win, &p->windowed_pos.w, &p->windowed_pos.h);
//     glfwGetWindowPos(p->win, &p->windowed_pos.x, &p->windowed_pos.y);

//     int w, h;
//     glfwGetFramebufferSize(p->win, &w, &h);
//     pl_swapchain_colorspace_hint(p->w.swapchain, &params->colors);
//     if (!pl_swapchain_resize(p->w.swapchain, &w, &h))
//     {
//         fprintf(stderr, "libplacebo: Failed initializing swapchain\n");
//         goto error;
//     }

//     return &p->w;

// error:
//     window_destroy((struct window **)&p);
//     return NULL;
// }

// void GlfwWindow::glfw_destroy(window **window)
// {
//     struct priv *p = (struct priv *)*window;
//     if (!p)
//         return;

//     pl_swapchain_destroy(&p->w.swapchain);
//     pl_opengl_destroy(&p->gl);
//     for (int i = 0; i < p->files_num; i++)
//         free(p->files[i]);
//     free(p->files);

//     glfwTerminate();
//     free(p);
//     *window = NULL;
// }

// void GlfwWindow::glfw_poll(window *window, bool block)
// {
//     if (block)
//     {
//         glfwWaitEvents();
//     }
//     else
//     {
//         glfwPollEvents();
//     }
// }

// void GlfwWindow::glfw_get_cursor(const window *window, int *x, int *y)
// {
//     struct priv *p = (struct priv *)window;
//     double dx, dy;
//     int fw, fh, ww, wh;
//     glfwGetCursorPos(p->win, &dx, &dy);
//     glfwGetFramebufferSize(p->win, &fw, &fh);
//     glfwGetWindowSize(p->win, &ww, &wh);
//     *x = floor(dx * fw / ww);
//     *y = floor(dy * fh / wh);
// }

// void GlfwWindow::glfw_get_scroll(const window *window, float *dx, float *dy)
// {
//     struct priv *p = (struct priv *)window;
//     *dx = p->scroll_dx;
//     *dy = p->scroll_dy;
//     p->scroll_dx = p->scroll_dy = 0.0;
// }

// char *GlfwWindow::glfw_get_file(const window *window)
// {
//     struct priv *p = (struct priv *)window;
//     if (p->file_seen)
//     {
//         assert(p->files_num);
//         free(p->files[0]);
//         memmove(&p->files[0], &p->files[1], --p->files_num * sizeof(char *));
//         p->file_seen = false;
//     }

//     if (!p->files_num)
//         return NULL;

//     p->file_seen = true;
//     return p->files[0];
// }

// bool GlfwWindow::glfw_is_fullscreen(const window *window)
// {
//     const struct priv *p = (const struct priv *)window;
//     return glfwGetWindowMonitor(p->win);
// }

// bool GlfwWindow::glfw_toggle_fullscreen(const window *window, bool fullscreen)
// {
//     struct priv *p = (struct priv *)window;
//     bool window_fullscreen = glfw_is_fullscreen(window);

//     if (window_fullscreen == fullscreen)
//         return true;

//     if (window_fullscreen)
//     {
//         glfwSetWindowMonitor(p->win, NULL, p->windowed_pos.x, p->windowed_pos.y,
//                              p->windowed_pos.w, p->windowed_pos.h, GLFW_DONT_CARE);
//         return true;
//     }

//     // For simplicity sake use primary monitor
//     GLFWmonitor *monitor = glfwGetPrimaryMonitor();
//     if (!monitor)
//         return false;

//     const GLFWvidmode *mode = glfwGetVideoMode(monitor);
//     if (!mode)
//         return false;

//     glfwGetWindowPos(p->win, &p->windowed_pos.x, &p->windowed_pos.y);
//     glfwGetWindowSize(p->win, &p->windowed_pos.w, &p->windowed_pos.h);
//     glfwSetWindowMonitor(p->win, monitor, 0, 0, mode->width, mode->height,
//                          mode->refreshRate);

//     return true;
// }

// const char *GlfwWindow::glfw_get_clipboard(const window *window)
// {
//     struct priv *p = (struct priv *)window;
//     return glfwGetClipboardString(p->win);
// }

// void GlfwWindow::glfw_set_clipboard(const window *window, const char *text)
// {
//     struct priv *p = (struct priv *)window;
//     glfwSetClipboardString(p->win, text);
// }

// void GlfwWindow::initIMPL()
// {
//     // _impl.name = IMPL_NAME;
//     // _impl.tag = IMPL_TAG;
//     // _impl.create = glfw_create;
//     // _impl.destroy = glfw_destroy;
//     // _impl.poll = glfw_poll;
//     // _impl.get_cursor = glfw_get_cursor;
//     // _impl.get_button = glfw_get_button;
//     // _impl.get_key = glfw_get_key;
//     // _impl.get_scroll = glfw_get_scroll;
//     // _impl.get_file = glfw_get_file;
//     // _impl.toggle_fullscreen = glfw_toggle_fullscreen;
//     // _impl.is_fullscreen = glfw_is_fullscreen;
//     // _impl.get_clipboard = glfw_get_clipboard;
//     // _impl.set_clipboard = glfw_set_clipboard;
// }

// bool GlfwWindow::glfw_get_key(const window *window, key key)
// {
//     static const int key_map[] = {
//         [KEY_ESC] = GLFW_KEY_ESCAPE,
//     };

//     struct priv *p = (struct priv *)window;
//     return glfwGetKey(p->win, key_map[key]) == GLFW_PRESS;
// }

// bool GlfwWindow::glfw_get_button(const window *window, button btn)
// {
//     static const int button_map[] = {
//         [BTN_LEFT] = GLFW_MOUSE_BUTTON_LEFT,
//         [BTN_RIGHT] = GLFW_MOUSE_BUTTON_RIGHT,
//         [BTN_MIDDLE] = GLFW_MOUSE_BUTTON_MIDDLE,
//     };

//     struct priv *p = (struct priv *)window;
//     return glfwGetMouseButton(p->win, button_map[btn]) == GLFW_PRESS;
// }
