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

module FigureLoader;

import <imgui_internal.h>;

void FigureLoader::renderSkylanderButtons()
{
    constexpr int SKYLANDERS_PER_ROW{6};
    constexpr ImVec2 PADDING{10.0f, 10.0f}; //TODO: Support accurate scaling for other resolutions and window sizes
    const ImGuiWindow* const window{ImGui::GetCurrentWindow()};
    const auto itemSpacingX{ImGui::GetStyle().ItemSpacing.x};
    const float totalWidth{(window->Size.x - itemSpacingX) / SKYLANDERS_PER_ROW};
    const float imageWidth{totalWidth - itemSpacingX - PADDING.x * 2.0f};
    int column{};
    int swapperBottomsOnPortal{};
    int swapperTopsOnPortal{};
    int index{1};
    auto& pos{window->DC.CursorPos};

    const std::lock_guard lockGuard{m_mutex};

    m_detachedTextures.clear();

    for (const auto&[name, group] : m_figureGroups)
    {
        if (window->DC.IsSameLine)
            ImGui::NewLine();

        ImGui::SeparatorText(name.c_str());

        for (const auto& figure : group.Figures.values())
        {
            figure->update(index, pos, imageWidth, swapperBottomsOnPortal, swapperTopsOnPortal, PADDING, m_portalEmulator);

            if (column++ % SKYLANDERS_PER_ROW != SKYLANDERS_PER_ROW - 1)
                ImGui::SameLine();
        }

        column = 0;
    }

    ImDrawList* const drawList{ImGui::GetCurrentWindow()->DrawList};
    const bool disableSwapperBottoms{swapperBottomsOnPortal > swapperTopsOnPortal};
    const bool disableSwapperTops{swapperTopsOnPortal > swapperBottomsOnPortal};
    const bool connected{m_portalEmulator.isConnected()};

    for (const auto& group : m_figureGroups.values())
        for (auto& figure : group.Figures.values())
            figure->render(drawList, disableSwapperBottoms, disableSwapperTops, connected);
}

void FigureLoader::run(const std::stop_token& token)
{
    std::condition_variable_any threadStopCondition{};
    std::mutex mutex{};
    std::unique_lock lock{mutex};

    while (!token.stop_requested())
    {
        const auto[playablesFound, groupsFound]{searchForFigures()};
        removeLostFigures(playablesFound, groupsFound);
        m_tick = !m_tick;

        constexpr auto SEARCH_TIME_INTERVAL{std::chrono::milliseconds(1000)};
        threadStopCondition.wait_for(lock, token, SEARCH_TIME_INTERVAL, []{return false;});
    }
}

int FigureLoader::searchForPlayables(const std::filesystem::path& directory, FigureList& figures, const SwapperHalf swapperHalf)
{
    const auto dumpsDirectory{directory / "Dumps"};
    const auto iconsDirectory{directory / "Icons"};
    int figuresFound{};

    if (!std::filesystem::is_directory(dumpsDirectory) || !std::filesystem::is_directory(iconsDirectory))
        return figuresFound;

    for (const auto& dump : std::filesystem::directory_iterator{dumpsDirectory})
    {
        const auto& dumpPath{dump.path()};
        constexpr std::string_view DUMP_EXTENSION{".dump"};

        if (dumpPath.extension() != DUMP_EXTENSION)
            continue;

        const auto dumpName{dumpPath.filename().string()};
        const std::string_view name{dumpName.begin(), dumpName.end() - DUMP_EXTENSION.size()};
        constexpr auto IMAGE_EXTENSION{".jpg"};
        const auto iconPath{iconsDirectory / name += IMAGE_EXTENSION};

        if (!std::filesystem::exists(iconPath))
            continue;

        loadPlayable(figures, swapperHalf, dumpPath, iconPath, name);
        figuresFound++;
    }

    return figuresFound;
}

std::pair<int, int> FigureLoader::searchForFigures()
{
    static const std::filesystem::path FIGURES_DIRECTORY{"Figures/"};

    int playablesFound{};
    int groupsFound{};

    if (!std::filesystem::exists(FIGURES_DIRECTORY))
        return {playablesFound, groupsFound};

    for (const auto& file : std::filesystem::directory_iterator{FIGURES_DIRECTORY})
    {
        if (!file.is_directory())
            continue;

        const auto& groupDirectory{file.path()};
        auto groupName{groupDirectory.filename().string()};
        auto it{m_figureGroups.lower_bound(groupName)};

        if (it == m_figureGroups.end() || it->first != groupName)
        {
            const std::lock_guard lock{m_mutex};
            it = m_figureGroups.emplace_hint(it, std::move(groupName), FigureGroup{});
        }

        auto&[figures, tickWhenFound]{it->second};

        groupsFound++;
        tickWhenFound = m_tick;

        playablesFound += searchForPlayables(groupDirectory, figures, SwapperHalf::NONE);
        playablesFound += searchForPlayables(groupDirectory / "Bottom Halves", figures, SwapperHalf::BOTTOM);
        playablesFound += searchForPlayables(groupDirectory / "Top Halves", figures, SwapperHalf::TOP);
    }

    return {playablesFound, groupsFound};
}

void FigureLoader::loadPlayable(FigureList& figures, const SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath, const std::string_view name)
{
    const auto swapper{swapperHalf != SwapperHalf::NONE};
    auto identifier{std::make_pair(swapper, std::string{name})};
    auto it{figures.lower_bound(identifier)};

    if (it == figures.end() || it->first != identifier)
    {
        std::unique_ptr<Figure> newFigure{};

        if (swapperHalf == SwapperHalf::NONE)
        {
            newFigure = std::make_unique<PlayableFigure>(dumpPath, iconPath);
            m_playablesLoaded++;
        }
        else
            newFigure = std::make_unique<Swapper>();

        const std::lock_guard lockGuard{m_mutex};
        it = figures.emplace_hint(it, std::move(identifier), std::move(newFigure));
    }

    if (const auto& figure{it->second}; figure->find(m_tick, swapperHalf, dumpPath, iconPath))
        m_playablesLoaded++;
}

void FigureLoader::removeLostFigures(const int playablesFound, const int groupsFound)
{
    if (playablesFound == m_playablesLoaded && groupsFound == m_figureGroups.size())
        return;

    if (playablesFound > m_playablesLoaded)
        std::println("More Playables were found than loaded.");

    if (groupsFound > m_figureGroups.size())
        std::println("More FigureGroups were found than loaded.");

    for (auto groupIt{m_figureGroups.begin()}; groupIt != m_figureGroups.end();)
    {
        auto&[figures, tickWhenFound]{groupIt->second};

        for (auto figureIt{figures.begin()}; figureIt != figures.end();)
        {
            const auto&[name, figure]{*figureIt};

            if (std::unique_lock uniqueLock{m_mutex, std::defer_lock}; figure->shouldRemove(m_tick, uniqueLock, m_detachedTextures, m_playablesLoaded))
            {
                figureIt = figures.erase(figureIt);
                continue;
            }

            ++figureIt;
        }

        if (tickWhenFound != m_tick)
        {
            if (!figures.empty())
            {
                std::println("Cannot remove a FigureGroup until all of its figures are removed");
                throw std::runtime_error{""};
            }

            const std::lock_guard lockGuard{m_mutex};
            groupIt = m_figureGroups.erase(groupIt);
            continue;
        }

        ++groupIt;
    }

    if (playablesFound != m_playablesLoaded)
        std::println("An error occurred when removing Playables. {} Playables were found, yet {} Playables are loaded.", playablesFound, m_playablesLoaded);

    if (groupsFound != m_figureGroups.size())
        std::println("An error occurred when removing FigureGroups. {} FigureGroups were found, yet {} FigureGroups are loaded.", groupsFound, m_figureGroups.size());
}