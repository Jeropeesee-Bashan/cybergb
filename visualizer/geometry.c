/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Andrey Kulenko
 */

#include "geometry.h"

static SDL_Vertex *create_strip(Uint32 leds, Uint32 segments)
{
  size_t i;

  const size_t size = leds * (segments + 1);
  SDL_Vertex *const strip = SDL_malloc(size * sizeof(SDL_Vertex));

  if (strip == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  for (i = 0; i < size; i++) {
    strip[i].color.r = strip[i].color.g = strip[i].color.b = 0.0f;
    strip[i].color.a = SDL_ALPHA_OPAQUE_FLOAT;
  }

  return strip;
}

static int *create_indices(Uint32 leds, Uint32 segments)
{
  size_t i, j;

  int *const indices = SDL_malloc(leds * segments * 3 * sizeof(int));

  if (indices == NULL) {
    SDL_OutOfMemory();
    return NULL;
  }

  for (i = 0; i < leds; i++) {
    for (j = 0; j < segments; j++) {
      indices[i * segments * 3 + j * 3] = i * (segments + 1);
      indices[i * segments * 3 + j * 3 + 1] = i * (segments + 1) + j + 1;
      indices[i * segments * 3 + j * 3 + 2] =
          i * (segments + 1) + (j + 1) % segments + 1;
    }
  }

  return indices;
}

static SDL_Point position(int w, int h, int i)
{
  SDL_Point p;

  const int perimeter = 2 * (w + h);

  i %= perimeter;
  if (i < w)
    p.x = i, p.y = 0;
  else if (i < w + h)
    p.x = w, p.y = i - w;
  else if (i < w + h + w)
    p.x = w + w + h - i, p.y = h;
  else
    p.x = 0, p.y = perimeter - i;

  return p;
}

static void set_positions(SDL_Vertex *strip, Uint32 leds, Uint32 segments,
                          int w, int h)
{
  size_t i, j;

  const int sw = w * 4 / 5;
  const int sh = h * 4 / 5;
  const int sx = (w - sw) / 2;
  const int sy = (h - sh) / 2;
  const int perimeter = 2 * (sw + sh);
  const float scale = SDL_min((float)perimeter * 0.5f / (float)leds,
                              SDL_min((float)sy, (float)sx));

  for (i = 0; i < leds; i++) {
    SDL_Point p = position(sw, sh, perimeter * i / leds);

    strip[i * (segments + 1)].position.x = (float)(sx + p.x);
    strip[i * (segments + 1)].position.y = (float)(h - sy - p.y);

    for (j = 0; j < segments; j++) {
      strip[i * (segments + 1) + j + 1].position.x =
          strip[i * (segments + 1)].position.x +
          scale * SDL_cosf(2.0f * SDL_PI_F * (float)j / (float)segments);
      strip[i * (segments + 1) + j + 1].position.y =
          strip[i * (segments + 1)].position.y +
          scale * SDL_sinf(2.0f * SDL_PI_F * (float)j / (float)segments);
    }
  }
}

static void set_colors(SDL_Vertex *strip, const Uint8 *colors, Uint32 leds,
                       Uint32 segments)
{
  size_t i, j;

  for (i = 0; i < leds; i++) {
    for (j = 0; j < segments + 1; j++) {
      strip[i * (segments + 1) + j].color.r = (float)colors[i * 3] / 255.0f;
      strip[i * (segments + 1) + j].color.g = (float)colors[i * 3 + 1] / 255.0f;
      strip[i * (segments + 1) + j].color.b = (float)colors[i * 3 + 2] / 255.0f;
    }
  }
}

int geometry_init(struct geometry *geometry)
{
  SDL_Vertex *new_strip;
  int *new_indices;

  if (geometry == NULL || geometry->strip != NULL ||
      geometry->indices != NULL || geometry->leds == 0 ||
      geometry->segments < 3)
    return 1;

  new_strip = create_strip(geometry->leds, geometry->segments);
  if (new_strip == NULL)
    return 1;

  new_indices = create_indices(geometry->leds, geometry->segments);
  if (new_indices == NULL) {
    SDL_free(new_strip);
    return 1;
  }

  geometry->strip = new_strip;
  geometry->indices = new_indices;

  return 0;
}

int geometry_set_positions(struct geometry *geometry, Uint32 leds,
                           Uint32 segments, int w, int h)
{
  struct geometry new;

  if (geometry->leds == leds && geometry->segments == segments) {
    set_positions(geometry->strip, leds, segments, w, h);
    return 0;
  }

  new.leds = leds;
  new.segments = segments;
  new.strip = NULL;
  new.indices = NULL;

  if (geometry_init(&new))
    return 1;
  set_positions(new.strip, leds, segments, w, h);

  geometry_deinit(geometry);

  SDL_memcpy(geometry, &new, sizeof(struct geometry));

  return 0;
}

void geometry_set_colors(struct geometry *geometry, const Uint8 *colors)
{
  set_colors(geometry->strip, colors, geometry->leds, geometry->segments);
}

void geometry_deinit(struct geometry *geometry)
{
  if (geometry == NULL)
    return;

  SDL_free(geometry->indices);
  SDL_free(geometry->strip);
}
