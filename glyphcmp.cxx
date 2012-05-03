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

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

// TODO: functions that create and free images might be nice.
typedef struct {
  size_t real_width;
  size_t real_height;
  size_t virt_width;
  size_t virt_height;
  uint8_t* p;
} image_data;

void
debug_write_pgm(image_data* image, char* path) {
  FILE* f = fopen(path, "wb");
  if (NULL == f) {
    // fopen failed
  }
  fprintf(f, "P5\n%u %u\n%u\n", image->virt_width,
    image->virt_height, 255);
  fwrite(image->p, image->virt_width * image->virt_height, 1, f);
  fclose(f);
}

double
create_difference(image_data* left, image_data* right) {
  image_data* d = (image_data*)malloc(sizeof(image_data));

  if (NULL == d) {
    // malloc failed
  }

  d->real_width = d->virt_width = 2
    + MAX(left->real_width, right->real_width);
  d->real_height = d->virt_height = 2
    + MAX(left->real_height, right->real_height);

  size_t image_data_byte_count =
    sizeof(*(d->p)) * d->virt_width * d->virt_height;

  d->p = (uint8_t*)malloc(image_data_byte_count);

  if (NULL == d->p) {
    // malloc failed
  }

  memset(d->p, 0xff, image_data_byte_count);

  size_t sum = 0;

  for (size_t y = 0; y < d->real_height - 1; ++y) {
    for (size_t x = 0; x < d->real_width - 1; ++x) {
      // This could use XOR instead
      d->p[(y + 1) * d->real_width + (x + 1)] = 
        abs(left->p[y * left->virt_width + x] -
            right->p[y * right->virt_width + x]);
    }
  }

  // debug_write_pgm(d, "debug.pgm");

  for (size_t y = 1; y < d->real_height - 1; ++y) {
    for (size_t x = 1; x < d->real_width - 1; ++x) {
      bool has_black_neighbour = 
        !d->p[(y - 1) * d->real_width + (x + 0)] ||
        !d->p[(y + 1) * d->real_width + (x + 0)] ||
        !d->p[(y + 0) * d->real_width + (x - 1)] ||
        !d->p[(y + 0) * d->real_width + (x + 1)];
      if (!d->p[(y) * d->real_width + (x)] || has_black_neighbour)
        sum++;
    }
  }

  double area = (d->real_width - 2) * (d->real_height - 2);
  double result = (double)sum/area;
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

  if (argc != 2) {
    fprintf(stderr, "Usage: %s /path/to/file.pgm (P5 portable graymap)\n", argv[0]);
    exit(0);
  }

  char* file_path = argv[1];

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

    uint8_t* pixels = (uint8_t*)
      malloc(width * height * sizeof(*pixels));

    if (NULL == pixels) {
      // malloc failed
      break;
    }

    if (width * height != fread(pixels, sizeof(*pixels), width * height, f)) {
      // This would fail if the pixel data is incomplete
      break;
    }

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
    images[image_count].virt_width  = width;
    images[image_count].real_height = height;
    images[image_count].virt_height = height;
    image_count++;
  }

  // This changes the pixel buffer for all images so they have the
  // same virtual dimensions, which makes some algorithms simpler.
  for (size_t ix = 0; ix < image_count; ++ix) {
    uint8_t* temp = (uint8_t*)
      malloc(sizeof(*temp) * max_width * max_height);

    if (NULL == temp) {
      // malloc failed
    }

    // Assume a white background!
    memset(temp, 0xff, sizeof(*temp) * max_width * max_height);

    images[ix].virt_width = max_width;
    images[ix].virt_height = max_height;
    for (size_t y = 0; y < images[ix].real_height; ++y) {
      for (size_t x = 0; x < images[ix].real_width; ++x) {
        temp[y * images[ix].virt_width + x] =
          images[ix].p[y * images[ix].real_width + x];
      }
    }
    free(images[ix].p);
    images[ix].p = temp;
  }

  for (size_t left = 0; left < image_count; ++left) {
    for (size_t right = left + 1; right < image_count; ++right) {
      double diff = create_difference(&images[left], &images[right]);
      printf("%u\t%u\t%f\n", left, right, diff);
    }
  }

  return EXIT_SUCCESS;
}
