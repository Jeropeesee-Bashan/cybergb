/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Andrey Kulenko
 */

#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include <SDL3/SDL.h>

struct geometry {
  SDL_Vertex *strip;
  int *indices;
  Uint32 leds;
  Uint32 segments;
};

int geometry_init(struct geometry *geometry);
void geometry_set_colors(struct geometry *geometry, const Uint8 *colors);
int geometry_set_positions(struct geometry *geometry, Uint32 leds,
                           Uint32 segments, int w, int h);
void geometry_deinit(struct geometry *geometry);

#endif /* GEOMETRY_H_ */
