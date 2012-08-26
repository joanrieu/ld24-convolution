#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>
#include <sstream>

struct Team {
        std::string name;
        sf::Color color;
        int score;
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
                                        cell.team->score += 10;

                                stuff.erase(stuffIt);

                        }

                        nextCells.insert(std::make_pair(cellPos, cell));

                }

        });

        cells.swap(nextCells);

}

struct Pattern {
        Pattern(const char* name, const char* filename);
        void make(Grid* grid, Team* team, const sf::Vector2i& pos) const;
        const char* name;
        std::vector<sf::Vector2i> positions;
};

Pattern::Pattern(const char* name_, const char* filename): name(name_) {

        std::ifstream file(filename);

        int w;
        file >> w;

        bool isCell;
        for (int i = 0; file >> isCell; ++i)
                if (isCell)
                        positions.push_back(sf::Vector2i(i % w, i / w));

}

void Pattern::make(Grid* grid, Team* team, const sf::Vector2i& pos) const {
        Cell cell = { team, false };
        std::for_each(positions.begin(), positions.end(), [&](const sf::Vector2i& offset) {
                grid->cells.insert(std::make_pair(pos + offset, cell));
        });
}

int main() {

        sf::Color bgColor(59, 55, 55), stuffColor(197, 205, 207);
        Team playerTeam = { "Player", sf::Color(9, 178, 207), 0 };
        Team enemyTeam = { "Enemy", sf::Color(204, 9, 9), 0 };

        std::vector<Pattern> patterns;
        patterns.push_back(Pattern("Glider", "glider.txt"));
        patterns.push_back(Pattern("Light-weight spaceship", "ship.txt"));
        std::size_t patternIndex = 0;

        Grid grid;
        patterns[1].make(&grid, &playerTeam, sf::Vector2i(-3, 1));
        patterns[0].make(&grid, &enemyTeam, sf::Vector2i(-5, -3));

        sf::Music music;
        music.openFromFile("distant.ogg");
        music.setLoop(true);
        music.play();

        sf::RenderWindow win(sf::VideoMode::getFullscreenModes().front(), "Convolution");
        win.setVerticalSyncEnabled(true);
        win.setKeyRepeatEnabled(false);

        const float scaleStep = 1.5f;
        float scaling = 100.f;
        sf::Vector2i gridAim;

        while (win.isOpen()) {

                const int moveStep = 1 + 20 / scaling;

                sf::Event event;
                while (win.pollEvent(event)) {

                        if (event.type == sf::Event::Closed)
                                win.close();

#                       define BIND(keyname, action) else if (event.type == sf::Event::KeyPressed and event.key.code == sf::Keyboard::keyname) action
                        BIND(Escape, win.close());
                        BIND(Space, grid.update());
                        BIND(Add, scaling *= scaleStep);
                        BIND(Subtract, scaling /= scaleStep);
                        BIND(Up, gridAim.y -= moveStep);
                        BIND(Down, gridAim.y += moveStep);
                        BIND(Left, gridAim.x -= moveStep);
                        BIND(Right, gridAim.x += moveStep);
                        BIND(PageUp, patternIndex = (patterns.size() + patternIndex - 1) % patterns.size());
                        BIND(PageDown, patternIndex = (patternIndex + 1) % patterns.size());
                        BIND(Return, patterns[patternIndex].make(&grid, &playerTeam, gridAim));
#                       undef BIND

                }

                win.clear(bgColor);

                auto grid_to_screen = [&](const sf::Vector2i& gridPos) {
                        return sf::Vector2f(gridPos.x - gridAim.x, gridPos.y - gridAim.y) * scaling + .5f * sf::Vector2f(win.getSize().x, win.getSize().y);
                };

                std::for_each(grid.cells.begin(), grid.cells.end(), [&](const std::pair<sf::Vector2i, Cell> pair) {

                        sf::RectangleShape cellShape(sf::Vector2f(scaling, scaling));
                        cellShape.setPosition(grid_to_screen(pair.first));
                        cellShape.setFillColor(pair.second.team->color);
                        win.draw(cellShape);

                });

                std::for_each(grid.stuff.begin(), grid.stuff.end(), [&](const std::pair<sf::Vector2i, Stuff> pair) {

                        sf::RectangleShape stuffShape(sf::Vector2f(scaling / 2.f, scaling / 2.f));
                        stuffShape.move(grid_to_screen(pair.first));
                        stuffShape.move(scaling / 4.f, scaling / 4.f);
                        stuffShape.setFillColor(stuffColor);
                        win.draw(stuffShape);

                });

                { // HUD lines

                        sf::RectangleShape line(sf::Vector2f(1, 110));
                        line.setOutlineThickness(1);

                        line.setFillColor(enemyTeam.color);
                        line.setOutlineColor(enemyTeam.color);
                        line.move(sf::Vector2f(20, 30));
                        win.draw(line);

                        line.setFillColor(playerTeam.color);
                        line.setOutlineColor(playerTeam.color);
                        line.move(sf::Vector2f(10, 10));
                        win.draw(line);

                }

                { // HUD text

                        sf::Text score;
                        score.move(sf::Vector2f(50, 0));
                        score.rotate(20);

                        score.move(sf::Vector2f(0, 50));
                        score.setColor(playerTeam.color);

                        {
                                std::ostringstream ss;
                                ss << "You: " << playerTeam.score;
                                score.setString(ss.str());
                        }

                        win.draw(score);

                        score.move(sf::Vector2f(0, 50));
                        score.setColor(enemyTeam.color);

                        {
                                std::ostringstream ss;
                                ss << "Enemy: " << enemyTeam.score;
                                score.setString(ss.str());
                        }

                        win.draw(score);

                }

                { // HUD pattern select

                        sf::Text text;
                        text.move(sf::Vector2f(50, 200));
                        text.setCharacterSize(14);

                        int i = 0;

                        std::for_each(patterns.begin(), patterns.end(), [&](const Pattern& pattern) {

                                text.move(sf::Vector2f(0, 1.5f * text.getLocalBounds().height));

                                std::ostringstream ss;
                                if (i++ == patternIndex)
                                        ss << "> ";
                                ss << pattern.name;
                                text.setString(ss.str());

                                win.draw(text);

                        });

                }

                { //HUD crosshair

                        sf::RectangleShape crosshair(sf::Vector2f(3, 3));
                        crosshair.setFillColor(sf::Color::Transparent);
                        crosshair.setOutlineThickness(2);
                        crosshair.setOutlineColor(sf::Color::Black);

                        crosshair.move(.5f * sf::Vector2f(win.getSize().x, win.getSize().y));
                        crosshair.move(sf::Vector2f(scaling / 2.f, scaling / 2.f));

                        win.draw(crosshair);

                }

                win.display();

        }

        music.stop();

        return EXIT_SUCCESS;

}
