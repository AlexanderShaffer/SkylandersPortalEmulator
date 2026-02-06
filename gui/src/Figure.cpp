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

module Figure;

import "Common.hpp";

Playable::~Playable()
{
    if (m_portalSlot)
        std::println("A Playable was destroyed while linked to a portal slot");
}

void Playable::renderTexture(ImDrawList* const drawList, const ImRect imageBounds, const ImU32 color)
{
    m_texture.render(drawList, imageBounds, color);
}

void Playable::detachTexture(std::vector<Texture>& destination)
{
    if (m_texture.isLoaded())
        destination.emplace_back(std::move(m_texture));
}

[[nodiscard]] ImRect Playable::update(const int index, const ImVec2 pos, const float imageWidth, const ImVec2 padding, PortalEmulator& portalEmulator)
{
    if (m_portalSlot && m_portalSlot->getState() == PortalSlotState::UNLOADED)
        m_portalSlot = nullptr;

    const auto textureSize{m_texture.getSize()};
    const auto iconSize{textureSize / textureSize.x * imageWidth};
    const auto iconPos{pos + padding};

    m_buttonBounds = {pos, pos + iconSize + padding * 2.0f};
    m_buttonIconBounds = {iconPos, iconPos + iconSize};

    bool held;

    if (m_buttonDisabled)
        m_buttonHovered = false;
    else
        ImGui::ButtonBehavior(m_buttonBounds, index, &m_buttonHovered, &held);

    //TODO: Fix left click and right click behavior
    if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
        setState(portalEmulator, State::OFF_PORTAL);
    else if (!m_buttonDisabled && m_buttonHovered)
    {
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            toggle(portalEmulator, State::ON_PORTAL);
        else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
            toggle(portalEmulator, State::RESTING);
    }

    return m_buttonBounds;
}

enum ButtonState
{
    NOT_HOVERED,
    HOVERED,
    DISABLED,
    COUNT
};

using SkylanderButtonColors = std::array<std::array<ImU32, ButtonState::COUNT>, Playable::State::COUNT>;

[[nodiscard]] SkylanderButtonColors createSkylanderButtonColors()
{
    SkylanderButtonColors colors{};

    colors[Playable::State::OFF_PORTAL][NOT_HOVERED] = 0xFFCCCCCC;
    colors[Playable::State::OFF_PORTAL][HOVERED] = 0xFFFFFFFF;
    colors[Playable::State::OFF_PORTAL][DISABLED] = 0xFF555555;

    colors[Playable::State::ON_PORTAL][NOT_HOVERED] = 0xFF00CCCC;
    colors[Playable::State::ON_PORTAL][HOVERED] = 0xFF33FFFF;
    colors[Playable::State::ON_PORTAL][DISABLED] = 0xFF005555;

    colors[Playable::State::RESTING][NOT_HOVERED] = 0xFF3333CC;
    colors[Playable::State::RESTING][HOVERED] = 0xFF6666FF;
    colors[Playable::State::RESTING][DISABLED] = 0xFF111155;

    return colors;
}

SkylanderButtonColors SKYLANDER_BUTTON_COLORS{createSkylanderButtonColors()};

void Playable::render(ImDrawList* const drawList, bool disabled)
{
    disabled = disabled || (m_portalSlot && m_portalSlot->getState() != PortalSlotState::LOADED);

    const int buttonState{disabled ? ButtonState::DISABLED : m_buttonHovered ? ButtonState::HOVERED : ButtonState::NOT_HOVERED};
    const ImU32 color{SKYLANDER_BUTTON_COLORS[computeState()][buttonState]};
    constexpr double VELOCITY{200.0};
    constexpr double MIN_DELTA_Y{-20.0};
    constexpr double MAX_DELTA_Y{};

    m_buttonDeltaY = std::clamp(m_buttonDeltaY + (m_buttonHovered ? -VELOCITY : VELOCITY) * ImGui::GetIO().DeltaTime, MIN_DELTA_Y, MAX_DELTA_Y);

    auto buttonBounds{m_buttonBounds};
    auto iconBounds{m_buttonIconBounds};

    iconBounds.TranslateY(m_buttonDeltaY);
    buttonBounds.TranslateY(m_buttonDeltaY);

    drawList->AddRectFilled(buttonBounds.Min, buttonBounds.Max, color);
    renderTexture(drawList, iconBounds, color);
    m_buttonDisabled = disabled;
}

ImVec2 Playable::getTextureSize() const
{
    return m_texture.getSize();
}

[[nodiscard]] bool Playable::isOnPortal() const
{
    return computeState() == State::ON_PORTAL;
}

auto Playable::computeState() const -> State
{
    return m_resting ? State::RESTING : m_portalSlot && (m_portalSlot->getState() == PortalSlotState::LOADING || m_portalSlot->getState() == PortalSlotState::LOADED) ? State::ON_PORTAL : State::OFF_PORTAL;
}

void Playable::setState(PortalEmulator& portalEmulator, const State newState)
{
    const auto state{computeState()};

    if (state == newState || (m_portalSlot && m_portalSlot->getState() != PortalSlotState::LOADED))
        return;

    if (newState == State::ON_PORTAL)
    {
        m_portalSlot = portalEmulator.linkPortalSlot(m_dumpPath);

        if (!m_portalSlot)
            return;
    }
    else if (state == State::ON_PORTAL && !portalEmulator.requestUnload(m_portalSlot))
        return;

    m_resting = newState == State::RESTING;
}

void Playable::toggle(PortalEmulator& portalEmulator, const State state)
{
    setState(portalEmulator, computeState() == state ? State::OFF_PORTAL : state);
}

bool PlayableFigure::find(const uint8_t tick, const SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath)
{
    TickWhenFound = tick;
    return false;
}

bool PlayableFigure::shouldRemove(const uint8_t tick, std::unique_lock<std::mutex>& uniqueLock, std::vector<Texture>& detachedTextures, int& playablesLoaded)
{
    const bool shouldRemove{TickWhenFound != tick};

    if (shouldRemove)
    {
        playablesLoaded--;

        if (!uniqueLock)
            uniqueLock.lock();

        detachTexture(detachedTextures);
    }

    return shouldRemove;
}

void PlayableFigure::update(int& index, const ImVec2 pos, const float imageWidth, int& swapperBottomsOnPortal, int& swapperTopsOnPortal, const ImVec2 padding, PortalEmulator& portalEmulator)
{
    const auto buttonBounds{Playable::update(index, pos, imageWidth, padding, portalEmulator)};

    ImGui::ItemSize(buttonBounds);
    ImGui::ItemAdd(buttonBounds, index);
    index++;
}

void PlayableFigure::render(ImDrawList* const drawList, const bool disableSwapperBottoms, const bool disableSwapperTops, const bool connectedToInterface)
{
    Playable::render(drawList, disableSwapperBottoms || disableSwapperTops || !connectedToInterface);
}

bool Swapper::find(const uint8_t tick, const SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath)
{
    bool loadedPlayable{};

    if (swapperHalf == SwapperHalf::BOTTOM)
    {
        if (!m_bottomHalf)
        {
            m_bottomHalf.emplace(dumpPath, iconPath);
            loadedPlayable = true;
        }

        m_bottomHalf->TickWhenFound = tick;
    }
    else if (swapperHalf == SwapperHalf::TOP)
    {
        if (!m_topHalf)
        {
            m_topHalf.emplace(dumpPath, iconPath);
            loadedPlayable = true;
        }

        m_topHalf->TickWhenFound = tick;
    }

    return loadedPlayable;
}

bool Swapper::shouldRemove(const uint8_t tick, std::unique_lock<std::mutex>& uniqueLock, std::vector<Texture>& detachedTextures, int& playablesLoaded)
{
    if (m_bottomHalf && m_bottomHalf->TickWhenFound != tick)
    {
        playablesLoaded--;

        if(!uniqueLock)
            uniqueLock.lock();

        m_bottomHalf->detachTexture(detachedTextures);
        m_bottomHalf.reset();
    }

    if (m_topHalf && m_topHalf->TickWhenFound != tick)
    {
        playablesLoaded--;

        if (!uniqueLock)
            uniqueLock.lock();

        m_topHalf->detachTexture(detachedTextures);
        m_topHalf.reset();
    }

    return !m_bottomHalf && !m_topHalf;
}

void Swapper::update(int& index, ImVec2 pos, const float imageWidth, int& swapperBottomsOnPortal, int& swapperTopsOnPortal, const ImVec2 padding, PortalEmulator& portalEmulator)
{
    if (!m_topHalf && !m_bottomHalf)
        return;

    ImRect bounds{pos, pos};

    if (m_topHalf)
    {
        const auto topButtonBounds{m_topHalf->update(index, pos, imageWidth, padding, portalEmulator)};

        if (m_topHalf->isOnPortal())
            swapperTopsOnPortal++;

        pos.y += topButtonBounds.GetHeight() + ImGui::GetStyle().ItemSpacing.y;
        bounds.Add(topButtonBounds);
        index++;
    }

    if (m_bottomHalf)
    {
        const auto bottomButtonBounds{m_bottomHalf->update(index, pos, imageWidth, padding, portalEmulator)};

        if (m_bottomHalf->isOnPortal())
            swapperBottomsOnPortal++;

        bounds.Add(bottomButtonBounds);
        index++;
    }

    ImGui::ItemSize(bounds);
    ImGui::ItemAdd(bounds, index);
}

void Swapper::render(ImDrawList* const drawList, const bool disableSwapperBottoms, const bool disableSwapperTops, const bool connectedToInterface)
{
    if (m_topHalf)
    {
        const bool topHalfOnPortal{m_topHalf->isOnPortal()};
        const bool disableTopHalf{(topHalfOnPortal && disableSwapperBottoms) || (!topHalfOnPortal && disableSwapperTops)};
        m_topHalf->render(drawList, disableTopHalf || !connectedToInterface);
    }

    if (m_bottomHalf)
    {
        const bool bottomHalfOnPortal{m_bottomHalf->isOnPortal()};
        const bool disableBottomHalf{(bottomHalfOnPortal && disableSwapperTops) || (!bottomHalfOnPortal && disableSwapperBottoms)};
        m_bottomHalf->render(drawList, disableBottomHalf || !connectedToInterface);
    }
}