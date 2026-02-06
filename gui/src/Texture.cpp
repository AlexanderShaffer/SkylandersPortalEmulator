/*
 * This file is part of Skylanders Portal Emulator.
 * Copyright (C) 2026  Alexander Shaffer <alexander.shaffer.623@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

module Texture;

import <glew/glew.h>;

Texture::Image::~Image()
{
    stbi_image_free(m_ptr);
}

Texture::Image::Image(Image&& other) noexcept
{
    *this = std::move(other);
}

auto Texture::Image::operator=(Image&& other) noexcept -> Image&
{
    if (this == &other)
        return *this;

    stbi_image_free(m_ptr);
    m_ptr = nullptr;
    std::swap(m_ptr, other.m_ptr);
    return *this;
}

Texture::Loaded::Loaded(const Image& image, const ImVec2 size)
{
    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.getPtr());
    glBindTexture(GL_TEXTURE_2D, NO_TEXTURE);
}

Texture::Loaded::~Loaded()
{
    glDeleteTextures(1, &m_id);
}

Texture::Loaded::Loaded(Loaded&& other) noexcept
{
    *this = std::move(other);
}

auto Texture::Loaded::operator=(Loaded&& other) noexcept -> Loaded&
{
    if (this == &other)
        return *this;

    glDeleteTextures(1, &m_id);
    m_id = NO_TEXTURE;
    std::swap(m_id, other.m_id);
    return *this;
}

Texture::Texture(const std::filesystem::path& imagePath)
{
    int width, height;
    stbi_uc* const image{stbi_load(imagePath.string().c_str(), &width, &height, nullptr, STBI_rgb_alpha)};

    if (!image)
        throw std::runtime_error{"Failed to read an image from a file"};

    m_state.emplace<Image>(image);
    m_size = {static_cast<float>(width), static_cast<float>(height)};
}

void Texture::render(ImDrawList* const drawList, const ImRect imageBounds, const ImU32 color)
{
    constexpr ImVec2 UV_MIN{0.0f, 0.0f};
    constexpr ImVec2 UV_MAX{1.0f, 1.0f};

    if (!isLoaded())
        load();

    drawList->AddImage(std::get<Loaded>(m_state).getId(), imageBounds.Min, imageBounds.Max, UV_MIN, UV_MAX, color);
}

bool Texture::isLoaded() const
{
    return std::holds_alternative<Loaded>(m_state);
}

[[nodiscard]] ImVec2 Texture::getSize() const
{
    return m_size;
}

void Texture::load()
{
    auto image{std::get<Image>(std::move(m_state))};
    m_state.emplace<Loaded>(image, m_size);
}