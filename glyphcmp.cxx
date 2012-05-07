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
create_difference(image_data* left, image_data* right) {

  // TODO: Now that the comments refer to left and right in the spatial
  // sense, the variables should probably be renamed into A and B or so.

  double area;
  double result;
  size_t sum = 0;
  image_data* d = (image_data*)malloc(sizeof(image_data));

  if (NULL == d) {
    // malloc failed
  }

  size_t max_real_width  = MAX( left->real_width,  right->real_width  );
  size_t min_real_width  = MIN( left->real_width,  right->real_width  );
  size_t min_real_height = MIN( left->real_height, right->real_height );
  size_t max_real_height = MAX( left->real_height, right->real_height );

  d->real_width  = 2 + max_real_width;
  d->real_height = 2 + max_real_height;

  size_t image_data_byte_count =
    sizeof(*(d->p)) * d->real_width * d->real_height;

  d->p = (uint8_t*)malloc(image_data_byte_count);

  if (NULL == d->p) {
    // malloc failed
  }

  // white border left and right
  for (size_t y = 0; y < d->real_height; ++y) {
    d->p[y * d->real_width + 0] = 0xFF;
    d->p[y * d->real_width + d->real_width - 1] = 0xFF;
  }

  // white border top and bottom
  for (size_t x = 0; x < d->real_width; ++x) {
    d->p[0 * d->real_width + x] = 0xFF;
    d->p[(d->real_height - 1) * d->real_width + x] = 0xFF;
  }

  // The bottom right part covered by neither `left` nor `right`
  // unless one image fully contains the other in which case the
  // later loops will override the value again...
  for (size_t y = min_real_height; y < max_real_height; ++y) {
    for (size_t x = min_real_width; x < max_real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] = 0;
    }
  }

  // The right part of `left` that does not intersect `right`
  for (size_t y = 0; y < left->real_height; ++y) {
    for (size_t x = right->real_width; x < left->real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] =
        left->p[y * left->real_width + x] ^ 0xff;
    }
  }

  // The right part of `right` that does not intersect `left`
  for (size_t y = 0; y < right->real_height; ++y) {
    for (size_t x = left->real_width; x < right->real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] =
        right->p[y * right->real_width + x] ^ 0xff;
    }
  }

  // The bottom part of `right` that does not intersect `left`
  for (size_t y = left->real_height; y < right->real_height; ++y) {
    for (size_t x = 0; x < right->real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] =
        right->p[y * right->real_width + x] ^ 0xff;
    }
  }

  // The bottom part of `left` that does not intersect `right`
  for (size_t y = right->real_height; y < left->real_height; ++y) {
    for (size_t x = 0; x < left->real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] =
        left->p[y * left->real_width + x] ^ 0xff;
    }
  }

  // The intersection of `left` and `right`
  for (size_t y = 0; y < min_real_height; ++y) {
    for (size_t x = 0; x < min_real_width; ++x) {
      d->p[(y + 1) * d->real_width + (x + 1)] = 
        (left->p[y * left->real_width + x] ^
            right->p[y * right->real_width + x]);
    }
  }

  // debug_write_pgm(d, "debug.pgm");

  for (size_t y = 1; y < d->real_height - 1; ++y) {
    for (size_t x = 1; x < d->real_width - 1; ++x) {
      int has_black_neighbour = 
        !d->p[(y - 1) * d->real_width + (x + 0)] || // above
        !d->p[(y + 1) * d->real_width + (x + 0)] || // below
        !d->p[(y + 0) * d->real_width + (x - 1)] || // left
        !d->p[(y + 0) * d->real_width + (x + 1)];   // right
      if (!d->p[(y) * d->real_width + (x)] || has_black_neighbour)
        sum++;
    }
  }

  area = (d->real_width - 2) * (d->real_height - 2);
  result = (double)sum/area;
  free(d->p);
  free(d);
  return result;
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
      double diff = create_difference(&images[left], &images[right]);
      printf("%u\t%u\t%f\n", left, right, diff);
    }
  }

  return EXIT_SUCCESS;
}
