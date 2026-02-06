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

export module Texture;

import <stb_image/stb_image.h>;
import <glew/glew.h>;
import <imgui/imgui.h>;
import <imgui/imgui_internal.h>;
import std;

export class Texture final
{
private:

    class Image final
    {
    public:

        explicit Image(stbi_uc* ptr) : m_ptr{ptr} {}
        ~Image();
        Image(const Image& other) = delete;
        Image(Image&& other) noexcept;
        Image& operator=(const Image& other) = delete;
        Image& operator=(Image&& other) noexcept;

        [[nodiscard]] stbi_uc* getPtr() const {return m_ptr;}

    private:

        stbi_uc* m_ptr{};
    };

    class Loaded final
    {
    public:

        Loaded(const Image& image, ImVec2 size);
        ~Loaded();
        Loaded(const Loaded& other) = delete;
        Loaded(Loaded&& other) noexcept;
        Loaded& operator=(const Loaded& other) = delete;
        Loaded& operator=(Loaded&& other) noexcept;

        [[nodiscard]] GLuint getId() const {return m_id;}

    private:

        static constexpr GLuint NO_TEXTURE{};

        GLuint m_id{NO_TEXTURE};
    };

public:

    explicit Texture(const std::filesystem::path& imagePath);
    ~Texture() = default;
    Texture(const Texture&) = delete;
    Texture(Texture&&) noexcept = default;
    Texture& operator=(const Texture&) = delete;
    Texture& operator=(Texture&&) noexcept = default;

    void render(ImDrawList* drawList, ImRect imageBounds, ImU32 color);
    [[nodiscard]] bool isLoaded() const;
    [[nodiscard]] ImVec2 getSize() const;

private:

    void load();

private:

    std::variant<std::monostate, Image, Loaded> m_state{};
    ImVec2 m_size{};
};