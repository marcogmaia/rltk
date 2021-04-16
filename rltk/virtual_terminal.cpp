#include "virtual_terminal.hpp"
#include "texture_resources.hpp"
#include "scaling.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace rltk {

void virtual_terminal::resize_pixels(const int width,
                                     const int height) noexcept {
    int w
        = static_cast<int>(width / (font->character_size.first * scale_factor));
    int h = static_cast<int>(height
                             / (font->character_size.second * scale_factor));
    resize_chars(w, h);
}

void virtual_terminal::resize_chars(const int width,
                                    const int height) noexcept {
    dirty               = true;
    const int num_chars = width * (height + 1);
    buffer.resize(num_chars);
    term_width  = width;
    term_height = height;
    backing.create(term_width * font->character_size.first,
                   term_height * font->character_size.second);

    // Build the vertex buffer
    vertices.setPrimitiveType(sf::Quads);
    if(has_background) {
        vertices.resize(width * height * 8);
    } else {
        vertices.resize(width * height * 4);
    }

    if(has_background) {
        const int idx_half    = width * height * 4;
        const int font_width  = font->character_size.first;
        const int font_height = font->character_size.second;

        const int space_x = (219 % 16) * font_width;
        const int space_y = (219 / 16) * font_height;

        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const int idx          = ((y * width) + x) * 4;
                const int idx2         = idx + idx_half;
                vertices[idx].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx + 1].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx + 2].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));
                vertices[idx + 3].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));

                vertices[idx].texCoords = sf::Vector2f(
                    static_cast<float>(space_x), static_cast<float>(space_y));
                vertices[idx + 1].texCoords
                    = sf::Vector2f(static_cast<float>(space_x + font_width),
                                   static_cast<float>(space_y));
                vertices[idx + 2].texCoords
                    = sf::Vector2f(static_cast<float>(space_x + font_width),
                                   static_cast<float>(space_y + font_height));
                vertices[idx + 3].texCoords
                    = sf::Vector2f(static_cast<float>(space_x),
                                   static_cast<float>(space_y + font_height));

                vertices[idx2].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx2 + 1].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx2 + 2].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));
                vertices[idx2 + 3].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));
            }
        }
    } else {
        for(int y = 0; y < height; ++y) {
            for(int x = 0; x < width; ++x) {
                const int idx          = ((y * width) + x) * 4;
                vertices[idx].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx + 1].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>(y * font->character_size.second));
                vertices[idx + 2].position = sf::Vector2f(
                    static_cast<float>((x + 1) * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));
                vertices[idx + 3].position = sf::Vector2f(
                    static_cast<float>(x * font->character_size.first),
                    static_cast<float>((y + 1) * font->character_size.second));
            }
        }
    }
}

void virtual_terminal::clear() noexcept {
    dirty = true;
    std::fill(buffer.begin(), buffer.end(),
              vchar{32, {255, 255, 255}, {0, 0, 0}});
}

void virtual_terminal::clear(const vchar& target) noexcept {
    dirty = true;
    std::fill(buffer.begin(), buffer.end(), target);
}

void virtual_terminal::set_char(const int idx, const vchar& target) noexcept {
    dirty = true;
    if(!(idx < 0 || idx > static_cast<int>(buffer.size()))) {
        buffer[idx] = target;
    }
}

void virtual_terminal::print(const int x, const int y, const std::string& s,
                             const color_t& fg, const color_t& bg) noexcept {
    int idx = at(x, y);
    for(std::size_t i = 0; i < s.size(); ++i) {
        buffer[idx] = {s[i], fg, bg};
        ++idx;
    }
}

void virtual_terminal::print_center(const int y, const std::string& s,
                                    const color_t& fg,
                                    const color_t& bg) noexcept {
    print(static_cast<int>((term_width / 2) - (s.size() / 2)), y, s, fg, bg);
}

void virtual_terminal::box(const int x, const int y, const int w, const int h,
                           const color_t& fg, const color_t& bg,
                           bool double_lines) noexcept {
    // std::cout << h << "\n";
    for(int i = 1; i < w; ++i) {
        if(!double_lines) {
            set_char(x + i, y, vchar{196, fg, bg});
            set_char(x + i, y + h, vchar{196, fg, bg});
        } else {
            set_char(x + i, y, vchar{205, fg, bg});
            set_char(x + i, y + h, vchar{205, fg, bg});
        }
    }

    for(int i = 1; i < h; ++i) {
        if(!double_lines) {
            set_char(x, y + i, vchar{179, fg, bg});
            set_char(x + w, y + i, vchar{179, fg, bg});
        } else {
            set_char(x, y + i, vchar{186, fg, bg});
            set_char(x + w, y + i, vchar{186, fg, bg});
        }
    }

    if(!double_lines) {
        set_char(x, y, vchar{218, fg, bg});
        set_char(x + w, y, vchar{191, fg, bg});
        set_char(x, y + h, vchar{192, fg, bg});
        set_char(x + w, y + h, vchar{217, fg, bg});
    } else {
        set_char(x, y, vchar{201, fg, bg});
        set_char(x + w, y, vchar{187, fg, bg});
        set_char(x, y + h, vchar{200, fg, bg});
        set_char(x + w, y + h, vchar{188, fg, bg});
    }
}

void virtual_terminal::fill(const int left_x, const int top_y,
                            const int right_x, const int bottom_y,
                            const uint8_t glyph, const color_t& fg,
                            const color_t& bg) noexcept {
    for(int y = top_y; y < bottom_y; ++y) {
        for(int x = left_x; x < right_x; ++x) {
            set_char(x, y, vchar{glyph, fg, bg});
        }
    }
}


void virtual_terminal::draw_sprite(const int x, const int y,
                                   xp::rex_sprite& sprite) {
    const int width  = sprite.get_width();
    const int height = sprite.get_height();

    for(int layer = 0; layer < sprite.get_num_layers(); ++layer) {
        for(int Y = 0; Y < height; ++Y) {
            const int screen_y = y + Y;
            for(int X = 0; X < width; ++X) {
                const int screen_x  = x + X;
                const vchar* output = sprite.get_tile(layer, X, Y);
                if(output != nullptr && !xp::is_transparent(output))
                    set_char(screen_x, screen_y, *output);
            }
        }
    }
}

void virtual_terminal::render(sf::RenderWindow& window) {
    if(!visible)
        return;

    if(dirty) {
        if(font == nullptr) {
            throw std::runtime_error("Font not loaded: " + font_tag);
        }
        if(tex == nullptr) {
            tex = get_texture(font->texture_tag);
        }
        const int font_width  = font->character_size.first;
        const int font_height = font->character_size.second;

        int vertex_idx = term_height * term_width * 4;
        if(!has_background)
            vertex_idx = 0;
        int bg_idx = 0;
        int idx    = 0;
        for(int y = 0; y < term_height; ++y) {
            for(int x = 0; x < term_width; ++x) {
                const vchar target  = buffer[idx];
                const int texture_x = (target.glyph % 16) * font_width;
                const int texture_y = (target.glyph / 16) * font_height;

                if(has_background) {
                    sf::Color bgsfml = color_to_sfml(target.background);
                    if(alpha == 0
                       || (target.background.r == 0 && target.background.g == 0
                           && target.background.b == 0)) {
                        bgsfml.a = 0;
                    } else {
                        bgsfml.a = alpha;
                    }
                    vertices[bg_idx].color     = bgsfml;
                    vertices[bg_idx + 1].color = bgsfml;
                    vertices[bg_idx + 2].color = bgsfml;
                    vertices[bg_idx + 3].color = bgsfml;
                }

                vertices[vertex_idx].texCoords
                    = sf::Vector2f(static_cast<float>(texture_x),
                                   static_cast<float>(texture_y));
                vertices[vertex_idx + 1].texCoords
                    = sf::Vector2f(static_cast<float>(texture_x + font_width),
                                   static_cast<float>(texture_y));
                vertices[vertex_idx + 2].texCoords
                    = sf::Vector2f(static_cast<float>(texture_x + font_width),
                                   static_cast<float>(texture_y + font_height));
                vertices[vertex_idx + 3].texCoords
                    = sf::Vector2f(static_cast<float>(texture_x),
                                   static_cast<float>(texture_y + font_height));

                sf::Color fgsfml           = color_to_sfml(target.foreground);
                vertices[vertex_idx].color = fgsfml;
                vertices[vertex_idx + 1].color = fgsfml;
                vertices[vertex_idx + 2].color = fgsfml;
                vertices[vertex_idx + 3].color = fgsfml;

                vertex_idx += 4;
                bg_idx += 4;
                ++idx;
            }
        }
        backing.clear(sf::Color(0, 0, 0, 0));
        // backing.applyBlendMode(blendmode);
        backing.draw(vertices, tex);
    }

    backing.display();
    sf::Sprite compositor(backing.getTexture());
    compositor.move(static_cast<float>(offset_x), static_cast<float>(offset_y));
    compositor.setColor(sf::Color(tint.r, tint.g, tint.b, alpha));
    compositor.scale(scale_factor, scale_factor);
    window.draw(sf::Sprite(compositor));
    dirty = false;
}

}  // namespace rltk
