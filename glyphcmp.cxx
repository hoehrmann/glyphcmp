////////////////////////////////////////////////////////////////////
//
// glyphcmp - Compare monochrome images (TODO: poor description)
//
// Copyright (c) 2012 Bjoern Hoehrmann <bjoern@hoehrmann.de>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// $Id$
//
////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <assert.h>

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

// TODO: functions that create and free images might be nice.
typedef struct {
  size_t real_width;
  size_t real_height;
  uint8_t* p;
} image_data;

void
debug_write_pgm(image_data* image, char* path) {
  FILE* f = fopen(path, "wb");
  if (NULL == f) {
    // fopen failed
  }
  fprintf(f, "P5\n%u %u\n%u\n", image->real_width,
    image->real_height, 255);
  fwrite(image->p, image->real_width * image->real_height, 1, f);
  fclose(f);
}

double
compute_similarity(size_t widthA, size_t heightA, unsigned char* pixelsA,
                   size_t widthB, size_t heightB, unsigned char* pixelsB) {

  double area;
  double result;
  size_t sum = 0;

  size_t max_real_width  = MAX(widthA, widthB);
  size_t min_real_width  = MIN(widthA, widthB);
  size_t min_real_height = MIN(heightA, heightB);
  size_t max_real_height = MAX(heightA, heightB);
  size_t widthD = 2 + max_real_width;
  size_t heightD = 2 + max_real_height;
  unsigned char* pixelsD;

  pixelsD = (uint8_t*)malloc(sizeof(*(pixelsD)) * widthD * heightD);

  if (NULL == pixelsD) {
    // malloc failed
  }

  // white border left and right
  for (size_t y = 0; y < heightD; ++y) {
    pixelsD[y * widthD + 0] = 0xFF;
    pixelsD[y * widthD + widthD - 1] = 0xFF;
  }

  // white border top and bottom
  for (size_t x = 0; x < widthD; ++x) {
    pixelsD[0 * widthD + x] = 0xFF;
    pixelsD[(heightD - 1) * widthD + x] = 0xFF;
  }

  // The bottom right part covered by neither `A` nor `B`
  // unless one image fully contains the other in which case the
  // later loops will override the value again...
  for (size_t y = min_real_height; y < max_real_height; ++y) {
    for (size_t x = min_real_width; x < max_real_width; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] = 0;
    }
  }

  // The right part of `A` that does not intersect `B`
  for (size_t y = 0; y < heightA; ++y) {
    for (size_t x = widthB; x < widthA; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] =
        pixelsA[y * widthA + x] ^ 0xff;
    }
  }

  // The right part of `B` that does not intersect `A`
  for (size_t y = 0; y < heightB; ++y) {
    for (size_t x = widthA; x < widthB; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] =
        pixelsB[y * widthB + x] ^ 0xff;
    }
  }

  // The bottom part of `B` that does not intersect `A`
  for (size_t y = heightA; y < heightB; ++y) {
    for (size_t x = 0; x < widthB; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] =
        pixelsB[y * widthB + x] ^ 0xff;
    }
  }

  // The bottom part of `A` that does not intersect `B`
  for (size_t y = heightB; y < heightA; ++y) {
    for (size_t x = 0; x < widthA; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] =
        pixelsA[y * widthA + x] ^ 0xff;
    }
  }

  // The intersection of `A` and `B`
  for (size_t y = 0; y < min_real_height; ++y) {
    for (size_t x = 0; x < min_real_width; ++x) {
      pixelsD[(y + 1) * widthD + (x + 1)] = 
        (pixelsA[y * widthA + x] ^
            pixelsB[y * widthB + x]);
    }
  }

  for (size_t y = 1; y < heightD - 1; ++y) {
    for (size_t x = 1; x < widthD - 1; ++x) {
      int has_black_neighbour = 
        !pixelsD[(y - 1) * widthD + (x + 0)] || // above
        !pixelsD[(y + 1) * widthD + (x + 0)] || // below
        !pixelsD[(y + 0) * widthD + (x - 1)] || // left
        !pixelsD[(y + 0) * widthD + (x + 1)];   // right
      if (!pixelsD[(y) * widthD + (x)] || has_black_neighbour)
        sum++;
    }
  }

  area = (widthD - 2) * (heightD - 2);
  result = (double)sum/area;
  free(pixelsD);
  return result;
}

double
compute_similarity_imgs(image_data* left, image_data* right) {
  return compute_similarity(left->real_width, left->real_height,
    left->p, right->real_width, right->real_height, right->p);
}

int
main(int argc, char *argv[]) {
  FILE* f;
  size_t max_width = 0;
  size_t max_height = 0;
  image_data* images;
  size_t image_count = 0;
  size_t image_alloc = 20000;
  char* file_path;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s /path/to/file.pgm (P5 portable graymap)\n", argv[0]);
    exit(0);
  }

  file_path = argv[1];
  f = fopen(file_path, "rb");

  if (NULL == f) {
    fprintf(stderr, "Unable to open %s\n", file_path);
    exit(1);
  }

  images = (image_data*)malloc(image_alloc * sizeof(*images));

  while (!feof(f)) {
    size_t width;
    size_t height;
    size_t maxval;
    uint8_t byte;
    uint8_t* pixels;

    // This assumes fscanf consider the right set of bytes
    // to be kind of white space allowed between elements.
    if (3 != fscanf(f, "P5%u%u%u", &width, &height, &maxval)) {
      // This would happen if there are no further images
      // or if there is junk after the last found image.
      break;
    }

    max_width = MAX(max_width, width);
    max_height = MAX(max_height, height);

    if (maxval > 255) {
      // The format allows up to 65535 but we don't
      break;
    }

    if (1 != fread(&byte, sizeof(byte), 1, f)) {
      // This would mean the image is malformed
      break;
    }

    if (byte != 0x0a && byte != 0x0d && byte != 0x09 && byte != 0x20) {
      // This would also be an error, though I am not sure this is the
      // correct set of white space characters; the documentation is
      // not clear on that...
      break;
    }

    pixels = (uint8_t*)
      malloc(width * height * sizeof(*pixels));

    if (NULL == pixels) {
      // malloc failed
      break;
    }

    if (width * height != fread(pixels, sizeof(*pixels), width * height, f)) {
      // This would fail if the pixel data is incomplete
      break;
    }

    // 

    if (image_count + 1 > image_alloc) {
      image_alloc *= 2;
      image_data* temp = (image_data*)
        realloc(images, image_alloc * sizeof(*images));

      if (NULL == temp) {
        // realloc failed
      }
      images = temp;
    }

    images[image_count].p           = pixels;
    images[image_count].real_width  = width;
    images[image_count].real_height = height;
    image_count++;
  }

  for (size_t left = 0; left < image_count; ++left) {
    for (size_t right = left + 1; right < image_count; ++right) {
      double s = compute_similarity_imgs(&images[left], &images[right]);
      printf("%u\t%u\t%f\n", left, right, s);
    }
  }

  return EXIT_SUCCESS;
}

