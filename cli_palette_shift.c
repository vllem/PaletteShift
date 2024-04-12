#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
    unsigned char r, g, b;
} RGBColor;

int read_palette(const char* filename, RGBColor palette[], int max_colors) {
    if (max_colors > 256) {
        max_colors = 256;
    }

    FILE* file = fopen(filename, "r");
    if (!file) return 0;

    char line[8];
    int count = 0;
    int line_count = 0;
    while (fscanf(file, "%7s", line) == 1) {
        line_count++;

        if (line_count > 256) {
            fclose(file);
            printf("There should not be more than 256 colors in your palette\n");
            return -1; 
        }

        unsigned int r, g, b;
        if (sscanf(line, "#%02x%02x%02x", &r, &g, &b) == 3) {
            if (count < max_colors) {
                palette[count++] = (RGBColor){r, g, b};
            }
        }
    }

    fclose(file);

    return count;
}

void apply_palette(unsigned char* data, int width, int height, RGBColor palette[], int palette_size, int pixel_width, int pixel_height) {
    #pragma omp parallel for collapse(2)
    for (int block_i = 0; block_i < height; block_i += pixel_height) {
        for (int block_j = 0; block_j < width; block_j += pixel_width) {
            double total_color[3] = {0, 0, 0};
            int count = 0;

            for (int i = 0; i < pixel_height && block_i + i < height; ++i) {
                for (int j = 0; j < pixel_width && block_j + j < width; ++j) {
                    int idx = ((block_i + i) * width + (block_j + j)) * 3;
                    total_color[0] += data[idx];
                    total_color[1] += data[idx + 1];
                    total_color[2] += data[idx + 2];
                    count++;
                }
            }

            if (count > 0) {
                for (int i = 0; i < 3; ++i) {
                    total_color[i] /= count;
                }
            }

            int closest = 0;
            double closest_distance = 1e9;
            for (int j = 0; j < palette_size; j++) {
                double dr = total_color[0] - palette[j].r;
                double dg = total_color[1] - palette[j].g;
                double db = total_color[2] - palette[j].b;
                double distance = dr * dr + dg * dg + db * db;

                if (distance < closest_distance) {
                    closest_distance = distance;
                    closest = j;
                }
            }

            for (int i = 0; i < pixel_height && block_i + i < height; ++i) {
                for (int j = 0; j < pixel_width && block_j + j < width; ++j) {
                    int idx = ((block_i + i) * width + (block_j + j)) * 3;
                    data[idx] = palette[closest].r;
                    data[idx + 1] = palette[closest].g;
                    data[idx + 2] = palette[closest].b;
                }
            }
        }
    }
}


int main(int argc, char** argv) {
    if (argc != 6) {
        printf("Usage: <input_image> <hex_palette_file> <pixel_height> <pixel_width> <output_image.bmp>\n");
        return 1;
    }

    int width, height, channels;
    unsigned char* img = stbi_load(argv[1], &width, &height, &channels, 3);
    if (!img) {
        printf("Error loading image\n");
        return 1;
    }

    RGBColor palette[256];
    int palette_size = read_palette(argv[2], palette, 256);
    if (palette_size == 0) {
        printf("Error loading palette\n");
        stbi_image_free(img);
        return 1;
    }
    int pixel_height = atoi(argv[3]);
    int pixel_width = atoi(argv[4]);

    if (pixel_height < 1 || pixel_width < 1) {
        printf("Block height and block width must be greater than zero\n");
        return 1;
    }
        
    apply_palette(img, width, height, palette, palette_size, pixel_width, pixel_height);
    
    stbi_write_bmp(argv[5], width, height, 3, img);

    stbi_image_free(img);
    return 0;
}
