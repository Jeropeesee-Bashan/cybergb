/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Andrey Kulenko
 */

#define TITLE "CybeRGB Visualizer"
#define VERSION "0.0.1"
#define IDENTIFIER "su.kulenko.cybergb"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <popt.h>

#include "geometry.h"
#include "io.h"

struct app_data {
  struct geometry geometry;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Thread *io_task;
  Uint32 updated_type;
  int w, h;
};

static int parse_arguments(int argc, char *argv[], Uint32 *leds,
                           Uint32 *segments, int *w, int *h, Uint8 bg[3])
{
  const char *extra;
  char *opt_color = NULL;
  int opt_w = *w;
  int opt_h = *h;
  int opt_leds = (int)*leds;
  int opt_segments = (int)*segments;
  int opt_help = 0;

  struct poptOption options[] = {
      {"width", 'W', POPT_ARG_INT, &opt_w, 0, "Window width (>= 4)", "PIXELS"},
      {"height", 'H', POPT_ARG_INT, &opt_h, 0, "Window height (>= 4)",
       "PIXELS"},
      {"leds", 'n', POPT_ARG_INT, &opt_leds, 0, "Number of LEDs (> 0)",
       "COUNT"},
      {"segments", 's', POPT_ARG_INT, &opt_segments, 0,
       "Segments per LED (>= 3)", "COUNT"},
      {"color", 'c', POPT_ARG_STRING, &opt_color, 0, "Background color #RRGGBB",
       "#RRGGBB"},
      {"help", 'h', POPT_ARG_NONE, &opt_help, 0, "Show this help message",
       NULL},
      POPT_TABLEEND};

  poptContext ctx = poptGetContext(NULL, argc, (const char **)argv, options, 0);
  poptSetOtherOptionHelp(ctx, "[OPTIONS]");

  int rc;
  while ((rc = poptGetNextOpt(ctx)) >= 0)
    ;

  if (rc < -1) {
    SDL_SetError("%s: %s\n", poptBadOption(ctx, POPT_BADOPTION_NOALIAS),
                 poptStrerror(rc));
    poptFreeContext(ctx);
    return 2;
  }

  extra = poptGetArg(ctx);
  if (extra) {
    SDL_SetError("unexpected argument: %s\n", extra);
    poptFreeContext(ctx);
    return 2;
  }

  if (opt_help) {
    poptPrintHelp(ctx, stderr, 0);
    poptFreeContext(ctx);
    return 1;
  }

  if (opt_w != 0 && opt_w < 4) {
    SDL_SetError("width must be at least 4 (got %d)\n", opt_w);
    poptFreeContext(ctx);
    return 2;
  }
  if (opt_h != 0 && opt_h < 4) {
    SDL_SetError("height must be at least 4 (got %d)\n", opt_h);
    poptFreeContext(ctx);
    return 2;
  }
  if (opt_leds < 1) {
    SDL_SetError("number of LEDs must be positive (got %d)\n", opt_leds);
    poptFreeContext(ctx);
    return 2;
  }
  if (opt_segments < 3) {
    SDL_SetError("number of segments must be at least 3 (got %d)\n",
                 opt_segments);
    poptFreeContext(ctx);
    return 2;
  }

  if (opt_color) {
    unsigned int r, g, b;
    if (strlen(opt_color) != 7 || opt_color[0] != '#' ||
        SDL_sscanf(opt_color + 1, "%02x%02x%02x", &r, &g, &b) != 3) {
      SDL_SetError("invalid color format '%s' (expected #RRGGBB)\n", opt_color);
      poptFreeContext(ctx);
      return 2;
    }
    bg[0] = (Uint8)r;
    bg[1] = (Uint8)g;
    bg[2] = (Uint8)b;
  }

  *w = opt_w;
  *h = opt_h;
  *leds = (Uint32)opt_leds;
  *segments = (Uint32)opt_segments;

  poptFreeContext(ctx);
  return 0;
}

static int init(struct app_data *data, int argc, char *argv[])
{
  SDL_Event updated;
  Uint8 bg[3];

  SDL_SetAppMetadata(TITLE, VERSION, IDENTIFIER);
  if (SDL_Init(SDL_INIT_VIDEO) == false) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize SDL: %s",
                 SDL_GetError());
    return 2;
  }

  SDL_zero(updated);
  data->updated_type = SDL_RegisterEvents(2);
  bg[0] = bg[1] = bg[2] = 0x1b;
  data->geometry.leds = 30;
  data->geometry.segments = 8;
  data->w = 0;
  data->h = 0;

  switch (parse_arguments(argc, argv, &data->geometry.leds,
                          &data->geometry.segments, &data->w, &data->h, bg)) {
  case 1:
    return 1;
  case 2:
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to parse arguments: %s",
                 SDL_GetError());
    return 2;
  default:
    break;
  }

  if (data->w == 0 || data->h == 0) {
    const SDL_DisplayMode *mode;
    SDL_DisplayID id;

    id = SDL_GetPrimaryDisplay();
    if (id == 0) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get primary display: %s",
                   SDL_GetError());
      return 2;
    }

    mode = SDL_GetCurrentDisplayMode(id);
    if (mode == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to get current display mode: %s", SDL_GetError());
      return 2;
    }
    data->w = mode->w * 2 / 3;
    data->h = mode->h * 2 / 3;
  }

  data->window =
      SDL_CreateWindow(TITLE, data->w, data->h, SDL_WINDOW_RESIZABLE);
  if (data->window == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create window: %s",
                 SDL_GetError());
    return 2;
  }

  data->renderer = SDL_CreateRenderer(data->window, NULL);
  if (data->renderer == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create renderer: %s",
                 SDL_GetError());
    return 2;
  }

  if (geometry_init(&data->geometry)) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize geometry: %s",
                 SDL_GetError());
    return 2;
  }

  updated.type = data->updated_type + 1;
  updated.user.data1 = (void *)(uintptr_t)data->geometry.leds;
  SDL_PushEvent(&updated);

  data->io_task = io_task(&data->geometry, data->updated_type);
  if (data->io_task == NULL) {
    SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create I/O task: %s",
                 SDL_GetError());
    return 2;
  }

  SDL_SetRenderDrawColor(data->renderer, bg[0], bg[1], bg[2], SDL_ALPHA_OPAQUE);

  return 0;
}

static int loop(struct app_data *data)
{
  SDL_Event event, updated;

  SDL_zero(updated);

  SDL_WaitEvent(&event);

  switch (event.type) {
  case SDL_EVENT_QUIT:
    return 1;
  case SDL_EVENT_WINDOW_RESIZED:
    data->w = (int)(intptr_t)event.window.data1;
    data->h = (int)(intptr_t)event.window.data2;
    /* fallthrough */
  case SDL_EVENT_WINDOW_EXPOSED:
    updated.type = data->updated_type + 1;
    updated.user.data1 = (void *)(uintptr_t)data->geometry.leds;
    SDL_PushEvent(&updated);
    break;
  default:
    break;
  }

  if (event.type == data->updated_type) {
    geometry_set_colors(&data->geometry, event.user.data1);
    SDL_free(event.user.data1);
  } else if (event.type == data->updated_type + 1) {
    geometry_set_positions(&data->geometry, (Uint32)(uintptr_t)event.user.data1,
                           data->geometry.segments, data->w, data->h);
  }

  if (event.type == data->updated_type ||
      event.type == data->updated_type + 1) {
    SDL_RenderClear(data->renderer);
    SDL_RenderGeometry(data->renderer, NULL, data->geometry.strip,
                       data->geometry.leds * (data->geometry.segments + 1),
                       data->geometry.indices,
                       data->geometry.leds * data->geometry.segments * 3);
    SDL_RenderPresent(data->renderer);
  }

  return 0;
}

static int deinit(struct app_data *data, int result)
{
  int io_task_status = 0;

  io_cancel();
  if (data->io_task != NULL)
    SDL_WaitThread(data->io_task, &io_task_status);
  geometry_deinit(&data->geometry);
  if (data->renderer != NULL)
    SDL_DestroyRenderer(data->renderer);
  if (data->window != NULL)
    SDL_DestroyWindow(data->window);
  SDL_Quit();

  return result != 0 && io_task_status;
}

int main(int argc, char *argv[])
{
  struct app_data data;
  int result;

  SDL_zero(data);

  result = init(&data, argc, argv);
  while (!result)
    result = loop(&data);
  return deinit(&data, result);
}
