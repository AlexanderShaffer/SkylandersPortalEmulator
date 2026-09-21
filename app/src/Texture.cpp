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

module;
#include <stb_image.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <variant>
#include <filesystem>
module Texture;

Texture::ImageState::~ImageState()
{
    stbi_image_free(m_ptr);
}

Texture::ImageState::ImageState(ImageState&& other) noexcept
{
    *this = std::move(other);
}

auto Texture::ImageState::operator=(ImageState&& other) noexcept -> ImageState&
{
    if (this == &other)
        return *this;

    std::swap(m_ptr, other.m_ptr);
    return *this;
}

Texture::LoadedState::LoadedState(const ImageState& image, const ImVec2 size, SDL_Renderer* const renderer)
{
    if (!image.getPtr())
        return;

    static constexpr int CHANNELS{4};
    SDL_Surface* const surface{SDL_CreateSurfaceFrom(size.x, size.y, SDL_PIXELFORMAT_RGBA32, image.getPtr(), CHANNELS * size.x)};

    if (!surface)
        return;

    m_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
}

Texture::LoadedState::~LoadedState()
{
    SDL_DestroyTexture(m_texture);
}

Texture::LoadedState::LoadedState(LoadedState&& other) noexcept
{
    *this = std::move(other);
}

auto Texture::LoadedState::operator=(LoadedState&& other) noexcept -> LoadedState&
{
    if (this == &other)
        return *this;

    std::swap(m_texture, other.m_texture);
    return *this;
}

Texture::Texture(const std::filesystem::path& imagePath)
{
    int width{766};
    int height{1205};
    stbi_uc* const image{stbi_load(imagePath.string().c_str(), &width, &height, nullptr, STBI_rgb_alpha)};

    m_state.emplace<ImageState>(image);
    m_size = {static_cast<float>(width), static_cast<float>(height)};
}

void Texture::render(ImDrawList* const drawList, SDL_Renderer* const renderer, const ImRect imageBounds, const ImU32 color)
{
    constexpr ImVec2 UV_MIN{0.0f, 0.0f};
    constexpr ImVec2 UV_MAX{1.0f, 1.0f};

    if (!isLoaded())
        load(renderer);

    if (SDL_Texture* const texture{std::get<LoadedState>(m_state).getTexture()}; texture)
        drawList->AddImage(texture, imageBounds.Min, imageBounds.Max, UV_MIN, UV_MAX, color);
    else
        drawList->AddRectFilled(imageBounds.Min, imageBounds.Max, color);
}

bool Texture::isLoaded() const
{
    return std::holds_alternative<LoadedState>(m_state);
}

[[nodiscard]] ImVec2 Texture::getSize() const
{
    return m_size;
}

void Texture::load(SDL_Renderer* const renderer)
{
    auto image{std::get<ImageState>(std::move(m_state))};
    m_state.emplace<LoadedState>(image, m_size, renderer);
}