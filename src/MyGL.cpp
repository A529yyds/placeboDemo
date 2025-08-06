#include <MyGL.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <libplacebo/opengl.h>
#include <libplacebo/log.h>
#include "defines.h"

MyGL *MyGL::_instance = nullptr;

MyGL *MyGL::getInstance()
{
    if (_instance == nullptr)
    {
        _instance = new MyGL();
    }
    return _instance;
}

pl_gpu MyGL::getGpu()
{
    return _gpu;
}

pl_renderer MyGL::getRenderer()
{
    return _rder;
}

MyGL::MyGL()
{
    initGLValue();
    initGPU();
    // if (initGLValue() && initGPU())
    //     COUT("SUCCESS", "initGLValue and initGPU", "")
}

MyGL::~MyGL()
{
    pl_gpu_flush(_gpu);
    pl_gpu_finish(_gpu);
    pl_opengl_destroy(&_plgl);
    pl_log_destroy(&_log);
    eglDestroyContext(_display, _context);
    eglTerminate(_display);
    DELETE(_instance);
}

EGLDisplay MyGL::GetSurfacelessDisplay()
{
    auto eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC)
        eglGetProcAddress("eglGetPlatformDisplayEXT");

    if (!eglGetPlatformDisplayEXT)
    {
        CERR("ERROR", "eglGetPlatformDisplayEXT not available")
        return EGL_NO_DISPLAY;
    }

    return eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
}

bool MyGL::initGLValue()
{
    // Step 1: Get display
    _display = GetSurfacelessDisplay();
    if (_display == EGL_NO_DISPLAY)
    {
        CERR("ERROR", "Failed to get EGL display");
        return false;
    }

    // Step 2: Initialize EGL
    if (!eglInitialize(_display, nullptr, nullptr))
    {
        CERR("ERROR", "Failed to initialize EGL");
        return false;
    }

    // Step 3: Choose config
    EGLint config_attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE};
    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(_display, config_attrs, &config, 1, &num_configs))
    {
        CERR("ERROR", "Failed to choose EGL config");
        return false;
    }

    // Step 4: Create EGL context
    EGLint context_attrs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE};
    _context = eglCreateContext(_display, config, EGL_NO_CONTEXT, context_attrs);
    if (_context == EGL_NO_CONTEXT)
    {
        CERR("ERROR", "Failed to create EGL context");
        return false;
    }

    // Step 5: Make context current (even if no surface)
    if (!eglMakeCurrent(_display, EGL_NO_SURFACE, EGL_NO_SURFACE, _context))
    {
        CERR("ERROR", "Failed to make context current");
        return false;
    }
}

bool MyGL::initGPU()
{
    // Step 6: Fill pl_opengl_params
    struct pl_opengl_params params = {
        .get_proc_addr = eglGetProcAddress, // For GL function loading
        .debug = true,
        .allow_software = true,
        .egl_display = _display,
        .egl_context = _context};

    // Step 7: Create pl_opengl
    // 初始化参数
    pl_log_params logParams{
        // .log_cb = myLogCallback,
        .log_level = PL_LOG_ERR,
    };
    _log = pl_log_create(PL_API_VER, &logParams);
    _plgl = pl_opengl_create(_log, &params);
    if (!_plgl)
    {
        CERR("ERROR", "Failed to create pl_opengl")
        return false;
    }
    _gpu = _plgl->gpu;
    _rder = pl_renderer_create(_log, _gpu);
    return true;
}
