/******************************************************************************

  Curse of War -- Real Time Strategy Game for Linux.
  Copyright (C) 2013 Alexey Nikolaev.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
******************************************************************************/

#include <stdlib.h>
#include <time.h>
#include "SDL.h"

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 480 

#include "grid.h"
#include "common.h"
#include "state.h"
#include "output-sdl.h"
#include "main-common.h"

/* delay in milliseconds */
#define TIME_DELAY 10

void run(struct state *st, struct ui *ui, SDL_Surface *screen, SDL_Surface *tileset){
  /* tile variation */
  int tile_variant[MAX_WIDTH][MAX_HEIGHT];
  int pop_variant[MAX_WIDTH][MAX_HEIGHT];
  int i, j;
  for (i=0; i<MAX_WIDTH; ++i)
    for (j=0; j<MAX_HEIGHT; ++j) {
      tile_variant[i][j] = rand();
      pop_variant[i][j] = rand();
    }

  int finished = 0;
  SDL_Event event;
  int previous_time = SDL_GetTicks();

  int k = 0;
  while (!finished){

    int time = SDL_GetTicks();

    if (time - previous_time >= TIME_DELAY) {

      previous_time = previous_time + TIME_DELAY;
      k++;
      if (k>=1600) k=0;

      char c = '\0';
      while (!finished && SDL_PollEvent(&event)){
        switch (event.type){
          case SDL_KEYDOWN:
            c = '\0';
            switch (event.key.keysym.sym) {
              case SDLK_LEFT: c = 'h'; break;
              case SDLK_RIGHT: c = 'l'; break;
              case SDLK_UP: c = 'k'; break;
              case SDLK_DOWN: c = 'j'; break;
              case SDLK_q: c = 'q'; break;
              case SDLK_SPACE: c = ' '; break;
              default:
                if ( (event.key.keysym.unicode & 0xFF80) == 0 ) {
                  c = event.key.keysym.unicode & 0x7F;
                }
            }
            finished = singleplayer_process_input(st, ui, c);
            break;
        }
      }

      int slowdown = game_slowdown(st->speed);
      if (k % slowdown == 0 && st->speed != sp_pause) { 
        kings_move(st);
        simulate(st);
      }

      if (k % 5 == 0) {
        output_sdl(tileset, screen, st, ui, tile_variant, pop_variant, k);
        SDL_Flip(screen);
      }
    }
    else{
      SDL_Delay(TIME_DELAY/2);
    }
  }
}

int main(int argc, char *argv[]) {

  srand(time(NULL));

  if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  atexit(SDL_Quit);

  SDL_Surface *screen;
  screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 16, SDL_DOUBLEBUF);
  if (screen == NULL) {
    fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
    exit(1);
  }

  SDL_EnableUNICODE(1);
  SDL_EnableKeyRepeat(300, 30);

  /* Load Image */
  SDL_Surface *tileset;
  SDL_Surface *temp;
   
  temp = SDL_LoadBMP("../draw/tileset.bmp");
  if (temp == NULL) {
      printf("Unable to load bitmap: %s\n", SDL_GetError());
        return 1;
  }

  SDL_LockSurface(temp);
  Uint32 colorkey = getpixel(temp, temp->w-1, temp->h-1);
  SDL_UnlockSurface(temp);
  colorkey = SDL_MapRGB(temp->format, 0, 255, 255);

  if (SDL_SetColorKey(temp, SDL_SRCCOLORKEY | SDL_RLEACCEL, colorkey) == -1) {
    fprintf(stderr, "Warning: colorkey will not be used, reason: %s\n", SDL_GetError());
  }
   
  tileset = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);

  /* Read command line arguments */
  struct basic_options op;
  struct multi_options mop;
  if (get_options(argc, argv, &op, &mop) == 1) return 1;

  /* Initialize the state */
  struct state st;
  struct ui ui;

  state_init(&st, &op, &mop);
  ui_init(&st, &ui);

  /* Run the game */
  run(&st, &ui, screen, tileset);

  /* Finalize */
  SDL_FreeSurface(tileset);
  
  if (!mop.multiplayer_flag || mop.server_flag)
    printf ("Random seed was %i\n", st.map_seed);
  
  free(mop.val_client_port);
  free(mop.val_server_addr);
  free(mop.val_server_port);
  return 0;
}
