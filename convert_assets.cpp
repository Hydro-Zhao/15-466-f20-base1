// https://github.com/jyanln/15-466-f20-game1/

#include "convert_assets.hpp"

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <array>

#include <glm/glm.hpp>
#include "load_save_png.hpp"
#include "data_path.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"


std::vector<uint8_t> convert_spritesheet(std::string png_loc) {
    // Load png data
    glm::uvec2 size;
    std::vector<glm::u8vec4> png_data;
    try {
        std::cout << "Loading sheet at " << png_loc << std::endl;
        load_png(png_loc, &size, &png_data, UpperLeftOrigin);
    } catch(std::exception const& e) {
        std::cerr << e.what() << std::endl;
        return std::vector<uint8_t>{};
    }


    // Scan png to convert to tiles & palette
    std::array<glm::u8vec4, 4> palette_data;
    std::vector<PPU466::Tile> tile_data;

    int num_colors = 0;

    for(uint8_t index_x = 0; index_x < size.x / SPRITE_WIDTH; index_x++) {
        for(uint8_t index_y = 0; index_y < size.y / SPRITE_HEIGHT; index_y++) {
            PPU466::Tile tile;
            for(int i = 0; i < 8; i++) {
                tile.bit0[i] = 0;
                tile.bit1[i] = 0;
            }

            for(uint8_t sprite_x = 0; sprite_x < SPRITE_WIDTH; sprite_x++) {
                for(uint8_t sprite_y = 0; sprite_y < SPRITE_HEIGHT; sprite_y++) {
                    // Calculate actual indices relative to spritesheet
                    uint16_t real_x = index_x * SPRITE_WIDTH + sprite_x;
                    uint16_t real_y = index_y * SPRITE_HEIGHT + sprite_y;
                    uint16_t real_index = real_y * size.x + real_x;

                    // color equality check
                    auto color_equal = [](glm::u8vec4 c1, glm::u8vec4 c2) {
                        return c1.r == c2.r &&
                            c1.g == c2.g &&
                            c1.b == c2.b &&
                            c1.a == c2.a;
                    };

                    // Check for color in palette
                    int color = -1;
                    for(int i = 0; i < num_colors; i++) {
                        //TODO
                        if(color_equal(png_data.at(real_index), palette_data[i])) {
                            color = i;
                            break;
                        }
                    }

                    // Check if color needs to be added to palette
                    if(color == -1) {
                        // Check if we run out of colors for the palette
                        if(num_colors == 4) {
                            std::cerr << "Error: Maximum color limit reached" << std::endl;
        					return std::vector<uint8_t>{};
                        }
                        // Add new color
                        palette_data[num_colors].r = png_data.at(real_index).r;
                        palette_data[num_colors].g = png_data.at(real_index).g;
                        palette_data[num_colors].b = png_data.at(real_index).b;
                        palette_data[num_colors].a = png_data.at(real_index).a;
                        color = num_colors;
                        num_colors++;
                    }

                    // Write colors data
                    tile.bit0[SPRITE_HEIGHT - sprite_y - 1] |= (color % 2) << sprite_x;
                    tile.bit1[SPRITE_HEIGHT - sprite_y - 1] |= (color / 2) << sprite_x;
                }
            }

            tile_data.emplace_back(tile);
        }
    }

    // Write all 4 colors even if they don't exist
    std::vector<uint8_t> output_data;
    for(int i = 0; i < 4; i++) {
        output_data.emplace_back(palette_data[i].r);
        output_data.emplace_back(palette_data[i].g);
        output_data.emplace_back(palette_data[i].b);
        output_data.emplace_back(palette_data[i].a);
    }
    for(PPU466::Tile tile : tile_data) {
        for(int i = 0; i < 8; i++) {
            output_data.emplace_back(tile.bit0[i]);
        }
        for(int i = 0; i < 8; i++) {
            output_data.emplace_back(tile.bit1[i]);
        }
    }

	return output_data;
}

/**
 * this program is located at asset/sprite/
 * ./convert_assets sprites.list ../../dist/asset/sprite
 */
int main(int argc, char** argv) {
	std::ifstream istrm(argv[1]);
	if (!istrm.is_open()) {
		std::cout << "failed to open " << argv[1] << '\n';
	} else {
		for (std::string line; std::getline(istrm, line);) {
			std::vector<uint8_t> output_data = convert_spritesheet(line + ".png");
    		std::string output_loc = std::string(argv[2]) + "/" + line + ASSET_EXTENSION;
    		std::ofstream output_file(output_loc, std::ios::binary);
    		write_chunk(MAGIC, output_data, &output_file);
    		output_file.close();
		}
  	}
	istrm.close();

    return 0;
}