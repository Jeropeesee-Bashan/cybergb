/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Andrey Kulenko
 */

#ifndef IO_H_
#define IO_H_

#include <SDL3/SDL.h>

#include "geometry.h"

SDL_Thread *io_task(struct geometry *geometry, Uint32 update_event_type);
void io_cancel(void);

#endif /* IO_H_ */
