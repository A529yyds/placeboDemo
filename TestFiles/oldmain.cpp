// 有窗口模式代码

// #include <vector>
// #include <fstream>
// #include <iostream>

// // thread head files
// #include <mutex>
// #include <queue>
// #include <thread>
// #include <condition_variable>

// #include <GL/glx.h>
// #include <GLFW/glfw3.h>

// #define PL_LIBAV_IMPLEMENTATION 0
// #include <libplacebo/gpu.h>
// #include <libplacebo/log.h>
// #include <libplacebo/opengl.h>
// #include <libplacebo/options.h>
// #include <libplacebo/renderer.h>
// #include <libplacebo/colorspace.h>
// #include <libplacebo/utils/libav.h>
// #include <libplacebo/utils/upload.h>
// extern "C"
// {
// #include <libavutil/frame.h>
// #include <libavutil/imgutils.h>
// #include <libswscale/swscale.h>
// #include <libavformat/avformat.h>
// #include <libavfilter/avfilter.h>
// }

// // define global params
// #define WIDTH 3840
// #define HEIGHT 2160
// #define OUTWIDTH 4000
// #define OUTHEIGHT 2300
// #define OUTFILE "/root/lincy/placeboDemo/yuv422.yuv"
// // #define OUTFILE "/root/lincy/projects/placeboDemo/yuv_scaler.yuv"
// // #define VIDEOFILE "/root/lincy/projects/placeboDemo/yuv420p.mkv"
// #define VIDEOFILE "/root/lincy/placeboDemo/nv12.mkv"
// #define CERR(title, ctx) \
//     std::cerr << "[" << title << "] " << ctx << std::endl;
// #define COUT(title, ctx, params) \
//     std::cout << "[" << title << "] " << ctx << params << std::endl;
// #define EQ(a, b) .b = a.b,
// #define EQYUV(b) .b = pl_yuv.b,

// #define DEL(a)       \
//     if (a)           \
//     {                \
//         delete a;    \
//         a = nullptr; \
//     }

// std::ofstream _outfile(OUTFILE, std::ios::binary);
// // 线程安全的队列
// std::queue<pl_frame> _nvFrameQueue;
// std::mutex _mutex;
// std::condition_variable _cv;
// pl_gpu _gpu;
// pl_dispatch _dp;
// bool _bCLoseFile = false;
// bool _bLastData = false;

// // void downloadTex(pl_frame nvframe);
// // // 主线程将 pl_frame 放入队列
// // void pushFrameToQueue(pl_frame nv)
// // {
// //     {
// //         std::lock_guard<std::mutex> lock(_mutex); // 加锁
// //         _nvFrameQueue.push(nv);                   // 将 pl_frame 放入队列
// //         // std::cout << "Main thread: Pushed frame " << nv.id << " to the queue." << std::endl;
// //     }
// //     _cv.notify_one();                                           // 通知子线程可以取数据了
// //     std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 模拟延时
// // }
// // // 子线程从队列中取出 pl_frame 进行处理
// // void processFramesFromQueue()
// // {
// //     while (true)
// //     {
// //         std::unique_lock<std::mutex> lock(_mutex); // 使用 unique_lock 可以用于等待条件变量
// //         _cv.wait(lock, []()
// //                  { return !_nvFrameQueue.empty(); }); // 如果队列为空，等待主线程通知

// //         // 从队列中取出 pl_frame
// //         pl_frame frame = _nvFrameQueue.front();
// //         _nvFrameQueue.pop(); // 弹出队头的元素
// //         downloadTex(frame);
// //         lock.unlock(); // 解锁，因为队列已经操作完了
// //     }
// // }
// int r2(int org, int ctrl)
// {
//     return org / (ctrl == 0 ? 1 : 2);
// }

// bool yuv2nv(pl_renderer render, AVFrame *avframe)
// {
//     bool ret = true;
//     pl_frame pl_yuv = {.planes = {0}};
//     pl_frame_from_avframe(&pl_yuv, avframe);
//     pl_tex inTex[3] = {NULL, NULL, NULL};
//     if (!pl_map_avframe(_gpu, &pl_yuv, inTex, avframe))
//     {
//         COUT("ERROR", "pl_map_avframe ret is ", "false");
//         return false;
//     }
//     pl_tex outTex[4] = {NULL, NULL, NULL, NULL};
//     pl_frame target = {.planes = {0}};
//     bool bTestScaler = false;
//     int outW = WIDTH;
//     int outH = HEIGHT;
//     AVFrame *out_frame = av_frame_alloc();
//     out_frame->format = AV_PIX_FMT_YUV422P10LE;
//     if (bTestScaler)
//     {
//         outW = WIDTH * 1.5;
//         outH = HEIGHT * 1.5;
//         out_frame->format = AV_PIX_FMT_YUV420P;
//     }
//     out_frame->width = outW;
//     out_frame->height = outH;
//     pl_frame_recreate_from_avframe(_gpu, &target, outTex, out_frame);
//     pl_render_params params = pl_render_default_params;
//     params.upscaler = &pl_filter_bicubic;
//     if (!pl_render_image(render, &pl_yuv, &target, &params))
//     {
//         COUT("ERROR", "pl_render_image ret is ", "false");
//     }
//     else
//     {
//         av_frame_get_buffer(out_frame, 32);
//         if (pl_download_avframe(_gpu, &target, out_frame))
//         {
//             if (out_frame->format == AV_PIX_FMT_YUV420P)
//             {
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[0]), outW * outH);
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[1]), outW * outH / 4);
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[2]), outW * outH / 4);
//             }
//             if (out_frame->format == AV_PIX_FMT_YUV422P10LE)
//             {
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[0]), outW * outH * 2);
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[1]), outW / 2 * outH * 2);
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[2]), outW / 2 * outH * 2);
//             }
//             else if (out_frame->format == AV_PIX_FMT_NV12)
//             {
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[0]), outW * outH);
//                 _outfile.write(reinterpret_cast<const char *>(out_frame->data[1]), outW * outH / 2);
//             }
//         }
//         av_frame_free(&out_frame);
//     }
//     pl_tex_destroy(_gpu, &inTex[0]);
//     pl_tex_destroy(_gpu, &inTex[1]);
//     pl_tex_destroy(_gpu, &inTex[2]);
//     pl_tex_destroy(_gpu, &outTex[0]);
//     pl_tex_destroy(_gpu, &outTex[1]);
//     pl_tex_destroy(_gpu, &outTex[2]);
//     pl_tex_destroy(_gpu, &outTex[3]);
//     return ret;
// }

// bool yuv2nvTest(pl_renderer render, AVFrame *avframe)
// {
//     bool ret = true;
//     pl_frame pl_yuv = {.planes = {0}};
//     pl_frame_from_avframe(&pl_yuv, avframe);
//     pl_tex inTex[3] = {NULL, NULL, NULL};
//     if (!pl_map_avframe(_gpu, &pl_yuv, inTex, avframe))
//     {
//         COUT("ERROR", "pl_map_avframe ret is ", "false");
//         return false;
//     }
//     pl_plane planes[2];
//     pl_tex outTex[2] = {NULL, NULL};
//     uint8_t *data[2] = {NULL, NULL};
//     // for (int i = 0; i < 2; i++)
//     // {
//     //     int w = avframe->width;
//     //     int h = r2(avframe->height, i);
//     //     data[i] = new uint8_t(w * h);
//     //     pl_plane_data planeData = {
//     //         .type = PL_FMT_UNORM,
//     //         .width = w,
//     //         .height = h,
//     //         .component_size = {8 * sizeof(uint8_t)},
//     //         .component_pad = 0,
//     //         .component_map = {i},
//     //         .pixel_stride = sizeof(uint8_t),
//     //         .row_stride = w,
//     //         .pixels = data[i], // avframe->data[i],
//     //     };

//     //     if (!pl_recreate_plane(_gpu, NULL, &outTex[i], &planeData))
//     //         return false;
//     //     planes[i] = pl_yuv.planes[i];
//     //     planes[i].texture = outTex[i];
//     // }
//     pl_frame target = {
//         .num_planes = 2,
//         .planes = {
//             // planes[0],
//             // planes[1],
//         },
//         EQYUV(repr) EQYUV(color) EQYUV(icc)};

//     AVFrame *out_frame = av_frame_alloc();
//     out_frame->format = AV_PIX_FMT_NV12;
//     out_frame->width = avframe->width;
//     out_frame->height = avframe->height;
//     pl_frame_recreate_from_avframe(_gpu, &target, outTex, out_frame);
//     pl_render_params params = pl_render_default_params;
//     params.upscaler = &pl_filter_bicubic;
//     ret = pl_render_image(render, &pl_yuv, &target, &params);
//     if (!ret)
//     {
//         COUT("ERROR", "pl_render_image ret is ", ret);
//     }
//     else
//     {
//         av_frame_get_buffer(out_frame, 32);
//         if (pl_download_avframe(_gpu, &target, out_frame))
//         {
//             _outfile.write(reinterpret_cast<const char *>(out_frame->data[0]), 3840 * 2160);
//             _outfile.write(reinterpret_cast<const char *>(out_frame->data[1]), 1920 * 1080 * 2);
//         }
//         av_frame_free(&out_frame);
//     }
//     pl_tex_destroy(_gpu, &inTex[0]);
//     pl_tex_destroy(_gpu, &inTex[1]);
//     pl_tex_destroy(_gpu, &inTex[2]);
//     pl_tex_destroy(_gpu, &outTex[0]);
//     pl_tex_destroy(_gpu, &outTex[1]);
//     DEL(data[0]);
//     DEL(data[1]);
//     return ret;
// }

// bool yuv2nv1(pl_renderer render, AVFrame *avframe)
// {
//     bool ret = true;
//     pl_frame pl_yuv = {.planes = {0}};
//     pl_frame_from_avframe(&pl_yuv, avframe);
//     pl_tex inTex[3] = {NULL, NULL, NULL};
//     pl_tex outTex[2] = {NULL, NULL};
//     for (int i = 0; i < pl_yuv.num_planes; i++)
//     {
//         int w = r2(avframe->width, i);
//         int h = r2(avframe->height, i);
//         pl_plane_data planeData = {
//             .type = PL_FMT_UNORM,
//             .width = w,
//             .height = h,
//             .component_size = {8 * sizeof(uint8_t)},
//             .component_map = {i},
//             .pixel_stride = sizeof(uint8_t),
//             .pixels = avframe->data[i],
//         };
//         if (!pl_upload_plane(_gpu, &pl_yuv.planes[i], &inTex[i], &planeData))
//         {
//             CERR("ERROR", "pl_upload_plane failure!")
//             return false;
//         }
//         if (i < 2)
//         {
//             pl_tex_params nvParams = {
//                 .w = w,
//                 .h = h,
//                 .format = pl_plane_find_fmt(_gpu, nullptr, &planeData),
//                 .sampleable = true,
//                 .renderable = true,
//                 .host_readable = true,
//             };
//             outTex[i] = pl_tex_create(_gpu, &nvParams);
//         }
//     }
//     pl_frame nv12_frame = {
//         .num_planes = 2,
//         .planes = {
//             {
//                 // Y平面
//                 .texture = outTex[0],
//                 .components = 1,
//                 .component_mapping = {0},
//             },
//             {
//                 // UV平面
//                 .texture = outTex[1],
//                 .components = 2,
//                 .component_mapping = {1, 2},
//             },
//         },
//         .repr = {
//             .sys = PL_COLOR_SYSTEM_BT_709, // 或 BT_709
//             .levels = PL_COLOR_LEVELS_LIMITED,
//             .bits = {.color_depth = 8}, // NV12通常为8位
//         },
//         .color = pl_color_space_bt709, // 需根据实际输入指定
//     };

//     ret = pl_render_image(render, &pl_yuv, &nv12_frame, &pl_render_default_params);

//     COUT("UPSCALER", "pl_num_scale_filters is ", pl_num_scale_filters);
//     for (int i = 0; i < pl_num_scale_filters; i++)
//     {
//         struct pl_render_params params = pl_render_default_params;
//         params.upscaler = pl_scale_filters[i].filter;
//         COUT("UPSCALER", "name is ", pl_scale_filters[i].name);
//     }
//     ret = false;
//     if (!ret)
//     {
//         COUT("ERROR", "pl_render_image ret is ", ret);
//     }
//     else
//     {
//         downloadTex(nv12_frame);
//         pl_tex_destroy(_gpu, &inTex[0]);
//         pl_tex_destroy(_gpu, &inTex[1]);
//         pl_tex_destroy(_gpu, &inTex[2]);
//         pl_tex_destroy(_gpu, &outTex[0]);
//         pl_tex_destroy(_gpu, &outTex[1]);
//     }
//     std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 模拟延时
//     _cv.notify_one();                                           // 通知子线程可以取数据了
//     _outfile.close();
//     return ret;
// }
// struct callData
// {
//     uint8_t *data;
//     int size;
// };

// callData calldata[3] = {{
//                             .data = new uint8_t[3840 * 2160],
//                             .size = 0,
//                         },
//                         {
//                             .data = new uint8_t[1920 * 1080],
//                             .size = 0,
//                         },
//                         {
//                             .data = new uint8_t[1920 * 1080],
//                             .size = 0,
//                         }};

// void callback(void *priv)
// {
//     callData *data = (callData *)priv;
//     _outfile.write(reinterpret_cast<const char *>(data->data), data->size);
//     COUT("CALLBACK", "datasize is ", data->size);
// }

// void downloadTex(pl_frame target)
// {
//     pl_tex_transfer_params tsParams[3] = {{}, {}, {}};
//     for (int i = 0; i < target.num_planes; i++)
//     {
//         pl_tex tex = target.planes[i].texture;
//         int w = tex->params.w;
//         int h = tex->params.h;
//         int bufSize = w * h;
//         calldata[i].size = bufSize;
//         tsParams[i] = {
//             .tex = tex,
//             .row_pitch = w * tex->params.format->texel_size,
//             .depth_pitch = 3,
//             .callback = callback,
//             .priv = (void *)&calldata[i],
//             .ptr = calldata[i].data,
//         };
//         COUT("tsParams", "i = ", i);
//         COUT("tsParams", "w = ", w);
//         COUT("tsParams", "h = ", h);
//         if (tex)
//             COUT("tsParams", "texture = ", tex);
//         if (calldata[i].data)
//             COUT("tsParams", "calldata[i].data = ", &calldata[i].data);
//         COUT("tsParams", "i = ", i);
//         if (!pl_tex_download(_gpu, &tsParams[i]))
//         {
//             COUT("ERROR", "纹理下载失败！i = ", i);
//         }
//     }
// }

// void downloadTex1(pl_tex outtex[2])
// {
//     static bool flag = true;
//     pl_tex_transfer_params tsParams[2] = {{}, {}};
//     for (int i = 0; i < 2; i++)
//     {
//         pl_tex tex = outtex[i];
//         int w = tex->params.w;
//         int h = tex->params.h;
//         int bufSize = w * h * (i == 1 ? 2 : 1);
//         calldata[i].size = bufSize;
//         tsParams[i] = {
//             .tex = tex,
//             .rc = {
//                 .x0 = 0,
//                 .y0 = 0,
//                 .x1 = w,
//                 .y1 = h,
//             },
//             .row_pitch = w * tex->params.format->texel_size * (i == 1 ? 2 : 1),
//             .depth_pitch = tsParams[i].row_pitch * 2,
//             .callback = callback,
//             .priv = (void *)&calldata[i],
//             .ptr = calldata[i].data,
//         };
//         if (!pl_tex_download(_gpu, &tsParams[i]))
//         {
//             CERR("ERROR", "纹理下载失败！");
//         }
//     }
//     flag = false;
// }
// void downloadTex2(pl_frame nvframe)
// {
//     static bool flag = true;
//     nvframe.planes[0].texture;
//     nvframe.planes[1].texture->planes[0];
//     nvframe.planes[1].texture->planes[1];
//     pl_tex_transfer_params tsParams[2] = {{}, {}};
//     for (int i = 0; i < nvframe.num_planes; i++)
//     {
//         pl_tex tex = nvframe.planes[i].texture;
//         int j = 0;
//         int w = tex->params.w;
//         int h = tex->params.h;
//         int bufSize = w * h * (i == 1 ? 2 : 1);
//         while (i == 1 && j < 2)
//         {
//             tex = nvframe.planes[i].texture->planes[j];
//             w = tex->params.w;
//             h = tex->params.h;
//             bufSize = w * h;
//             calldata[i].size = bufSize;
//             tsParams[i] = {
//                 .tex = tex,
//                 .row_pitch = w * tex->params.format->texel_size * (i == 1 ? 2 : 1),
//                 .depth_pitch = 2,
//                 .callback = callback,
//                 .priv = (void *)&calldata[i],
//                 .ptr = calldata[i].data,
//             };
//             if (!pl_tex_download(_gpu, &tsParams[i]))
//             {
//                 CERR("ERROR", "纹理下载失败！");
//             }
//             j++;
//         }
//         calldata[i].size = bufSize;
//         tsParams[i] = {
//             .tex = tex,
//             .row_pitch = w * tex->params.format->texel_size * (i == 1 ? 2 : 1),
//             .depth_pitch = 2,
//             .callback = callback,
//             .priv = (void *)&calldata[i],
//             .ptr = calldata[i].data,
//         };
//         if (!pl_tex_download(_gpu, &tsParams[i]))
//         {
//             CERR("ERROR", "纹理下载失败！");
//         }
//     }
//     flag = false;
// }

// int readAvFrame(const char *filename, pl_renderer render)
// {
//     avformat_network_init();
//     AVFormatContext *fmtCtx = nullptr;
//     if (avformat_open_input(&fmtCtx, filename, nullptr, nullptr) < 0)
//     {
//         std::cout << "avformat_open_input failed!\n";
//         return -1;
//     }
//     if (avformat_find_stream_info(fmtCtx, nullptr) < 0)
//     {
//         std::cout << "avformat_find_stream_info failed!\n";
//         return -1;
//     }

//     int videoStreamIndex = -1;
//     for (unsigned i = 0; i < fmtCtx->nb_streams; ++i)
//     {
//         if (fmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
//         {
//             videoStreamIndex = i;
//             break;
//         }
//     }
//     if (videoStreamIndex < 0)
//     {
//         std::cout << "videoStreamIndex < 0\n";
//         return -1;
//     }
//     AVCodecParameters *codecpar = fmtCtx->streams[videoStreamIndex]->codecpar;
//     const AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
//     if (!decoder)
//     {
//         std::cout << "vavcodec_find_decoder failed\n";
//         avformat_close_input(&fmtCtx);
//         return -1;
//     }
//     AVCodecContext *codecCtx = avcodec_alloc_context3(decoder);
//     if (!codecCtx)
//     {
//         std::cout << "avcodec_alloc_context3 failed\n";
//         avcodec_free_context(&codecCtx);
//         avformat_close_input(&fmtCtx);
//         return -1;
//     }
//     if (avcodec_parameters_to_context(codecCtx, codecpar) < 0)
//     {
//         std::cout << "avcodec_parameters_to_context failed\n";
//         avcodec_free_context(&codecCtx);
//         avformat_close_input(&fmtCtx);
//         return -1;
//     }
//     if (avcodec_open2(codecCtx, decoder, nullptr) < 0)
//     {
//         std::cout << "avcodec_open2 failed\n";
//         avcodec_free_context(&codecCtx);
//         avformat_close_input(&fmtCtx);
//         return -1;
//     }

//     AVPacket *pkt = av_packet_alloc();
//     AVFrame *frame = av_frame_alloc();
//     bool stopFlag = false;
//     while (av_read_frame(fmtCtx, pkt) >= 0)
//     {
//         if (pkt->stream_index == videoStreamIndex)
//         {
//             if (avcodec_send_packet(codecCtx, pkt) == 0)
//             {
//                 while (avcodec_receive_frame(codecCtx, frame) == 0)
//                 {
//                     if (!yuv2nv(render, frame))
//                     {
//                         stopFlag = true;
//                         break;
//                     }
//                 }
//                 if (stopFlag)
//                     break;
//             }
//         }
//     }
//     _bCLoseFile = true;
//     _outfile.close();
//     av_frame_free(&frame);
//     av_packet_free(&pkt);
//     avcodec_free_context(&codecCtx);
//     avformat_close_input(&fmtCtx);
// }

// // 带窗口版的main代码：
// // // OpenGL函数加载器
// // pl_voidfunc_t get_proc_addr(const char *procname)
// // {
// //     return (pl_voidfunc_t)glXGetProcAddressARB((const GLubyte *)procname);
// // }
// // struct GLXContextData
// // {
// //     Display *display;
// //     GLXDrawable drb;
// //     GLXContext ctx;
// // };
// // // OpenGL上下文绑定和释放的回调
// // bool make_current(void *priv)
// // {
// //     // 1. 绑定OpenGL上下文
// //     // Display *display = (Display *)priv;
// //     // GLXContext context = (GLXContext)priv;
// //     // GLXDrawable drb = DefaultRootWindow(display);
// //     // return glXMakeContextCurrent(display, drb, drb, context);
// //     GLXContextData *data = (GLXContextData *)priv;
// //     return glXMakeCurrent(data->display, data->drb, data->ctx);
// // }
// // void release_current(void *priv)
// // {
// //     GLXContextData *data = (GLXContextData *)priv;
// //     glXMakeContextCurrent(data->display, None, None, NULL);
// //     glXDestroyContext(data->display, data->ctx);
// //     XDestroyWindow(data->display, data->drb);
// //     XCloseDisplay(data->display);
// // }
// // void myLogCallback(void *log_priv, enum pl_log_level level, const char *msg)
// // {
// //     // const char *lvStr = "???";
// //     // switch (level)
// //     // {
// //     // case PL_LOG_FATAL:
// //     //     lvStr = "FATAL";
// //     //     break;
// //     // case PL_LOG_ERR:
// //     //     lvStr = "PL_LOG_ERR";
// //     //     break;
// //     // case PL_LOG_WARN:
// //     //     lvStr = "PL_LOG_WARN";
// //     //     break;
// //     // case PL_LOG_INFO:
// //     //     lvStr = "PL_LOG_INFO";
// //     //     break;
// //     // case PL_LOG_DEBUG:
// //     //     lvStr = "PL_LOG_DEBUG";
// //     //     break;
// //     // case PL_LOG_TRACE:
// //     //     lvStr = "PL_LOG_TRACE";
// //     //     break;
// //     // default:
// //     //     break;
// //     // }
// //     std::cout << "[libplacebo]" << msg << std::endl;
// // }
// // struct gl_priv_data
// // {
// //     GLFWwindow *window;
// //     bool owns_context; // 标记是否由我们创建上下文
// // };
// // // 上下文绑定回调
// // static bool gl_make_current(void *priv)
// // {
// //     gl_priv_data *data = (gl_priv_data *)priv;
// //     if (!data || !data->window)
// //         return false;
// //     glfwMakeContextCurrent(data->window);
// //     return true;
// // }
// // // 上下文释放回调
// // static void gl_release_current(void *priv)
// // {
// //     glfwMakeContextCurrent(NULL);
// // }
// // // 完整初始化函数
// // struct pl_opengl_params init_gl_params(GLFWwindow *window, bool debug_mode)
// // {
// //     // 初始化GLFW上下文（如果尚未初始化）
// //     if (!glfwGetCurrentContext())
// //     {
// //         glfwMakeContextCurrent(window);
// //         glfwSwapInterval(1); // 启用垂直同步
// //     }
// //     // 配置私有数据结构
// //     gl_priv_data *priv = new gl_priv_data();
// //     *priv = (gl_priv_data){
// //         .window = window,
// //         .owns_context = !glfwGetCurrentContext(),
// //     };
// //     // 填充pl_opengl_params
// //     return (struct pl_opengl_params){
// //         .get_proc_addr_ex = NULL,
// //         .proc_ctx = NULL,
// //         .get_proc_addr = glfwGetProcAddress,
// //         .debug = debug_mode,
// //         .allow_software = false, // 默认禁用软件渲染
// //         .no_compute = false,     // 启用计算着色器
// //         .max_glsl_version = 450, // 典型桌面GLSL版本
// //         .egl_display = NULL,
// //         .egl_context = NULL,
// //         .make_current = gl_make_current,
// //         .release_current = gl_release_current,
// //         .priv = priv,
// //     };
// // }
// // 清理函数
// // void cleanup_gl_params(struct pl_opengl_params *params)
// // {
// //     if (params->priv)
// //     {
// //         gl_priv_data *data = (gl_priv_data *)params->priv;
// //         if (data->owns_context)
// //             glfwMakeContextCurrent(NULL);
// //         free(data);
// //     }
// // }
// // void upscale(pl_renderer rr, pl_tex fbo, pl_frame image)
// // {
// //     // struct pl_frame target = {
// //     //     .num_planes = 1,
// //     //     .planes = {{
// //     //         .texture = fbo,
// //     //         .components = 3,
// //     //         .component_mapping = {0, 1, 2},
// //     //     }},
// //     //     .repr = {
// //     //         .sys = PL_COLOR_SYSTEM_RGB,
// //     //         .levels = PL_COLOR_LEVELS_FULL,
// //     //         .bits.color_depth = 32,
// //     //     },
// //     //     .color = pl_color_space_srgb,
// //     // };
// //     // for (int i = 0; i < pl_num_scale_filters; i++)
// //     // {
// //     //     struct pl_render_params params = pl_render_default_params;
// //     //     params.upscaler = pl_scale_filters[i].filter;
// //     //     printf("- testing `params.upscaler = /* %s */`\n", pl_scale_filters[i].name);
// //     //     pl_render_image(rr, &image, &target, &params);
// //     //     pl_gpu_flush(_gpu);
// //     //     pl_renderer_get_errors(rr).errors == PL_RENDER_ERR_NONE;
// //     // }
// // }
// // void glfw_error_callback(int error, const char *description)
// // {
// //     std::cerr << "GLFW Error [" << error << "]: " << description << std::endl;
// // }
// // int main2()
// // {
// //     // readAvFrame(VIDEOFILE, pl_renderer());
// //     glfwSetErrorCallback(glfw_error_callback);
// //     int ret = glfwInit();
// //     // 不显示窗口
// //     glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
// //     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
// //     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
// //     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
// //     GLFWwindow *window = glfwCreateWindow(800, 600, "Demo", NULL, NULL);
// //     // 初始化参数
// //     struct pl_opengl_params params = init_gl_params(window, true);
// //     pl_log_params logParams{
// //         .log_cb = myLogCallback,
// //         .log_level = PL_LOG_ERR,
// //     };
// //     pl_log log = pl_log_create(PL_API_VER, &logParams);
// //     // 创建GPU实例
// //     pl_opengl gl = pl_opengl_create(log, &params);
// //     _gpu = gl->gpu;
// //     _dp = pl_dispatch_create(log, _gpu);
// //     if (!_gpu)
// //     {
// //         fprintf(stderr, "Failed creating OpenGL GPU!\n");
// //         return 1;
// //     }
// //     pl_renderer rder = pl_renderer_create(log, _gpu);
// //     // std::thread workerThread(processFramesFromQueue);
// //     // _queue = pl_queue_create(gpu);
// //     readAvFrame(VIDEOFILE, rder);
// //     // workerThread.join();
// //     std::cout << VIDEOFILE << " file find end ... \n";
// //     // 清理
// //     pl_renderer_destroy(&rder);
// //     pl_gpu_flush(_gpu);
// //     pl_gpu_finish(_gpu);
// //     cleanup_gl_params(&params);
// //     glfwTerminate();
// // }
