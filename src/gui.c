//
//   Copyright 2024 Darius Kellermann
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//

#include "nuklear_impl.h"
#include "padre.c"

int main(const int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init(): %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Window *const win =
      SDL_CreateWindow("Padre", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       640, 480, SDL_WINDOW_SHOWN);
  if (win == NULL) {
    printf("SDL_CreateWindow(): %s\n", SDL_GetError());
    SDL_Quit();
    return EXIT_FAILURE;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(
      win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    printf("SDL_CreateRenderer(): %s\n", SDL_GetError());
    SDL_DestroyWindow(win);
    SDL_Quit();
    return EXIT_FAILURE;
  }

  struct nk_context *ctx = nk_sdl_init(win, renderer);
  {
    struct nk_font_atlas *atlas;
    const struct nk_font_config config = nk_font_config(0);

    nk_sdl_font_stash_begin(&atlas);
    const struct nk_font *font = nk_font_atlas_add_default(atlas, 13, &config);
    nk_sdl_font_stash_end();
    nk_style_set_font(ctx, &font->handle);
  }

  for (int quit = 0; !quit;) {
    nk_input_begin(ctx);
    for (SDL_Event event; SDL_WaitEventTimeout(&event, 10);) {
      if (event.type == SDL_QUIT) {
        quit = 1;
      } else {
        nk_sdl_handle_event(&event);
      }
    }
    nk_input_end(ctx);
    // TODO draw here
  }

  nk_sdl_shutdown();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return EXIT_SUCCESS;
}
