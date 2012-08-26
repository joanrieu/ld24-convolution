#include <SFML/Graphics.hpp>
#include <iostream>

struct Team {
        std::string name;
        sf::Color color;
        float score;
};

struct Cell {
        Team* team;
        bool undead;
};

enum class Stuff {
        DeadCell
};

struct sfVector2i_less {
        bool operator()(const sf::Vector2i& v1, const sf::Vector2i& v2) {
                return v1.y < v2.y or (v1.y == v2.y and v1.x < v2.x);
        }
};

struct Grid {
        std::map<sf::Vector2i, Cell, sfVector2i_less> cells;
        std::map<sf::Vector2i, Stuff, sfVector2i_less> stuff;
        void update();
};

void Grid::update() {

        std::map<sf::Vector2i, std::vector<Team*>, sfVector2i_less> adjacency;

        std::for_each(cells.begin(), cells.end(), [&](const std::pair<sf::Vector2i, Cell>& cellInfo) {

                auto& cellPos(cellInfo.first);
                auto& cell(cellInfo.second);

                sf::Vector2i neighbourPos;
                for (neighbourPos.y = cellPos.y - 1; neighbourPos.y <= cellPos.y + 1; ++neighbourPos.y)
                        for (neighbourPos.x = cellPos.x - 1; neighbourPos.x <= cellPos.x + 1; ++neighbourPos.x)
                                adjacency[neighbourPos].push_back(cell.team);

        });

        decltype(cells) nextCells;

        std::for_each(adjacency.begin(), adjacency.end(), [&](const std::pair<sf::Vector2i, std::vector<Team*>>& adjacencyInfo) {

                auto& cellPos(adjacencyInfo.first);
                auto& neighbours(adjacencyInfo.second);
                int neighbourCount = neighbours.size();

                Team* winningTeam;

                {

                        std::map<Team*, int> teamMembers;
                        std::for_each(neighbours.begin(), neighbours.end(), [&](Team* team) {
                                ++teamMembers[team];
                        });

                        std::map<int, Team*> sortedTeams;
                        std::for_each(teamMembers.begin(), teamMembers.end(), [&](std::pair<Team*, int> teamInfo) {
                                sortedTeams.insert(std::make_pair(teamInfo.second, teamInfo.first));
                        });

                        winningTeam = sortedTeams.rbegin()->second;

                }

                auto cellIt = cells.find(cellPos);

                if (cellIt != cells.end()) {

                        --neighbourCount; // The tile is counted as its own neighbour for team counting purposes.

                        Cell cell = cellIt->second;
                        cell.team = winningTeam;

                        if (neighbourCount == 2 or neighbourCount == 3 or cell.undead)
                                nextCells.insert(std::make_pair(cellPos, cell));
                        else
                                stuff.insert(std::make_pair(cellPos, Stuff::DeadCell));

                } else if (neighbourCount == 3) { // Cells can only appear with three neighbours

                        Cell cell;
                        cell.team = winningTeam;

                        auto stuffIt = stuff.find(cellPos);

                        if (stuffIt != stuff.end()) {

                                if (stuffIt->second == Stuff::DeadCell)
                                        cell.team->score *= 1.05f;

                                stuff.erase(stuffIt);

                        }

                        nextCells.insert(std::make_pair(cellPos, cell));

                }

        });

        cells.swap(nextCells);

}

int main() {

        Team playerTeam = { "Player", sf::Color(200, 50, 50), 1.f };
        Team enemy1Team = { "Enemy 1", sf::Color(50, 200, 50), 1.f };
        Team enemy2Team = { "Enemy 2", sf::Color(50, 50, 200), 1.f };
        Cell playerCell = { &playerTeam, false };
        Cell enemy1Cell = { &enemy1Team, false };
        Cell enemy2Cell = { &enemy2Team, false };

        Grid grid;
        grid.cells.insert(std::make_pair(sf::Vector2i(0, 0), playerCell));
        grid.cells.insert(std::make_pair(sf::Vector2i(1, 0), playerCell));
        grid.cells.insert(std::make_pair(sf::Vector2i(2, 0), playerCell));
        grid.cells.insert(std::make_pair(sf::Vector2i(2, -1), playerCell));
        grid.cells.insert(std::make_pair(sf::Vector2i(1, -2), playerCell));/*
        grid.cells.insert(std::make_pair(sf::Vector2i(0, 1), enemy1Cell));
        grid.cells.insert(std::make_pair(sf::Vector2i(1, 1), enemy1Cell));
        grid.cells.insert(std::make_pair(sf::Vector2i(2, 1), enemy1Cell));
        grid.cells.insert(std::make_pair(sf::Vector2i(0, 2), enemy2Cell));
        grid.cells.insert(std::make_pair(sf::Vector2i(1, 2), enemy2Cell));
        grid.cells.insert(std::make_pair(sf::Vector2i(2, 2), enemy2Cell));*/

        sf::RenderWindow win(sf::VideoMode::getFullscreenModes().front(), "Convolution");
        win.setVerticalSyncEnabled(true);
        win.setKeyRepeatEnabled(false);

        const float scaleStep = 1.5f;
        float scaling = 100.f;
        sf::Vector2f gridAim;

        while (win.isOpen()) {

                const float moveStep = .05f * win.getSize().x / scaling;

                sf::Event event;
                while (win.pollEvent(event)) {

                        if (event.type == sf::Event::Closed)
                                win.close();

#                       define BIND(keyname, action) else if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::keyname) action
                        BIND(Escape, win.close());
                        BIND(Space, { grid.update(); std::cout << playerTeam.score << std::endl; })
                        BIND(Add, scaling *= scaleStep);
                        BIND(Subtract, scaling /= scaleStep);
                        BIND(Up, gridAim.y -= moveStep);
                        BIND(Down, gridAim.y += moveStep);
                        BIND(Left, gridAim.x -= moveStep);
                        BIND(Right, gridAim.x += moveStep);
#                       undef BIND

                }

                win.clear();

                std::for_each(grid.cells.begin(), grid.cells.end(), [&](const std::pair<sf::Vector2i, Cell> pair) {

                        sf::Vector2f cellPos(pair.first.x, pair.first.y);
                        cellPos -= gridAim;
                        cellPos *= scaling;
                        cellPos += .5f * sf::Vector2f(win.getSize().x, win.getSize().y);

                        sf::RectangleShape cellShape(sf::Vector2f(scaling, scaling));
                        cellShape.setPosition(cellPos);
                        cellShape.setFillColor(pair.second.team->color);
                        win.draw(cellShape);

                });

                win.display();

        }

}
