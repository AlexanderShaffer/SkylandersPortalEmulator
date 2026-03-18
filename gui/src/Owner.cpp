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

module Owner;

import <SDL3/SDL.h>;
import <imgui.h>;
import <backends/imgui_impl_sdl3.h>;
import <backends/imgui_impl_opengl3.h>;
import Font;

SdlOwner::SdlOwner()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        throw std::runtime_error("Failed to initialize SDL");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    constexpr SDL_WindowFlags window_flags{SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_FULLSCREEN};

    m_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    m_window = SDL_CreateWindow("Skylanders Portal Emulator", 0.0f, 0.0f, window_flags);

    if (!m_window)
        throw std::runtime_error("Failed to create SDL window");

    m_glContext = SDL_GL_CreateContext(m_window);

    if (!m_glContext)
        throw std::runtime_error("Failed to create GL context");

    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(m_window);
}

SdlOwner::~SdlOwner()
{
    SDL_GL_DestroyContext(m_glContext);
    SDL_DestroyWindow(m_window);
    SDL_Quit();
}

ImGuiOwner::ImGuiOwner(const SdlOwner& sdlOwner)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(sdlOwner.getWindow(), sdlOwner.getGlContext());
    ImGui_ImplOpenGL3_Init();

    auto& io{ImGui::GetIO()};

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.WantSaveIniSettings = false;
    io.IniFilename = nullptr;
    io.Fonts->AddFontFromMemoryCompressedTTF(FONT_COMPRESSED_DATA, sizeof(FONT_COMPRESSED_DATA), 60.0f);

    ImGui::StyleColorsDark();
    auto& style{ImGui::GetStyle()};

    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = {0.06f, 0.06f, 0.06f, 1.0f};
    style.Colors[ImGuiCol_Border] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_Separator] = {1.0f, 0.5f, 0.0f, 1.0f};
    style.SeparatorTextBorderSize = 10.0f;
    style.SeparatorTextPadding.y = 15.0f;
    style.ItemSpacing = {6.0f, 6.0f};
    style.WindowPadding = {6.0f, 6.0f};
    style.ScaleAllSizes(sdlOwner.getScale());
    style.FontScaleDpi = sdlOwner.getScale();
}

ImGuiOwner::~ImGuiOwner()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}