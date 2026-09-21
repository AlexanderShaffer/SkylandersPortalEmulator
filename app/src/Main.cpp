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

#include <SDL3/SDL.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
import Owner;
import PortalEmulator;
import FigureLoader;

int main()
{
    const SdlOwner sdlOwner{};
    PortalEmulator portalEmulator{};
    auto* const window{sdlOwner.getWindow()};
    const ImGuiOwner imGuiOwner{sdlOwner};
    const ImVec2 maxItemSpacing{ImGui::GetStyle().ItemSpacing};
    const auto& io{ImGui::GetIO()};
    bool running{true};
    FigureLoader figureLoader{};

    while (running)
    {
        SDL_Event event;
        const bool fullscreen{static_cast<bool>(SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN)};

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_MOUSE_WHEEL)
                event.wheel.y *= 0.3f;
            else if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11)
                SDL_SetWindowFullscreen(window, !fullscreen);

            ImGui_ImplSDL3_ProcessEvent(&event);
            running = event.type != SDL_EVENT_QUIT && event.type != SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        }

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos({0.0f, 0.0f});
        ImGui::SetNextWindowSize({io.DisplaySize.x, io.DisplaySize.y});
        ImGui::Begin("Main Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        if (ImGui::Button("Exit"))
            break;

        if (ImGui::Button(fullscreen ? "Fullscreen (F11): On" : "Fullscreen (F11): Off"))
            SDL_SetWindowFullscreen(window, !fullscreen);

        ImGui::Text("Pico W connection status:");
        ImGui::SameLine();
        auto[message, color, _]{portalEmulator.getConnectionStatus()};
        ImGui::TextColored(color, " %s", message.data());

        SDL_Rect displayBounds;
        SDL_GetDisplayBounds(SDL_GetDisplayForWindow(window), &displayBounds);
        ImGui::GetStyle().ItemSpacing = maxItemSpacing * (io.DisplaySize.x / displayBounds.w);
        ImGui::GetStyle().WindowPadding = ImGui::GetStyle().ItemSpacing;

        figureLoader.renderSkylanderButtons(portalEmulator, sdlOwner.getRenderer());
        ImGui::End();

        ImGui::Render();
        SDL_SetRenderScale(sdlOwner.getRenderer(), io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(sdlOwner.getRenderer(), 0.0f, 0.0f, 0.0f, 1.0f);
        SDL_RenderClear(sdlOwner.getRenderer());
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), sdlOwner.getRenderer());
        SDL_RenderPresent(sdlOwner.getRenderer());
    }

    sdlOwner.destroyRendererAndWindow();
    return 0;
}