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

export module Owner;

import <SDL3/SDL.h>;
import std;

export class SdlOwner
{
public:

    SdlOwner();
    ~SdlOwner();
    SdlOwner(const SdlOwner&) = delete;
    SdlOwner(SdlOwner&&) noexcept = delete;
    SdlOwner& operator=(const SdlOwner&) = delete;
    SdlOwner& operator=(SdlOwner&&) noexcept = delete;

    [[nodiscard]] SDL_Window* getWindow() const {return m_window;}
    [[nodiscard]] SDL_GLContext getGlContext() const {return m_glContext;}
    [[nodiscard]] float getScale() const {return m_scale;}

private:

    SDL_Window* m_window{};
    SDL_GLContext m_glContext{};
    float m_scale{};
};

export class ImGuiOwner
{
public:

    explicit ImGuiOwner(const SdlOwner& sdlOwner);
    ~ImGuiOwner();
    ImGuiOwner(const ImGuiOwner&) = delete;
    ImGuiOwner(ImGuiOwner&&) noexcept = delete;
    ImGuiOwner& operator=(const ImGuiOwner&) = delete;
    ImGuiOwner& operator=(ImGuiOwner&&) noexcept = delete;
};