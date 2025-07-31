// // #include "videoFilter.h"

// // int main()
// // {
// //     VideoFilter filter;
// //     filter.api1Example();
// //     return 0;
// // }

// /* Presented are two hypothetical scenarios of how one might use libplacebo
//  * as something like an FFmpeg or mpv video filter. We examine two example
//  * APIs (loosely modeled after real video filtering APIs) and how each style
//  * would like to use libplacebo.
//  *
//  * For sake of a simple example, let's assume this is a debanding filter.
//  * For those of you too lazy to compile/run this file but still want to see
//  * results, these are from my machine (RX 5700 XT + 1950X, as of 2020-05-25):
//  *
//  * RADV+ACO:
//  *   api1: 10000 frames in 16.328440 s => 1.632844 ms/frame (612.43 fps)
//  *         render: 0.113524 ms, upload: 0.127551 ms, download: 0.146097 ms
//  *   api2: 10000 frames in 5.335634 s => 0.533563 ms/frame (1874.19 fps)
//  *         render: 0.064378 ms, upload: 0.000000 ms, download: 0.189719 ms
//  *
//  * AMDVLK:
//  *   api1: 10000 frames in 14.921859 s => 1.492186 ms/frame (670.16 fps)
//  *         render: 0.110603 ms, upload: 0.114412 ms, download: 0.115375 ms
//  *   api2: 10000 frames in 4.667386 s => 0.466739 ms/frame (2142.53 fps)
//  *         render: 0.030781 ms, upload: 0.000000 ms, download: 0.075237 ms
//  *
//  * You can see that AMDVLK is still better at doing texture streaming than
//  * RADV - this is because as of writing RADV still does not support
//  * asynchronous texture queues / DMA engine transfers. If we disable the
//  * `async_transfer` option with AMDVLK we get this:
//  *
//  *   api1: 10000 frames in 16.087723 s => 1.608772 ms/frame (621.59 fps)
//  *         render: 0.111154 ms, upload: 0.122476 ms, download: 0.133162 ms
//  *   api2: 10000 frames in 6.344959 s => 0.634496 ms/frame (1576.05 fps)
//  *         render: 0.031307 ms, upload: 0.000000 ms, download: 0.083520 ms
//  *
//  * License: CC0 / Public Domain
//  */

// #include <assert.h>
// #include <stdlib.h>
// #include <stdbool.h>
// #include <stdio.h>
// #include <string.h>

// #include "common.h"
// #include "pl_clock.h"
// #include "pl_thread.h"

// #ifdef _WIN32
// #include <windows.h>
// #endif

// #include <libplacebo/dispatch.h>
// #include <libplacebo/shaders/sampling.h>
// #include <libplacebo/utils/upload.h>
// #include <libplacebo/vulkan.h>

// ///////////////////////
// /// API definitions ///
// ///////////////////////

// // Stuff that would be common to each API

// void *init(void);
// void uninit(void *priv);

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

// #define MAX_PLANES 4

// struct image
// {
//     int width, height;
//     int num_planes;
//     struct plane planes[MAX_PLANES];

//     // For API #2, the associated mapped buffer (if any)
//     struct api2_buf *associated_buf;
// };

// // Example API design #1: synchronous, blocking, double-copy (bad!)
// //
// // In this API, `api1_filter` must immediately return with the new data.
// // This prevents parallelism on the GPU and should be avoided if possible,
// // but sometimes that's what you have to work with. So this is what it
// // would look like.
// //
// // Also, let's assume this API design reconfigures the filter chain (using
// // a blank `proxy` image every time the image format or dimensions change,
// // and doesn't expect us to fail due to format mismatches or resource
// // exhaustion afterwards.

// bool api1_reconfig(void *priv, const struct image *proxy);
// bool api1_filter(void *priv, struct image *dst, struct image *src);

// // Example API design #2: asynchronous, streaming, queued, zero-copy (good!)
// //
// // In this API, `api2_process` will run by the calling code every so often
// // (e.g. when new data is available or expected). This function has access
// // to non-blocking functions `get_image` and `put_image` that interface
// // with the video filtering engine's internal queueing system.
// //
// // This API is also designed to feed multiple frames ahead of time, i.e.
// // it will feed us as many frames as it can while we're still returning
// // `API2_WANT_MORE`. To drain the filter chain, it would continue running
// // the process function until `API2_HAVE_MORE` is no longer present
// // in the output.
// //
// // This API is also designed to do zero-copy where possible. When it wants
// // to create a data buffer of a given size, it will call our function
// // `api2_alloc` which will return a buffer that we can process directly.
// // We can use this to do zero-copy uploading to the GPU, by creating
// // host-visible persistently mapped buffers. In order to prevent the video
// // filtering system from re-using our buffers while copies are happening, we
// // use special functions `image_lock` and `image_unlock` to increase a
// // refcount on the image's backing storage. (As is typical of such APIs)
// //
// // Finally, this API is designed to be fully dynamic: The image parameters
// // could change at any time, and we must be equipped to handle that.

// enum api2_status
// {
//     // Negative values are used to signal error conditions
//     API2_ERR_FMT = -2,     // incompatible / unsupported format
//     API2_ERR_UNKNOWN = -1, // some other error happened
//     API2_OK = 0,           // no error, no status - everything's good

//     // Positive values represent a mask of status conditions
//     API2_WANT_MORE = (1 << 0), // we want more frames, please feed some more!
//     API2_HAVE_MORE = (1 << 1), // we have more frames but they're not ready
// };

// enum api2_status api2_process(void *priv);

// // Functions for creating persistently mapped buffers
// struct api2_buf
// {
//     void *data;
//     size_t size;
//     void *priv;
// };

// bool api2_alloc(void *priv, size_t size, struct api2_buf *out);
// void api2_free(void *priv, const struct api2_buf *buf);

// // These functions are provided by the API. The exact details of how images
// // are enqueued, dequeued and locked are not really important here, so just
// // do something unrealistic but simple to demonstrate with.
// struct image *get_image(void);
// void put_image(struct image *img);
// void image_lock(struct image *img);
// void image_unlock(struct image *img);

// /////////////////////////////////
// /// libplacebo implementation ///
// /////////////////////////////////

// // For API #2:
// #define PARALLELISM 8

// struct entry
// {
//     pl_buf buf; // to stream the download
//     pl_tex tex_in[MAX_PLANES];
//     pl_tex tex_out[MAX_PLANES];
//     struct image image;

//     // For entries that are associated with a held image, so we can unlock them
//     // as soon as possible
//     struct image *held_image;
//     pl_buf held_buf;
// };

// // For both APIs:
// struct priv
// {
//     pl_log log;
//     pl_vulkan vk;
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

//     // API #2: A ring buffer of textures/buffers for streaming
//     int idx_in;  // points the next free entry
//     int idx_out; // points to the first entry still in progress
//     struct entry entries[PARALLELISM];
// };

// void *init(void)
// {
//     struct priv *p = (priv *)calloc(1, sizeof(struct priv));
//     if (!p)
//         return NULL;
//     pl_log_params params{
//         .log_cb = pl_log_simple,
//         .log_level = PL_LOG_INFO,
//     };

//     p->log = pl_log_create(PL_API_VER, &params);

//     pl_vulkan_params vkParams = {
//         // Note: This is for API #2. In API #1 you could just pass params=NULL
//         // and it wouldn't really matter much.
//         .async_transfer = true,
//         .async_compute = true,
//         .queue_count = PARALLELISM,
//     };
//     p->vk = pl_vulkan_create(p->log, &vkParams);

//     if (!p->vk)
//     {
//         fprintf(stderr, "Failed creating vulkan context\n");
//         goto error;
//     }

//     // Give this a shorter name for convenience
//     p->gpu = p->vk->gpu;

//     p->dp = pl_dispatch_create(p->log, p->gpu);
//     if (!p->dp)
//     {
//         fprintf(stderr, "Failed creating shader dispatch object\n");
//         goto error;
//     }

//     p->render_timer = pl_timer_create(p->gpu);
//     p->upload_timer = pl_timer_create(p->gpu);
//     p->download_timer = pl_timer_create(p->gpu);

//     return p;

// error:
//     uninit(p);
//     return NULL;
// }

// void uninit(void *p_)
// {
//     struct priv *p = (priv *)p_;

//     // API #1
//     for (int i = 0; i < MAX_PLANES; i++)
//     {
//         pl_tex_destroy(p->gpu, &p->tex_in[i]);
//         pl_tex_destroy(p->gpu, &p->tex_out[i]);
//     }

//     // API #2
//     for (int i = 0; i < PARALLELISM; i++)
//     {
//         pl_buf_destroy(p->gpu, &p->entries[i].buf);
//         for (int j = 0; j < MAX_PLANES; j++)
//         {
//             pl_tex_destroy(p->gpu, &p->entries[i].tex_in[j]);
//             pl_tex_destroy(p->gpu, &p->entries[i].tex_out[j]);
//         }
//         if (p->entries[i].held_image)
//             image_unlock(p->entries[i].held_image);
//     }

//     pl_timer_destroy(p->gpu, &p->render_timer);
//     pl_timer_destroy(p->gpu, &p->upload_timer);
//     pl_timer_destroy(p->gpu, &p->download_timer);

//     pl_shader_obj_destroy(&p->dither_state);
//     pl_dispatch_destroy(&p->dp);
//     pl_vulkan_destroy(&p->vk);
//     pl_log_destroy(&p->log);

//     free(p);
// }

// // Helper function to set up the `pl_plane_data` struct from the image params
// static void setup_plane_data(const struct image *img,
//                              struct pl_plane_data out[MAX_PLANES])
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

// static bool do_plane(struct priv *p, pl_tex dst, pl_tex src)
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

// static void check_timers(struct priv *p)
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

// // API #1 implementation:
// //
// // In this design, we will create all GPU resources inside `reconfig`, based on
// // the texture format configured from the proxy image. This will avoid failing
// // later on due to e.g. resource exhaustion or texture format mismatch, and
// // thereby falls within the intended semantics of this style of API.

// bool api1_reconfig(void *p_, const struct image *proxy)
// {
//     struct priv *p = (priv *)p_;
//     struct pl_plane_data data[MAX_PLANES];
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

// bool api1_filter(void *p_, struct image *dst, struct image *src)
// {
//     struct priv *p = (priv *)p_;
//     struct pl_plane_data data[MAX_PLANES];
//     setup_plane_data(src, data);
//     pl_tex_transfer_params tfParams;

//     // Upload planes
//     for (int i = 0; i < src->num_planes; i++)
//     {
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
//         if (!do_plane(p, p->tex_out[i], p->tex_in[i]))
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

//     check_timers(p);
//     return true;
// }

// // API #2 implementation:
// //
// // In this implementation we maintain a queue (implemented as ring buffer)
// // of "work entries", which are isolated structs that hold independent GPU
// // resources - so that the GPU has no cross-entry dependencies on any of the
// // textures or other resources. (Side note: It still has a dependency on the
// // dither state, but this is just a shared LUT anyway)

// // Align up to the nearest multiple of a power of two
// #define ALIGN2(x, align) (((x) + (align) - 1) & ~((align) - 1))
// ////////////////////////////////////
// /// Proof of Concept / Benchmark ///
// ////////////////////////////////////

// #define FRAMES 10000

// // Let's say we're processing a 1920x1080 4:2:0 8-bit NV12 video, arbitrarily
// // with a stride aligned to 256 bytes. (For no particular reason)
// #define TEXELSZ sizeof(uint8_t)
// #define WIDTH 1920
// #define HEIGHT 1080
// #define STRIDE (ALIGN2(WIDTH, 256) * TEXELSZ)
// // Subsampled planes
// #define SWIDTH (WIDTH >> 1)
// #define SHEIGHT (HEIGHT >> 1)
// #define SSTRIDE (ALIGN2(SWIDTH, 256) * TEXELSZ)
// // Plane offsets / sizes
// #define SIZE0 (HEIGHT * STRIDE)
// #define SIZE1 (2 * SHEIGHT * SSTRIDE)
// #define OFFSET0 0
// #define OFFSET1 SIZE0
// #define BUFSIZE (OFFSET1 + SIZE1)

// // Skeleton of an example image
// static struct image example_image = {
//     .width = WIDTH,
//     .height = HEIGHT,
//     .num_planes = 2,
//     .planes = {
//         {
//             .subx = 0,
//             .suby = 0,
//             .stride = STRIDE,
//         },
//         {
//             .subx = 1,
//             .suby = 1,
//             .stride = SSTRIDE * 2,
//         },
//     },
// };

// // API #1: Nice and simple (but slow)
// static void api1_example(void)
// {
//     struct priv *vf = (priv *)init();
//     if (!vf)
//         return;
//     example_image.planes[0].fmt = {
//         .num_comps = 1,
//         .bitdepth = 8 * TEXELSZ};
//     example_image.planes[1].fmt = {
//         .num_comps = 2,
//         .bitdepth = 8 * TEXELSZ};
//     if (!api1_reconfig(vf, &example_image))
//     {
//         fprintf(stderr, "api1: Failed configuring video filter!\n");
//         return;
//     }

//     // Allocate two buffers to hold the example data, and fill the source
//     // buffer arbitrarily with a "simple" pattern. (Decoding the data into
//     // the buffer is not meant to be part of this benchmark)
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
//         if (!api1_filter(vf, &dst, &src))
//         {
//             fprintf(stderr, "api1: Failed filtering frame... aborting\n");
//             break;
//         }
//     }

//     const pl_clock_t stop = pl_clock_now();
//     const float secs = pl_clock_diff(stop, start);

//     printf("api1: %4u frames in %1.6f s => %2.6f ms/frame (%5.2f fps)\n",
//            frames, secs, 1000 * secs / frames, frames / secs);

//     if (vf->render_count)
//     {
//         printf("      render: %f ms, upload: %f ms, download: %f ms\n",
//                1e-6 * vf->render_sum / vf->render_count,
//                vf->upload_count ? (1e-6 * vf->upload_sum / vf->upload_count) : 0.0,
//                vf->download_count ? (1e-6 * vf->download_sum / vf->download_count) : 0.0);
//     }

// done:
//     free(srcbuf);
//     free(dstbuf);
//     uninit(vf);
// }

// #define POOLSIZE (PARALLELISM + 1)
// static struct image images[POOLSIZE] = {0};
// static int refcount[POOLSIZE] = {0};
// void image_unlock(struct image *img)
// {
//     int index = img - images;
//     refcount[index]--;
// }

// // int main(void)
// // {
// //     printf("Running benchmarks...\n");
// //     api1_example();
// //     return 0;
// // }

// 一些main.cpp里不要的接口

// bool yuv2nvFrame(pl_renderer render, AVFrame *avframe, pl_gpu gpu)
// {
//     pl_fmt txtfmt = *gpu->formats;
//     std::cout
//         << "[GPU Limits]\n "
//         << "max_tex_1d_dim = " << gpu->limits.max_tex_1d_dim << std::endl
//         << "max_tex_2d_dim = " << gpu->limits.max_tex_2d_dim << std::endl
//         << "[GPU Formats]\n ";
//     for (int n = 0; n < gpu->num_formats; n++)
//     {
//         pl_fmt fmt = gpu->formats[n];
//         std::cout << fmt->name << std::endl;
//     }

//     pl_frame yuvFrame;
//     pl_frame nv12Frame;
//     // pl_frame_from_avframe(&yuvFrame, avframe); // 无法直接分配纹理
//     pl_tex_params params = {
//         .w = avframe->width,
//         .h = avframe->height,
//         .d = 0,
//         .format = pl_fmt_from_avformat(gpu, (AVPixelFormat)avframe->format),
//         .sampleable = true,
//         .renderable = true,
//         .storable = false,
//         .host_writable = true,
//     };
//     pl_tex tex_in[4];
//     int planes = av_pix_fmt_count_planes((AVPixelFormat)avframe->format);
//     // std::cout << "[TEX Params]\nplanes is " << planes << std::endl
//     //           << "tex params.w is " << params.w << std::endl
//     //           << "tex params.h is " << params.h << std::endl
//     //           << "tex params.format->name is " << params.format->name << std::endl
//     //           << "pl_tex_create start\n";
//     pl_tex_transfer_params tsParams;
//     tsParams.buf = nullptr;
//     tsParams.timer = pl_timer_create(gpu);

//     for (int i = 0; i < planes; i++)
//     {
//         params.w = avframe->linesize[i];
//         params.h = (i == 0) ? avframe->height : avframe->height / 2;
//         // params.initial_data = avframe->data[i];
//         std::cout << "Creating tex for plane " << i
//                   << " with w " << params.w
//                   << " with h " << params.h << std::endl;

//         tex_in[i] = pl_tex_create(gpu, &params);

//         if (tex_in[i])
//             std::cout << "pl_tex_create index is " << i << ", ptr is " << tex_in[i] << " end\n";
//         std::cout << "[RC TEST]" << std::endl
//                   << " tex id: " << tex_in[i]->shared_mem.handle.fd << std::endl
//                   << " tex handle: " << tex_in[i]->shared_mem.handle.handle << std::endl
//                   << " parent: " << tex_in[i]->parent << std::endl;
//         tsParams.tex = tex_in[i];
//         tsParams.row_pitch = 3840;
//         tsParams.depth_pitch = 3840 * 2160;
//         tsParams.ptr = avframe->data[i];
//         // tsParams.rc = {
//         //     .x0 = 0,
//         //     .y0 = 0,
//         //     .x1 = params.w,
//         //     .y1 = params.h,
//         //     .z1 = 0,
//         // };
//         // std::cout << "[RC TEST]" << std::endl
//         //           << " tex id: " << tex_in[i]->shared_mem.handle.fd << std::endl;
//         //   << " row_pitch " << tsParams.row_pitch << std::endl
//         //   << " tex texel_align " << tex_in[i]->params.format->texel_align << std::endl
//         //   << " with tsParams.rc.x0 " << tsParams.rc.x0 << std::endl
//         //   << " with tsParams.rc.x1 " << tsParams.rc.x1 << std::endl
//         //   << " with tsParams.rc.y0 " << tsParams.rc.y0 << std::endl
//         //   << " with tsParams.rc.y1 " << tsParams.rc.y1 << std::endl;
//         // bool status = pl_tex_upload(gpu, &tsParams);
//         pl_map_avframe(gpu, &yuvFrame, tex_in, avframe);
//         std::cout << "[RC TEST]" << std::endl
//                   << " tex id: " << tex_in[i]->shared_mem.handle.fd << std::endl
//                   << " yuvFrame : " << tex_in[i]->shared_mem.handle.fd << std::endl;

//         // std::cout << "pl_tex_upload status is " << status << "\n";
//     }
//     // pl_map_avframe(gpu, &yuvFrame, tex_in, avframe);
//     // std::cout << "[AV2PL]" << std::endl
//     //           << "pl_map_avframe status is " << std::endl
//     //           << pl_map_avframe(gpu, &yuvFrame, tex_in, avframe)
//     //           << std::endl;

//     pl_frame_from_avframe(&nv12Frame, avframe);

//     bool ret = pl_render_image(render, &yuvFrame, &nv12Frame, &pl_render_default_params);
//     std::cout << "pl_render_image ret is " << ret << "\n";
//     pl_unmap_avframe(gpu, &yuvFrame);
//     pl_unmap_avframe(gpu, &nv12Frame);
//     // av_frame_free(&avframeOut);
//     return ret;
// }

// int _apply_dovi = 0;
// int _apply_filmgrain = 0;
// pl_queue _queue;
// static bool map_frame(pl_gpu gpu, pl_tex *tex, const struct pl_source_frame *src, struct pl_frame *out)
// {
//     AVFrame *avframe = (AVFrame *)src->frame_data;
//     pl_avframe_params params = {
//         .frame = avframe,
//         .tex = tex,
//         .map_dovi = _apply_dovi,
//     };
//     std::cout << "[MAPFRAME]pl_map_avframe_ex start" << std::endl;
//     bool ok = pl_map_avframe_ex(gpu, out, &params);
//     std::cout << "[MAPFRAME]pl_map_avframe_ex status" << ok << std::endl;
//     if (!_apply_filmgrain)
//         out->film_grain.type = PL_FILM_GRAIN_NONE;
//     av_frame_free(&avframe);
//     return ok;
// }

// static void unmap_frame(pl_gpu gpu, struct pl_frame *frame, const struct pl_source_frame *src)
// {
//     pl_unmap_avframe(gpu, frame);
// }

// static void discard_frame(const struct pl_source_frame *src)
// {
//     AVFrame *avframe = (AVFrame *)src->frame_data;
//     av_frame_free(&avframe);
// }

// static pl_fmt pl_fmt_from_avformat(pl_gpu gpu, enum AVPixelFormat av_fmt)
// {
//     switch (av_fmt)
//     {
//     case AV_PIX_FMT_YUV420P:
//         return pl_find_fmt(gpu, PL_FMT_UNORM, 1, 8, 0,
//                            (pl_fmt_caps)(PL_FMT_CAP_LINEAR | PL_FMT_CAP_RENDERABLE | PL_FMT_CAP_SAMPLEABLE | PL_FMT_CAP_HOST_READABLE));
//     case AV_PIX_FMT_NV12:
//         return pl_find_fmt(gpu, PL_FMT_UNORM, 2, 8, 0,
//                            (pl_fmt_caps)(PL_FMT_CAP_LINEAR | PL_FMT_CAP_SAMPLEABLE | PL_FMT_CAP_RENDERABLE));
//     case AV_PIX_FMT_RGBA:
//         return pl_find_fmt(gpu, PL_FMT_UNORM, 4, 8, 0,
//                            PL_FMT_CAP_LINEAR);
//     // 其他格式映射...
//     default:
//         return NULL;
//     }
// }

// int plQueuePush(AVFrame *frame)
// {
//     AVRational tb = AVRational{1, 60};
//     // std::cout << "[MAPFRAME]pl_queue_push start, frame->time_base.num = " << frame->time_base.num << ", den = " << frame->time_base.den << std::endl;
//     pl_source_frame params = pl_source_frame{
//         .pts = frame->pts * av_q2d(tb),
//         .duration = frame->duration * av_q2d(tb),
//         .first_field = pl_field_from_avframe(frame),
//         .frame_data = frame,
//         .map = map_frame,
//         .unmap = unmap_frame,
//         .discard = discard_frame,
//     };
//     pl_queue_push(_queue, &params);
// }

// int plQueuePull(AVFrame *frame)
// {
//     // 从队列中拉取帧（会隐式调用map_frame）struct
//     pl_frame_mix mix;
//     // pl_frame mapped_frame;
//     AVRational tb = AVRational{1, 60};
//     pl_queue_params params = pl_queue_params{
//         .pts = frame->pts * av_q2d(tb),
//         // .radius = pl_frame_mix_radius(&s->opts->params),
//         .vsync_duration = av_q2d(av_inv_q(tb)),
//     };
//     pl_queue_update(_queue, &mix, &params);
// }

// int main1()
// {

//     // readAvFrame(VIDEOFILE, nullptr, pl_gpu());
//     Display *display = XOpenDisplay(nullptr);
//     int screen = DefaultScreen(display);
//     Window root = RootWindow(display, screen);
//     int attrs[] = {
//         GLX_RGBA,
//         GLX_DEPTH_SIZE, 24,
//         GLX_DOUBLEBUFFER,
//         None};
//     XVisualInfo *vi = glXChooseVisual(display, screen, attrs);
//     Colormap cmap = XCreateColormap(display, root, vi->visual, AllocNone);
//     XSetWindowAttributes swa;
//     swa.colormap = cmap;
//     swa.event_mask = ExposureMask | KeyPressMask;
//     Window win = XCreateWindow(display, root, 0, 0, WIDTH, HEIGHT, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
//     XMapWindow(display, win);
//     XStoreName(display, win, "GLX Window");
//     GLXContext glxCtx = glXCreateContext(display, vi, nullptr, GL_TRUE);
//     GLXContextData ctxData = {
//         .display = display,
//         .drb = win,
//         .ctx = glxCtx,
//     };
//     // glXMakeContextCurrent(ctxData.display, ctxData.drb, ctxData.drb, ctxData.ctx);
//     if (!glXMakeCurrent(ctxData.display, ctxData.drb, ctxData.ctx))
//     {
//         std::cerr << "Failed to make GLX context current" << std::endl;
//         return -1;
//     }
//     pl_log_params logParams{
//         .log_cb = myLogCallback,
//         .log_level = PL_LOG_DEBUG,
//     };
//     pl_log log = pl_log_create(PL_API_VER, &logParams);
//     // pl_log log = pl_test_logger();
//     // 初始化libplacebo OpenGL参数
//     pl_opengl_params glParams = {};
//     glParams.get_proc_addr = get_proc_addr; // 设置OpenGL函数加载器
//     glParams.debug = true;                  // 开启条设计模式，方便捕获OpenGL错误
//     glParams.allow_software = false;        // 不允许使用软件加速，确保使用硬件加速
//     glParams.no_compute = false;            // 不适用计算着色器
//     glParams.max_glsl_version = 0;          // 设置最大GLSL版本【0：没有限制】
//     // EGL参数：如果使用EGL管理上下文则设置，不用就设为null
//     glParams.egl_context = nullptr;
//     glParams.egl_display = nullptr;
//     // 设置上下文回调
//     glParams.make_current = make_current;
//     glParams.release_current = release_current;
//     glParams.priv = &ctxData; // 这里可以传递相关数据，通常为上下文和显示连接
//                               // 初始化libplacebo GPU设备

//     pl_opengl gl = pl_opengl_create(log, &glParams);
//     if (!gl)
//     {
//         std::cout << "pl_opengl_create failed.\n";
//     }
//     pl_gpu gpu = gl->gpu;
//     pl_renderer rder = pl_renderer_create(log, gpu);

//     // readAvFrame(VIDEOFILE);

//     std::cout << VIDEOFILE << " file find end ... \n";
//     pl_renderer_destroy(&rder);
//     pl_gpu_flush(gpu);
//     pl_gpu_finish(gpu);
//     return 0;
// }

// #include <unistd.h>
// #include "pl_clock.h"
// static void pl_log_timestamp(void *stream, enum pl_log_level level, const char *msg)
// {
//     static char letter[] = {
//         [PL_LOG_FATAL] = 'f',
//         [PL_LOG_ERR] = 'e',
//         [PL_LOG_WARN] = 'w',
//         [PL_LOG_INFO] = 'i',
//         [PL_LOG_DEBUG] = 'd',
//         [PL_LOG_TRACE] = 't',
//     };
//     // Log time relative to the first message
//     static pl_clock_t base = 0;
//     if (!base)
//         base = pl_clock_now();
//     double secs = pl_clock_diff(pl_clock_now(), base);
//     printf("[%2.3f][%c] %s\n", secs, letter[level], msg);
//     if (level <= PL_LOG_WARN)
//     {
//         // duplicate warnings/errors to stderr
//         fprintf(stderr, "[%2.3f][%c] %s\n", secs, letter[level], msg);
//         fflush(stderr);
//     }
// }

// static inline pl_log pl_test_logger(void)
// {
//     setbuf(stdout, NULL);
//     setbuf(stderr, NULL);
//     pl_log_params params = {
//         .log_cb = isatty(fileno(stdout)) ? pl_log_color : pl_log_timestamp,
//         .log_level = PL_LOG_DEBUG,
//     };
//     return pl_log_create(PL_API_VER, &params);
// }

// pl_gpu getOpenglGpu()
// {
//     pl_log log = pl_test_logger();
//     pl_opengl_params params = {
//         .get_proc_addr = (pl_voidfunc_t (*)(const char *))eglGetProcAddress,
//         .max_glsl_version = egl_vers[i].glsl_ver,
//         .egl_display = dpy,
//         .egl_context = egl,
//         .debug = true,
//     };
//     pl_opengl gl = pl_opengl_create(log, pl_opengl_params(
//                                                  .get_proc_addr = (pl_voidfunc_t (*)(const char *))eglGetProcAddress,
//                                                  .max_glsl_version = egl_vers[i]
//                                                  .glsl_ver,
//                                                  .debug = true,
//                                                  .egl_display = dpy,
//                                                  .egl_context = egl,
// #ifdef CI_ALLOW_SW
//                                                  .allow_software = true,
// #endif
//                                              ));
// }

// void yuv420p2nv12(pl_renderer render, pl_gpu gpu, int w, int h, uint8_t *yuvdata)
// {
//     // 创建YUV420P纹理
//     pl_fmt yuvFmt = pl_find_fmt(gpu, PL_FMT_UNORM, 3, 8, 8, PL_FMT_CAP_SAMPLEABLE);
//     pl_fmt nvFmt = pl_find_fmt(gpu, PL_FMT_UNORM, 2, 8, 8, (pl_fmt_caps)(PL_FMT_CAP_RENDERABLE | PL_FMT_CAP_STORABLE));

//     std::cout << "find fmt is" << yuvFmt->name << std::endl;
//     std::cout << "find fmt is" << nvFmt->name << std::endl;
//     pl_tex_params tParams = {};
//     tParams.w = w;
//     tParams.h = h;
//     tParams.d = 0;
//     tParams.format = yuvFmt; // pl_find_named_fmt(gpu, "yuv420p"); // pl_find_fmt(gpu, PL_FMT_UNORM, 3,2);
//     tParams.sampleable = true;
//     // tParams.renderable = false;
//     // tParams.storable = false;
//     // tParams.blit_dst = false;
//     // tParams.blit_src = false;
//     tParams.host_writable = true;
//     // tParams.host_readable = true;
//     std::cout << "yuvTex pl_tex_create start" << std::endl;
//     pl_tex yuvTex = pl_tex_create(gpu, &tParams);
//     std::cout << "yuvTex pl_tex_create end" << std::endl;
//     // 创建NV12输出纹理
//     tParams.format = nvFmt; // pl_find_named_fmt(gpu, "nv12");
//     tParams.host_readable = true;
//     std::cout << "tex_nv12 pl_tex_create start" << std::endl;
//     pl_tex tex_nv12 = pl_tex_create(gpu, &tParams);

//     // 上传Y、U、V数据到子纹理
//     int ySize = w * h;
//     int uSize = w * h / 4;
//     pl_buf_t buffer;
//     buffer.data = yuvdata;
//     pl_tex_transfer_params tsParams;
//     tsParams.buf = nullptr;
//     tsParams.tex = yuvTex->planes[0];
//     tsParams.ptr = yuvdata;
//     pl_tex_upload(gpu, &tsParams);
//     tsParams.tex = yuvTex->planes[1];
//     tsParams.ptr = yuvdata + w * h;
//     pl_tex_upload(gpu, &tsParams);
//     tsParams.tex = yuvTex->planes[2];
//     tsParams.ptr = yuvdata + w * h + (w * h / 4);
//     pl_tex_upload(gpu, &tsParams);

//     // 图像转化

//     pl_frame srcFrame;
//     pl_frame dstFrame;
//     srcFrame.num_planes = 3;
//     srcFrame.planes[0].texture = yuvTex->planes[0];
//     srcFrame.planes[1].texture = yuvTex->planes[1];
//     srcFrame.planes[2].texture = yuvTex->planes[2];
//     dstFrame.num_planes = 2;
//     dstFrame.planes[0].texture = tex_nv12->planes[0];
//     dstFrame.planes[1].texture = tex_nv12->planes[1];
//     bool success = pl_render_image(render, &srcFrame, &dstFrame, &pl_render_default_params);
//     // 此时texnv12已经包含NV12格式的图像，可以下载导出

//     uint8_t *outData = new uint8_t(w * h * 3 / 2);
//     tsParams.tex = tex_nv12->planes[0];
//     tsParams.ptr = outData;
//     pl_tex_download(gpu, &tsParams);
//     tsParams.tex = tex_nv12->planes[1];
//     tsParams.ptr = outData + w * h;
//     pl_tex_download(gpu, &tsParams);

//     // 清理
//     pl_tex_destroy(gpu, &yuvTex);
//     pl_tex_destroy(gpu, &tex_nv12);
// }
