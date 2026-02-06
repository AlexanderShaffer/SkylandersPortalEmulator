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

export module FigureLoader;

import std;
import Figure;
import PortalEmulator;
import Texture;
import Owner;

export class FigureLoader
{
private:

    using FigureList = std::flat_map<std::pair<bool, std::string>, std::unique_ptr<Figure>>;

    struct FigureGroup
    {
        FigureList Figures{};
        std::uint8_t TickWhenFound{};
    };

public:

    void renderSkylanderButtons();

private:

    [[nodiscard]] static int searchFor(const std::filesystem::path& directory, const std::function<int(const std::filesystem::path&, const std::filesystem::path&)>& searchForPlayables);

    void run(const std::stop_token& token);
    [[nodiscard]] std::pair<int, int> searchForFigures();
    [[nodiscard]] int searchForPlayables(FigureList& figures, SwapperHalf swapperHalf, const std::filesystem::path& dumpsDirectory, const std::filesystem::path& iconsDirectory);
    void loadPlayable(FigureList& figures, SwapperHalf swapperHalf, const std::filesystem::path& dumpPath, const std::filesystem::path& iconPath, std::string_view name);
    void removeLostFigures(int playablesFound, int groupsFound);

private:

    PortalEmulator m_portalEmulator{};
    std::flat_map<std::string, FigureGroup> m_figureGroups{};
    std::vector<Texture> m_detachedTextures{};
    std::mutex m_mutex{};
    std::uint8_t m_tick{};
    int m_playablesLoaded{};

    std::jthread m_thread{std::bind_front(&FigureLoader::run, this)};
};