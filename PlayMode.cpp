#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "load_save_png.hpp"
#include "convert_assets.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	// https://github.com/jyanln/15-466-f20-game1/blob/07d108eef55419a0ab6545aafc0a3ea924d98eef/PlayMode.cpp
	std::ifstream istrm(data_path("asset/sprite/sprites.list"));
	int tile_index = 0;
	uint32_t i = 0;
	for (std::string line; std::getline(istrm, line); i++) {
   		std::vector<uint8_t> raw_data;
   		std::string asset_loc = data_path(std::string("asset/sprite/") + line + ASSET_EXTENSION);
   		std::ifstream asset_file(asset_loc, std::ios::binary);
   		read_chunk(asset_file, MAGIC, &raw_data);
   		asset_file.close();

		sprites[line] = i;

   		uint32_t data_idx = 0;
        // TODO, the number of palette is greater than 8. Acturally, I just use the first one
        if (i<8) {
            for(int j = 0; j < 4; j++) {
                ppu.palette_table[i][j].r = raw_data[data_idx++];
                ppu.palette_table[i][j].g = raw_data[data_idx++];
                ppu.palette_table[i][j].b = raw_data[data_idx++];
                ppu.palette_table[i][j].a = raw_data[data_idx++];
            }
        }
   		while(data_idx < raw_data.size()) {
   		    for(int j = 0; j < 8; j++) {
   		        ppu.tile_table[tile_index].bit0[j] = raw_data[data_idx++];
   		    }
   		    for(int j = 0; j < 8; j++) {
   		        ppu.tile_table[tile_index].bit1[j] = raw_data[data_idx++];
   		    }
   		    tile_index++;
   		}
	}
	istrm.close();

    /**
     * TODO, not practical
     * 0-7 => 1-8
     * 8 unkonow
     * 9 bomb
     * 10 unknown
     * 11 close
     */ 
    sprites["bomb"] = 9;
    sprites["close"] = 8;
    sprites["open"] = 11;

}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (left.pressed) player_at.x -= 1;
	if (right.pressed) player_at.x += 1;
	if (down.pressed) player_at.y -= 1;
	if (up.pressed) player_at.y += 1;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	if (new_game) {
		std::srand(std::time(nullptr));
		for (uint32_t i = 0; i < PPU466::BackgroundWidth * PPU466::BackgroundHeight; i++) {
            // TODO, do not hardcoded the level
			if ((1 + std::rand()/((RAND_MAX + 1u)/100)) < 20) {
				bomb_map.push_back("bomb");
			} else {
				bomb_map.push_back("");
            }
		}
		for (uint32_t i = 0; i < PPU466::BackgroundHeight; i++) {
			for (uint32_t j = 0; j < PPU466::BackgroundWidth; j++) {
				if (bomb_map[i*PPU466::BackgroundWidth + j] != "bomb") {
					int bomb_num = 0;
					if (i>0 && j>0 && bomb_map[(i-1)*PPU466::BackgroundWidth + j-1] == "bomb") {
						bomb_num += 1;
					}
					if (i>0 && bomb_map[(i-1)*PPU466::BackgroundWidth + j] == "bomb") {
						bomb_num += 1;
					}
					if (i>0 && j<PPU466::BackgroundWidth-1 && bomb_map[(i-1)*PPU466::BackgroundWidth + j+1] == "bomb") {
						bomb_num += 1;
					}
					if (j>0 && bomb_map[i*PPU466::BackgroundWidth + j-1] == "bomb") {
						bomb_num += 1;
					}
					if (j<PPU466::BackgroundWidth-1 && bomb_map[i*PPU466::BackgroundWidth + j+1] == "bomb") {
						bomb_num += 1;
					}
					if (i<PPU466::BackgroundHeight-1&& j>0 && bomb_map[(i+1)*PPU466::BackgroundWidth + j-1] == "bomb") {
						bomb_num += 1;
					}
					if (i<PPU466::BackgroundHeight-1&& bomb_map[(i+1)*PPU466::BackgroundWidth + j] == "bomb") {
						bomb_num += 1;
					}
					if (i<PPU466::BackgroundHeight-1&& j<PPU466::BackgroundWidth-1 && bomb_map[(i+1)*PPU466::BackgroundWidth + j+1] == "bomb") {
						bomb_num += 1;
					}
                   
					bomb_map[i*PPU466::BackgroundWidth + j] = bomb_num == 0 ? "open" : std::to_string(bomb_num);
				}
			}
		}
		new_game = false;
	}

}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---

	ppu.background_color = glm::u8vec4(63,174,0,0xff);

	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			ppu.background[x+PPU466::BackgroundWidth*y] = sprites[bomb_map[x+PPU466::BackgroundWidth*y]]; // all tiles use the same palette
		}
	}

	//background scroll:
	ppu.background_position.x = int32_t(-0.5f * player_at.x);
	ppu.background_position.y = int32_t(-0.5f * player_at.y);

	//player sprite:
	ppu.sprites[0].x = int32_t(player_at.x);
	ppu.sprites[0].y = int32_t(player_at.y);
	ppu.sprites[0].index = sprites["bomb"]; // use bomb to represent the current position
	ppu.sprites[0].attributes = 0;

	//--- actually draw ---
	ppu.draw(drawable_size);
}
