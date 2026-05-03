/*
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Andrey Kulenko
 */

#include "io.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <gio/gio.h>

static GInputStream *input;
static GOutputStream *output;
static GCancellable *cancellable;
static struct {
  Uint32 leds;
  Uint32 update_event_type;
} data;

static void deinit(void)
{
  g_object_unref(cancellable);
  g_object_unref(output);
  g_object_unref(input);
  cancellable = NULL;
  output = NULL;
  input = NULL;
}

#ifdef _WIN32
#include <gio/gwin32inputstream.h>
#include <gio/gwin32outputstream.h>
#include <windows.h>

static int init_streams(void)
{
  HANDLE std_input, std_output;

  std_input = GetStdHandle(STD_INPUT_HANDLE);
  if (std_input == INVALID_HANDLE_VALUE || std_input == NULL) {
    SDL_SetError("Failed to acquire standard input handle");
    return 1;
  }

  std_output = GetStdHandle(STD_OUTPUT_HANDLE);
  if (std_output == INVALID_HANDLE_VALUE || std_output == NULL) {
    SDL_SetError("Failed to acquire standard output handle");
    return 1;
  }

  input = g_win32_input_stream_new(std_input, FALSE);
  if (input == NULL) {
    SDL_SetError("Failed to create GioWin32 input stream");
    return 1;
  }

  output = g_win32_output_stream_new(std_output, FALSE);
  if (output == NULL) {
    SDL_SetError("Failed to create GioWin32 output stream");
    g_object_unref(input);
    input = NULL;
    return 1;
  }

  return 0;
}

#else
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <unistd.h>

static int init_streams(void)
{
  input = g_unix_input_stream_new(STDIN_FILENO, FALSE);
  if (input == NULL) {
    SDL_SetError("Failed to create GioUnix input stream");
    return 1;
  }

  output = g_unix_output_stream_new(STDOUT_FILENO, FALSE);
  if (output == NULL) {
    SDL_SetError("Failed to create GioUnix output stream");
    g_object_unref(input);
    input = NULL;
    return 1;
  }

  return 0;
}
#endif /* _WIN32 */

static int update_colors(SDL_Event *event)
{
  Uint8 *colors;
  size_t size, read;

  size = data.leds * 3;
  colors = SDL_malloc(size);

  if (colors == NULL)
    return 0;

  if (g_input_stream_read_all(input, colors, size, &read, cancellable, NULL) ==
          FALSE ||
      size != read) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to read color data");
    SDL_free(colors);
    return 1;
  }

  event->type = data.update_event_type;
  event->user.data1 = colors;

  return 0;
}

static int update_positions(SDL_Event *event)
{
  size_t size;

  if (g_input_stream_read_all(input, &data.leds, sizeof(data.leds), &size,
                              cancellable, NULL) == FALSE ||
      size != sizeof(data.leds)) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Failed to read LED count");
    return 1;
  }

  data.leds = SDL_Swap32LE(data.leds);
  event->type = data.update_event_type + 1;
  event->user.data1 = (void *)(uintptr_t)data.leds;

  return 0;
}

static int task(void *task_data)
{
  SDL_Event event;
  int result;

  (void)task_data;

  SDL_zero(event);

  if (g_output_stream_printf(output, NULL, cancellable, NULL, "%u\n",
                             data.leds) == FALSE) {
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Failed to write initial LED count");
    deinit();
    return 1;
  }

  result = 0;
  while (!result) {
    size_t size;
    Uint8 command;

    if (g_input_stream_read_all(input, &command, sizeof(command), &size,
                                cancellable, NULL) == FALSE ||
        size == 0)
      break;

    switch (command) {
    case 0x00:
      result = update_colors(&event);
      break;
    case 0x01:
      result = update_positions(&event);
      if (!result && g_output_stream_printf(output, NULL, cancellable, NULL,
                                            "%u\n", data.leds) == FALSE) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Failed to write updated LED count");
        deinit();
        return 1;
      }
      break;
    default:
      SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                  "Received unknown command: 0x%02X", command);
      continue;
    }

    if (!result)
      SDL_PushEvent(&event);
  }

  deinit();
  return 0;
}

SDL_Thread *io_task(struct geometry *geometry, Uint32 update_event_type)
{
  if (input != NULL || output != NULL || cancellable != NULL) {
    SDL_SetError("I/O task already initialized");
    return NULL;
  }

  if (init_streams())
    return NULL;

  cancellable = g_cancellable_new();
  if (cancellable == NULL) {
    SDL_SetError("Failed to create GCancellable");
    deinit();
    return NULL;
  }

  data.leds = geometry->leds;
  data.update_event_type = update_event_type;

  return SDL_CreateThread(task, "I/O", NULL);
}

void io_cancel(void) { g_cancellable_cancel(cancellable); }
