#pragma once
#include "common.hpp"
#include <png.h>

inline void setRGB(png_byte* ptr, float val)
{
    int v = (int)(val * 767);
    if (v < 0)
        v = 0;
    if (v > 767)
        v = 767;
    int offset = v % 256;

    if (v < 256) {
        ptr[0] = 0;
        ptr[1] = 0;
        ptr[2] = offset;
    } else if (v < 512) {
        ptr[0] = 0;
        ptr[1] = offset;
        ptr[2] = 255 - offset;
    } else {
        ptr[0] = offset;
        ptr[1] = 255 - offset;
        ptr[2] = 0;
    }
}

int writeImage(const char* filename, int width, int height, char* buffer, char* title)
{
    int code = 0;
    FILE* fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep row = NULL;

    // Open file for writing (binary mode)
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        code = 1;
        goto finalise;
    }

    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
        goto finalise;
    }

    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
        goto finalise;
    }

    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
        goto finalise;
    }

    png_init_io(png_ptr, fp);

    // Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, info_ptr, width, height,
        8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // Set title
    if (title != NULL) {
        png_text title_text;
        title_text.compression = PNG_TEXT_COMPRESSION_NONE;
        title_text.key = "Title";
        title_text.text = title;
        png_set_text(png_ptr, info_ptr, &title_text, 1);
    }

    png_write_info(png_ptr, info_ptr);

    // Allocate memory for one row (3 bytes per pixel - RGB)
    row = (png_bytep)malloc(3 * width * sizeof(png_byte));

    // Write image data
    int x, y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            row[x * 3 + 0] = buffer[(height - y - 1) * width * 3 + x * 3 + 2];
            row[x * 3 + 1] = buffer[(height - y - 1) * width * 3 + x * 3 + 1];
            row[x * 3 + 2] = buffer[(height - y - 1) * width * 3 + x * 3 + 0];
        }
        png_write_row(png_ptr, row);
    }

    // End write
    png_write_end(png_ptr, NULL);

finalise:
    if (fp != NULL)
        fclose(fp);
    if (info_ptr != NULL)
        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL)
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (row != NULL)
        free(row);

    return code;
}

void screenShotPNG(const char* file, short W, short H)
{
    char* pixel_data = (char*)malloc(3 * W * H * sizeof(char));
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, W, H, GL_BGR, GL_UNSIGNED_BYTE, pixel_data);
    std::string str = "title";
    char* cstr = new char[str.length() + 1];
    strcpy(cstr, str.c_str());
    writeImage(file, W, H, pixel_data, cstr);
    free(pixel_data);
    delete[] cstr;
}