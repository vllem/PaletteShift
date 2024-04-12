#include <libavfilter/avfilter.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    unsigned char r, g, b;
} RGBColor;

typedef struct {
    const AVClass *class;
    char *palette_string;
    RGBColor palette[256];
    int palette_size;
} ApplyPaletteContext;

AVFILTER_DEFINE_CLASS(applypalette);

static double color_distance(RGBColor c1, RGBColor c2) {
    return sqrt(pow(c1.r - c2.r, 2) + pow(c1.g - c2.g, 2) + pow(c1.b - c2.b, 2));
}

// Parse the palette string into RGBColor structs
static int parse_palette(ApplyPaletteContext *ctx) {
    char *arg = ctx->palette_string;
    int r, g, b;
    int index = 0;

    while (*arg && sscanf(arg, "%d,%d,%d;", &r, &g, &b) == 3) {
        ctx->palette[index++] = (RGBColor){ .r = (unsigned char)r, .g = (unsigned char)g, .b = (unsigned char)b };
        if (index >= 256) break; 
        arg = strchr(arg, ';') + 1; 
    }
    ctx->palette_size = index;
    return 0;
}

static int query_formats(AVFilterContext *ctx) {
    static const enum AVPixelFormat pix_fmts[] = { AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE };
    AVFilterFormats *fmts = avfilter_make_format_list(pix_fmts);
    return avfilter_set_common_formats(ctx, fmts);
}

// The filter frame function processes each frame
static int filter_frame(AVFilterLink *inlink, AVFrame *frame) {
    ApplyPaletteContext *ctx = inlink->dst->priv;

    if (frame->format != AV_PIX_FMT_RGB24) {
        av_log(ctx, AV_LOG_ERROR, "Unsupported pixel format. Please use RGB24.\n");
        return AVERROR(EINVAL);
    }

    int width = frame->width;
    int height = frame->height;
    int line_size = frame->linesize[0];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int pixel_pos = y * line_size + x * 3;
            RGBColor pixel_color = {
                frame->data[0][pixel_pos],
                frame->data[0][pixel_pos+1],
                frame->data[0][pixel_pos+2]
            };

            double min_distance = INFINITY;
            int nearest_color_index = 0;
            for (int i = 0; i < ctx->palette_size; i++) {
                double dist = color_distance(pixel_color, ctx->palette[i]);
                if (dist < min_distance) {
                    min_distance = dist;
                    nearest_color_index = i;
                }
            }

            RGBColor closest_color = ctx->palette[nearest_color_index];
            frame->data[0][pixel_pos] = closest_color.r;
            frame->data[0][pixel_pos+1] = closest_color.g;
            frame->data[0][pixel_pos+2] = closest_color.b;
        }
    }

    return ff_filter_frame(inlink->dst->outputs[0], frame);
}

// AVOptions available for this filter
static const AVOption applypalette_options[] = {
    { "palette", "set color palette as 'r,g,b;r,g,b;...'", offsetof(ApplyPaletteContext, palette_string), AV_OPT_TYPE_STRING, {.str = NULL}, 0, 0, AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_FILTERING_PARAM },
    { NULL }
};

static const AVFilterPad applypalette_inputs[] = {
    {
        .name         = "default",
        .type         = AVMEDIA_TYPE_VIDEO,
        .filter_frame = filter_frame,
    },
    { NULL }
};

static const AVFilterPad applypalette_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
    },
    { NULL }
};

AVFilter ff_vf_applypalette = {
    .name          = "paletteshift",
    .description   = NULL_IF_CONFIG_SMALL("Apply a specified palette to video frames."),
    .priv_size     = sizeof(ApplyPaletteContext),
    .priv_class    = &applypalette_class,
    .query_formats = query_formats,
    .inputs        = applypalette_inputs,
    .outputs       = applypalette_outputs,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
};