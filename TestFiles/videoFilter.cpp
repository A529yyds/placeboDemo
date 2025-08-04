// #include "videoFilter.h"
// #include "GLFW/glfw3.h"
// #define WIDTH 1920
// #define HEIGHT 1080
// #define SWIDTH (WIDTH >> 1)
// #define SHEIGHT (HEIGHT >> 1)
// #define ALIGN2(x, align) (((x) + (align) - 1) & ~((align) - 1))
// #define TEXELSZ sizeof(uint8_t)
// #define STRIDE (ALIGN2(WIDTH, 256) * TEXELSZ)
// #define SSTRIDE (ALIGN2(SWIDTH, 256) * TEXELSZ)

// #define SIZE0 (HEIGHT * STRIDE)
// #define OFFSET1 SIZE0
// #define SIZE1 (2 * SHEIGHT * SSTRIDE)
// #define BUFSIZE (OFFSET1 + SIZE1)

// #define OFFSET0 0
// #define FRAMES 10000

// VideoFilter::VideoFilter()
// {
// }

// VideoFilter::~VideoFilter()
// {
// }

// void VideoFilter::api1Example()
// {
//     init();
//     image example_image = {
//         .width = WIDTH,
//         .height = HEIGHT,
//         .num_planes = 2,
//         .planes = {
//             {
//                 .subx = 0,
//                 .suby = 0,
//                 .stride = STRIDE,
//             },
//             {
//                 .subx = 1,
//                 .suby = 1,
//                 .stride = SSTRIDE * 2,
//             },
//         },
//     };
//     example_image.planes[0].fmt = {
//         .num_comps = 1,
//         .bitdepth = 8 * TEXELSZ};
//     example_image.planes[1].fmt = {
//         .num_comps = 2,
//         .bitdepth = 8 * TEXELSZ};

//     if (!api1_reconfig(&example_image))
//     {
//         fprintf(stderr, "api1: Failed configuring video filter!\n");
//         return;
//     }

//     uint8_t *srcbuf = new uint8_t(BUFSIZE);
//     uint8_t *dstbuf = new uint8_t(BUFSIZE);
//     for (size_t i = 0; i < BUFSIZE; i++)
//         srcbuf[i] = i;

//     struct image src = example_image, dst = example_image;
//     src.planes[0].data = srcbuf + OFFSET0;
//     src.planes[1].data = srcbuf + OFFSET1;
//     dst.planes[0].data = dstbuf + OFFSET0;
//     dst.planes[1].data = dstbuf + OFFSET1;

//     const pl_clock_t start = pl_clock_now();
//     // Process this dummy frame a bunch of times
//     unsigned frames = 0;
//     for (frames = 0; frames < FRAMES; frames++)
//     {
//         if (!api1_filter(&dst, &src))
//         {
//             fprintf(stderr, "api1: Failed filtering frame... aborting\n");
//             break;
//         }
//     }
//     const pl_clock_t stop = pl_clock_now();
//     const float secs = pl_clock_diff(stop, start);
//     printf("api1: %4u frames in %1.6f s => %2.6f ms/frame (%5.2f fps)\n",
//            frames, secs, 1000 * secs / frames, frames / secs);
//     if (p->render_count)
//     {
//         printf("      render: %f ms, upload: %f ms, download: %f ms\n",
//                1e-6 * p->render_sum / p->render_count,
//                p->upload_count ? (1e-6 * p->upload_sum / p->upload_count) : 0.0,
//                p->download_count ? (1e-6 * p->download_sum / p->download_count) : 0.0);
//     }
//     free(srcbuf);
//     free(dstbuf);
// }

// // void VideoFilter::createInstance()
// // {
// //     pl_opengl_params glparams;
// //     glparams.allow_software = true;
// //     glparams.debug = false;
// //     glparams.get_proc_addr = glfwGetProcAddress;
// //     pl_log_params params{
// //         .log_cb = pl_log_color,
// //         .log_level = PL_LOG_INFO,
// //     };
// //     _plLog = pl_log_create(PL_API_VER, &params);
// //     _pl = pl_opengl_create(_plLog, &glparams);
// // }
// // void VideoFilter::configYUV420P(GLuint yTex, GLuint uvTex, uint16_t w, uint16_t h, uint16_t depth)
// // {
// //     pl_opengl_wrap_params params =
// //         {
// //             .texture = yTex,
// //             .framebuffer = 0, // 默认缓冲
// //             .width = w,
// //             .height = h,
// //             .depth = depth,
// //             .target = GL_TEXTURE_2D,
// //             .iformat = GL_LUMINANCE};
// //     _yTex = pl_opengl_wrap(_gpu, &params); // 包装y纹理
// //     // uv纹理
// //     params.texture = uvTex; // 将uv纹理传给包装器
// //     _uvTex = pl_opengl_wrap(_gpu, &params);
// // }

// // void VideoFilter::configNV12(GLuint nv12Tex, uint16_t w, uint16_t h, uint16_t depth)
// // {
// //     // glGenTextures(1, &nv12Tex);
// //     // GLsizei sizei = w * h;
// //     // glBindTextures(GL_TEXTURE_2D, sizei, &nv12Tex);
// //     // glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w, h, 0, GL_RG, GL_UNSIGNED_BYTE, NULL);
// //     // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
// //     // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
// //     // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
// //     // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

// //     pl_opengl_wrap_params params = {
// //         .texture = nv12Tex,
// //         .framebuffer = 0, // 默认缓冲
// //         .width = w,
// //         .height = h,
// //         .depth = depth,
// //         .target = GL_TEXTURE_2D,
// //         .iformat = GL_RG};
// //     _nv12Tex = pl_opengl_wrap(_gpu, &params);
// // }

// // void VideoFilter::yuv420p2nv12()
// // {
// //     // 构建着色器
// //     // const char *vertexShaderCode =R"";
// // }

// static bool make_current(void *priv)
// {
//     // GLFWwindow *win = (GLFWwindow *)priv;
//     // glfwMakeContextCurrent(win);
//     return true;
// }

// static void release_current(void *priv)
// {
//     glfwMakeContextCurrent(NULL);
// }
// void VideoFilter::init()
// {
//     p = (priv *)calloc(1, sizeof(struct priv));
//     pl_opengl_params glparams;
//     glparams.allow_software = true;
//     glparams.debug = true;
//     glparams.make_current = make_current;
//     glparams.release_current = release_current;
//     glparams.get_proc_addr = glfwGetProcAddress;
//     glparams.priv = p;
//     pl_log_params params{
//         .log_cb = pl_log_simple,
//         .log_level = PL_LOG_INFO,
//     };
//     p->log = pl_log_create(PL_API_VER, &params);
//     p->gl = pl_opengl_create(p->log, &glparams);

//     p->gpu = p->gl->gpu;
//     p->render_timer = pl_timer_create(p->gpu);
//     p->upload_timer = pl_timer_create(p->gpu);
//     p->download_timer = pl_timer_create(p->gpu);
//     p->dp = pl_dispatch_create(p->log, p->gpu);
// }

// // bool VideoFilter::make_current(void *priv)
// // {
// //     return true;
// // }

// // void VideoFilter::release_current(void *priv)
// // {
// //     glfwMakeContextCurrent(NULL);
// // }

// bool VideoFilter::api1_reconfig(const image *proxy)
// {
//     pl_plane_data data[MAX_PLANES];
//     setup_plane_data(proxy, data);
//     pl_tex_params params = {
//         .sampleable = true,
//         .host_writable = true,
//     };
//     for (int i = 0; i < proxy->num_planes; i++)
//     {
//         pl_fmt fmt = pl_plane_find_fmt(p->gpu, NULL, &data[i]);
//         if (!fmt)
//         {
//             fprintf(stderr, "Failed configuring filter: no good texture format!\n");
//             return false;
//         }
//         bool ok = true;
//         params.w = data[i].width;
//         params.h = data[i].height;
//         params.format = fmt;
//         ok &= pl_tex_recreate(p->gpu, &p->tex_in[i], &params);
//         ok &= pl_tex_recreate(p->gpu, &p->tex_out[i], &params);
//         if (!ok)
//         {
//             fprintf(stderr, "Failed creating GPU textures!\n");
//             return false;
//         }
//     }
//     return true;
// }

// bool VideoFilter::api1_filter(image *dst, image *src)
// {
//     pl_plane_data data[MAX_PLANES];
//     setup_plane_data(src, data);
//     // Upload planes
//     pl_tex_transfer_params tfParams;
//     for (int i = 0; i < src->num_planes; i++)
//     {
//         // ??
//         tfParams.tex = p->tex_in[i];
//         tfParams.row_pitch = data[i].row_stride;
//         tfParams.ptr = src->planes[i].data;
//         tfParams.timer = p->upload_timer;
//         bool ok = pl_tex_upload(p->gpu, &tfParams);
//         if (!ok)
//         {
//             fprintf(stderr, "Failed uploading data to the GPU!\n");
//             return false;
//         }
//     }

//     // Process planes
//     for (int i = 0; i < src->num_planes; i++)
//     {
//         if (!do_plane(p->tex_out[i], p->tex_in[i]))
//         {
//             fprintf(stderr, "Failed processing planes!\n");
//             return false;
//         }
//     }
//     // Download planes
//     for (int i = 0; i < src->num_planes; i++)
//     {
//         tfParams.tex = p->tex_out[i];
//         tfParams.row_pitch = dst->planes[i].stride;
//         tfParams.ptr = dst->planes[i].data;
//         tfParams.timer = p->download_timer;
//         bool ok = pl_tex_download(p->gpu, &tfParams);
//         if (!ok)
//         {
//             fprintf(stderr, "Failed downloading data from the GPU!\n");
//             return false;
//         }
//     }
//     check_timers();
//     return true;
// }

// void VideoFilter::setup_plane_data(const image *img, pl_plane_data out[MAX_PLANES])

// {
//     for (int i = 0; i < img->num_planes; i++)
//     {
//         const struct plane *plane = &img->planes[i];

//         out[i] = (struct pl_plane_data){
//             .type = PL_FMT_UNORM,
//             .width = img->width >> plane->subx,
//             .height = img->height >> plane->suby,
//             .pixel_stride = plane->fmt.num_comps * plane->fmt.bitdepth / 8,
//             .row_stride = plane->stride,
//             .pixels = plane->data,
//         };

//         // For API 2 (direct rendering)
//         if (img->associated_buf)
//         {
//             pl_buf buf = (pl_buf)img->associated_buf->priv;
//             out[i].pixels = NULL;
//             out[i].buf = buf;
//             out[i].buf_offset = (uintptr_t)plane->data - (uintptr_t)buf->data;
//         }

//         for (int c = 0; c < plane->fmt.num_comps; c++)
//         {
//             out[i].component_size[c] = plane->fmt.bitdepth;
//             out[i].component_pad[c] = 0;
//             out[i].component_map[c] = c;
//         }
//     }
// }

// bool VideoFilter::do_plane(pl_tex dst, pl_tex src)
// {
//     int new_depth = dst->params.format->component_depth[0];
//     // Do some debanding, and then also make sure to dither to the new depth
//     // so that our debanded gradients are actually preserved well
//     pl_shader sh = pl_dispatch_begin(p->dp);
//     pl_sample_src srcParams{.tex = src};
//     pl_shader_deband(sh, &srcParams, NULL);
//     pl_shader_dither(sh, new_depth, &p->dither_state, NULL);
//     pl_dispatch_params params = {
//         .shader = &sh,
//         .target = dst,
//         .timer = p->render_timer,
//     };
//     return pl_dispatch_finish(p->dp, &params);
// }

// void VideoFilter::check_timers()
// {
//     uint64_t ret;

//     while ((ret = pl_timer_query(p->gpu, p->render_timer)))
//     {
//         p->render_sum += ret;
//         p->render_count++;
//     }

//     while ((ret = pl_timer_query(p->gpu, p->upload_timer)))
//     {
//         p->upload_sum += ret;
//         p->upload_count++;
//     }

//     while ((ret = pl_timer_query(p->gpu, p->download_timer)))
//     {
//         p->download_sum += ret;
//         p->download_count++;
//     }
// }
