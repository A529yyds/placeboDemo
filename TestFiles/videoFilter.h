// #pragma once
// #include <libplacebo/opengl.h>
// #include <libplacebo/utils/upload.h>
// #include <GL/glew.h>
// #include <vector>
// #include <time.h>

// // Functions for creating persistently mapped buffers
// struct api2_buf
// {
//     void *data;
//     size_t size;
//     void *priv;
// };

// #define MAX_PLANES 4
// struct format
// {
//     // For simplicity let's make a few assumptions here, since configuring the
//     // texture format is not the point of this example. (In practice you can
//     // go nuts with the `utils/upload.h` helpers)
//     //
//     // - All formats contain unsigned integers only
//     // - All components have the same size in bits
//     // - All components are in the "canonical" order
//     // - All formats have power of two sizes only (2 or 4 components, not 3)
//     // - All plane strides are a multiple of the pixel size
//     int num_comps;
//     int bitdepth;
// };
// struct plane
// {
//     int subx, suby; // subsampling shift
//     struct format fmt;
//     size_t stride;
//     void *data;
// };

// struct image
// {
//     int width, height;
//     int num_planes;
//     struct plane planes[MAX_PLANES];

//     // For API #2, the associated mapped buffer (if any)
//     struct api2_buf *associated_buf;
// };

// typedef uint64_t pl_clock_t;

// // For both APIs:
// struct priv
// {
//     pl_log log;
//     pl_opengl gl;
//     pl_gpu gpu;
//     pl_dispatch dp;
//     pl_shader_obj dither_state;

//     // Timer objects
//     pl_timer render_timer;
//     pl_timer upload_timer;
//     pl_timer download_timer;
//     uint64_t render_sum;
//     uint64_t upload_sum;
//     uint64_t download_sum;
//     int render_count;
//     int upload_count;
//     int download_count;

//     // API #1: A simple pair of input and output textures
//     pl_tex tex_in[MAX_PLANES];
//     pl_tex tex_out[MAX_PLANES];
// };

// class VideoFilter
// {
// public:
//     VideoFilter();
//     ~VideoFilter();

//     void api1Example();
//     // // 创建OpenGL渲染器
//     // void createInstance();
//     // // 包装YUV420p输入纹理：使用pl_opengl_wrap包装Y和UV纹理为pl_tex对象
//     // void configYUV420P(GLuint yTex, GLuint _uvTex, uint16_t w, uint16_t h, uint16_t depth);
//     // // 创建NV12输出纹理：使用OpenGLv黄建一个纹理来保存NV12格式的数据
//     // void configNV12(GLuint nv12Tex, uint16_t w, uint16_t h, uint16_t depth);
//     // // 转化：仔fragment shader中进行格式转换，yuv420p的y与uv分量需要在nv12中交错存储
//     // void yuv420p2nv12();
//     // // 渲染和获取结果
//     // // 清理资源
// private:
//     void init();
//     // bool make_current(void *priv);
//     // void release_current(void *priv);
//     bool api1_reconfig(const image *proxy);
//     bool api1_filter(image *dst, image *src);
//     void setup_plane_data(const struct image *img, struct pl_plane_data out[MAX_PLANES]);
//     bool do_plane(pl_tex dst, pl_tex src);
//     void check_timers();
//     inline pl_clock_t pl_clock_now()
//     {
//         struct timespec tp = {.tv_sec = 0, .tv_nsec = 0};
//         timespec_get(&tp, TIME_UTC);
//         return tp.tv_sec * UINT64_C(1000000000) + tp.tv_nsec;
//     }
//     inline double pl_clock_diff(pl_clock_t a, pl_clock_t b)
//     {
//         double frequency = 1e9;
//         if (b > a)
//             return (b - a) / -frequency;
//         else
//             return (a - b) / frequency;
//     }

// protected:
// private:
//     // pl_opengl _pl;
//     // pl_log _plLog;
//     priv *p;
//     // pl_gpu _gpu;
//     // pl_dispatch _dp;
//     // pl_shader_obj _dither_state;
//     // pl_timer _upload_timer;
//     // pl_timer _render_timer;
//     // pl_timer _download_timer;

//     // uint64_t _render_sum;
//     // uint64_t _upload_sum;
//     // uint64_t _download_sum;
//     // int _render_count;
//     // int _upload_count;
//     // int _download_count;

//     pl_tex _yTex;
//     pl_tex _uvTex;
//     pl_tex _nv12Tex;
//     // pl_tex _tex_in[MAX_PLANES];
//     // pl_tex _tex_out[MAX_PLANES];
// };