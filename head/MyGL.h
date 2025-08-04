
/******************************
 * 文件名：MyGL.h
 * 功能：创建一个无窗口的opengl单例
 ******************************/

#pragma once

#include <EGL/egl.h>
#include <libplacebo/opengl.h>
#include <libplacebo/renderer.h>

class MyGL
{
public:
    MyGL(const MyGL &) = delete;
    MyGL &operator=(const MyGL &) = delete;
    static MyGL *getInstance();
    pl_gpu getGpu();
    pl_renderer getRenderer();

private:
    MyGL();
    ~MyGL();
    EGLDisplay GetSurfacelessDisplay();
    bool initGLValue();
    bool initGPU();

private:
    static MyGL *_instance;
    EGLDisplay _display;
    EGLContext _context;
    pl_gpu _gpu;
    pl_log _log;
    pl_opengl _plgl;
    pl_renderer _rder;
};