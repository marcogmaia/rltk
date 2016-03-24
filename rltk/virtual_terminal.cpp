#include "virtual_terminal.hpp"
#include "texture_resources.hpp"
#include <algorithm>
#include <stdexcept>

namespace rltk {

void virtual_terminal::resize_pixels(const int width, const int height) {
	int w = width/font->character_size.first;
	int h = height/font->character_size.second;
	resize_chars(w,h);
}

void virtual_terminal::resize_chars(const int width, const int height) {
	const int num_chars = width*height;
	buffer.resize(num_chars);
	term_width = width;
	term_height = height;
	backing.create(term_width * font->character_size.first, term_height * font->character_size.second);

	// Build the vertex buffer
	vertices.setPrimitiveType(sf::Quads);
	if (has_background) {
		vertices.resize(width * height * 8);
	} else {
		vertices.resize(width * height * 4);		
	}

	if (has_background) {
		const int idx_half = width * height * 4;
		const int font_width = font->character_size.first;
		const int font_height = font->character_size.second;

		const int space_x = (219 % 16) * font_width;
		const int space_y = (219 / 16) * font_height;

		for (int y=0; y<height; ++y) {
			for (int x=0; x<width; ++x) {
				const int idx = ((y*width) + x)*4;
				const int idx2 = idx + idx_half;
				vertices[idx].position = sf::Vector2f(x * font->character_size.first, y * font->character_size.second);
				vertices[idx+1].position = sf::Vector2f((x+1) * font->character_size.first, y * font->character_size.second);
				vertices[idx+2].position = sf::Vector2f((x+1) * font->character_size.first, (y+1) * font->character_size.second);
				vertices[idx+3].position = sf::Vector2f(x * font->character_size.first, (y+1) * font->character_size.second);

				vertices[idx].texCoords = sf::Vector2f( space_x, space_y );
				vertices[idx+1].texCoords = sf::Vector2f( space_x + font_width, space_y );
				vertices[idx+2].texCoords = sf::Vector2f( space_x + font_width, space_y + font_height );
				vertices[idx+3].texCoords = sf::Vector2f( space_x, space_y + font_height );

				vertices[idx2].position = sf::Vector2f(x * font->character_size.first, y * font->character_size.second);
				vertices[idx2+1].position = sf::Vector2f((x+1) * font->character_size.first, y * font->character_size.second);
				vertices[idx2+2].position = sf::Vector2f((x+1) * font->character_size.first, (y+1) * font->character_size.second);
				vertices[idx2+3].position = sf::Vector2f(x * font->character_size.first, (y+1) * font->character_size.second);
			}
		}
	} else {
		const int font_width = font->character_size.first;
		const int font_height = font->character_size.second;

		const int space_x = (219 % 16) * font_width;
		const int space_y = (219 / 16) * font_height;

		for (int y=0; y<height; ++y) {
			for (int x=0; x<width; ++x) {
				const int idx = ((y*width) + x)*4;
				vertices[idx].position = sf::Vector2f(x * font->character_size.first, y * font->character_size.second);
				vertices[idx+1].position = sf::Vector2f((x+1) * font->character_size.first, y * font->character_size.second);
				vertices[idx+2].position = sf::Vector2f((x+1) * font->character_size.first, (y+1) * font->character_size.second);
				vertices[idx+3].position = sf::Vector2f(x * font->character_size.first, (y+1) * font->character_size.second);
			}
		}
	}
}

void virtual_terminal::clear() {
	std::fill(buffer.begin(), buffer.end(), vchar{ 32, {255,255,255}, {0,0,0} });
}

void virtual_terminal::clear(const vchar &target) {
	std::fill(buffer.begin(), buffer.end(), target);
}

void virtual_terminal::set_char(const int idx, const vchar &target) {
	buffer[idx] = target;
}

void virtual_terminal::print(const int x, const int y, const std::string &s, const color_t &fg, const color_t &bg) {
	int idx = at(x,y);
	for (std::size_t i=0; i<s.size(); ++i) {
		buffer[idx] = { s[i], fg, bg };
		++idx;
	}
}

void virtual_terminal::render(sf::RenderWindow &window) {
	if (!visible) return;

	if (font == nullptr) {
		throw std::runtime_error("Font not loaded: " + font_tag);
	}
	if (tex == nullptr) {
		tex = get_texture(font->texture_tag);
	}
	const int font_width = font->character_size.first;
	const int font_height = font->character_size.second;

	const int space_x = (219 % 16) * font_width;
	const int space_y = (219 / 16) * font_height;

	backing.clear();

	int vertex_idx = term_height * term_width * 4;
	if (!has_background) vertex_idx = 0;
	int bg_idx = 0;
	int idx=0;
	for (int y=0; y<term_height; ++y) {
		for (int x=0; x<term_width; ++x) {
			const vchar target = buffer[idx];
			const int texture_x = (target.glyph % 16) * font_width;
			const int texture_y = (target.glyph / 16) * font_height;

			if (has_background) {
				sf::Color bgsfml = color_to_sfml(target.background);
				vertices[bg_idx].color = bgsfml;
				vertices[bg_idx+1].color = bgsfml;
				vertices[bg_idx+2].color = bgsfml;
				vertices[bg_idx+3].color = bgsfml;
			}

			vertices[vertex_idx].texCoords = sf::Vector2f( texture_x, texture_y );
			vertices[vertex_idx+1].texCoords = sf::Vector2f( texture_x + font_width, texture_y );
			vertices[vertex_idx+2].texCoords = sf::Vector2f( texture_x + font_width, texture_y + font_height );
			vertices[vertex_idx+3].texCoords = sf::Vector2f( texture_x, texture_y + font_height );

			sf::Color fgsfml = color_to_sfml(target.foreground);
			vertices[vertex_idx].color = fgsfml;
			vertices[vertex_idx+1].color = fgsfml;
			vertices[vertex_idx+2].color = fgsfml;
			vertices[vertex_idx+3].color = fgsfml;

			vertex_idx += 4;
			bg_idx += 4;
			++idx;
		}
	}
	backing.draw(vertices, tex);

	backing.display();
	sf::Sprite compositor(backing.getTexture());
	compositor.move(offset_x, offset_y);
	compositor.setColor(sf::Color(tint.r, tint.g, tint.b, alpha));
	window.draw(sf::Sprite(compositor));
}

}
