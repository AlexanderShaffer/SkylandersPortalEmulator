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

export module Figure;

import <imgui/imgui.h>;
import <imgui/imgui_internal.h>;
import std;
import Texture;
import PortalEmulator;
import PortalSlot;

export enum class SwapperHalf
{
    NONE,
    BOTTOM,
    TOP
};

export class Figure
{
public:

    virtual ~Figure() = default;

    [[nodiscard]] virtual bool find(std::uint8_t tick, SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath) = 0;
    [[nodiscard]] virtual bool shouldRemove(std::uint8_t tick, std::unique_lock<std::mutex>& uniqueLock, std::vector<Texture>& detachedTextures, int& playablesLoaded) = 0;
    virtual void update(int& index, ImVec2 pos, float imageWidth, int& swapperBottomsOnPortal, int& swapperTopsOnPortal, ImVec2 padding, PortalEmulator& portalEmulator) = 0;
    virtual void render(ImDrawList* drawList, bool disableSwapperBottoms, bool disableSwapperTops, bool connectedToInterface) = 0;
};

export class Playable
{
public:

    enum State
    {
        OFF_PORTAL,
        ON_PORTAL,
        RESTING,
        COUNT
    };

public:

    Playable(std::filesystem::path dumpPath, const std::filesystem::path& iconPath) : m_dumpPath{std::move(dumpPath)}, m_texture{iconPath} {}
    ~Playable();

    void renderTexture(ImDrawList* drawList, ImRect imageBounds, ImU32 color);
    void detachTexture(std::vector<Texture>& destination);
    [[nodiscard]] ImRect update(int index, ImVec2 pos, float imageWidth, ImVec2 padding, PortalEmulator& portalEmulator);
    void render(ImDrawList* drawList, bool disabled);
    [[nodiscard]] ImVec2 getTextureSize() const;
    [[nodiscard]] bool isOnPortal() const;

private:

    [[nodiscard]] State computeState() const;
    void setState(PortalEmulator& portalEmulator, State newState);
    void toggle(PortalEmulator& portalEmulator, State state);

public:

    std::uint8_t TickWhenFound{};

private:

    std::filesystem::path m_dumpPath{};
    Texture m_texture;
    std::shared_ptr<PortalSlot> m_portalSlot{}; //TODO: Save any open Skylander dumps before the program terminates
    ImRect m_buttonBounds{}; //TODO: encapsulate button logic in another class
    ImRect m_buttonIconBounds{};
    bool m_buttonHovered{};
    bool m_buttonDisabled{};
    double m_buttonDeltaY{};
    bool m_resting{};
};

export class PlayableFigure final : public Playable, public Figure
{
public:

    PlayableFigure(const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath) : Playable{dumpPath, iconPath} {}

    [[nodiscard]] bool find(std::uint8_t tick, SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath) override;
    [[nodiscard]] bool shouldRemove(std::uint8_t tick, std::unique_lock<std::mutex>& uniqueLock, std::vector<Texture>& detachedTextures, int& playablesLoaded) override;
    void update(int& index, ImVec2 pos, float imageWidth, int& swapperBottomsOnPortal, int& swapperTopsOnPortal, ImVec2 padding, PortalEmulator& portalEmulator) override;
    void render(ImDrawList* drawList, bool disableSwapperBottoms, bool disableSwapperTops, bool connectedToInterface) override;
};

export class Swapper final : public Figure
{
public:

    [[nodiscard]] bool find(std::uint8_t tick, SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath) override;
    [[nodiscard]] bool shouldRemove(std::uint8_t tick, std::unique_lock<std::mutex>& uniqueLock, std::vector<Texture>& detachedTextures, int& playablesLoaded) override;
    void update(int& index, ImVec2 pos, float imageWidth, int& swapperBottomsOnPortal, int& swapperTopsOnPortal, ImVec2 padding, PortalEmulator& portalEmulator) override;
    void render(ImDrawList* drawList, bool disableSwapperBottoms, bool disableSwapperTops, bool connectedToInterface) override;

private:

    std::optional<Playable> m_bottomHalf{};
    std::optional<Playable> m_topHalf{};
};