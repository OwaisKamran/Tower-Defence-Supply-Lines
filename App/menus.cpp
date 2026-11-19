#include "menus.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>

using namespace Theme;

static const char* MAIN_LABELS[] = {"Play", "How to Play", "Algorithm View", "Quit"};
static const char* PAUSE_LABELS[] = {"Resume", "Restart", "How to Play", "Main Menu", "Quit"};

sf::FloatRect Menus::mainButton(int index) const {
    const float w = 230.f, h = 54.f, gap = 18.f;
    float startX = (WINDOW_W - (4 * w + 3 * gap)) / 2.f;
    return {{startX + index * (w + gap), 600.f}, {w, h}};
}

sf::FloatRect Menus::pauseButton(int index) const {
    return {{WINDOW_W / 2.f - 140.f, 250.f + index * 66.f}, {280.f, 52.f}};
}

sf::FloatRect Menus::backButton() const {
    return {{WINDOW_W / 2.f - 100.f, WINDOW_H - 82.f}, {200.f, 50.f}};
}

sf::FloatRect Menus::gameOverButton(int index) const {
    return {{WINDOW_W / 2.f - 250.f + index * 260.f, 520.f}, {240.f, 54.f}};
}

static void drawRidge(sf::RenderTarget& target, float baseY, float amp1, float freq1, float amp2, float freq2,
                      float phase, sf::Color color) {
    sf::VertexArray strip(sf::PrimitiveType::TriangleStrip);
    for (float x = 0.f; x <= WINDOW_W + 20.f; x += 20.f) {
        float y = baseY - amp1 * std::sin(x * freq1 + phase) - amp2 * std::sin(x * freq2 + 2.f * phase);
        strip.append(sf::Vertex{{x, y}, color});
        strip.append(sf::Vertex{{x, static_cast<float>(WINDOW_H)}, color});
    }
    target.draw(strip);
}

static void rect(sf::RenderTarget& target, float x, float y, float w, float h, sf::Color color) {
    sf::RectangleShape r({w, h});
    r.setPosition({x, y});
    r.setFillColor(color);
    target.draw(r);
}

static void drawCastle(sf::RenderTarget& target, float cx, float groundY) {
    const sf::Color stone(28, 32, 50), stoneLight(40, 46, 70), window(255, 196, 90);
    struct Tower { float dx, w, h; };
    const Tower towers[] = {{-190, 50, 150}, {-120, 60, 200}, {-30, 70, 240}, {60, 60, 220}, {140, 50, 160}};

    rect(target, cx - 200.f, groundY - 110.f, 400.f, 110.f, stone);
    for (const Tower& t : towers) {
        float x = cx + t.dx, top = groundY - t.h;
        rect(target, x, top, t.w, t.h, stone);
        rect(target, x, top, 4.f, t.h, stoneLight);
        for (float bx = x; bx < x + t.w - 4.f; bx += 14.f)
            rect(target, bx, top - 10.f, 8.f, 10.f, stone);
        for (float wy = top + 24.f; wy < groundY - 40.f; wy += 44.f)
            rect(target, x + t.w / 2.f - 4.f, wy, 8.f, 14.f, window);
    }
    for (float bx = cx - 200.f; bx < cx + 200.f; bx += 16.f)
        rect(target, bx, groundY - 120.f, 9.f, 10.f, stone);
    Ui::roundedRect(target, {{cx - 22.f, groundY - 60.f}, {44.f, 60.f}}, 12.f, sf::Color(14, 16, 26));
    float poleX = cx + 5.f, poleTop = groundY - 240.f - 50.f;
    rect(target, poleX, poleTop, 3.f, 50.f, stoneLight);
    sf::ConvexShape flag(3);
    flag.setPoint(0, {poleX + 3.f, poleTop});
    flag.setPoint(1, {poleX + 38.f, poleTop + 10.f});
    flag.setPoint(2, {poleX + 3.f, poleTop + 20.f});
    flag.setFillColor(Bad);
    target.draw(flag);
}

void Menus::drawBackground(sf::RenderTarget& target, std::uint8_t darken) const {
    const float W = static_cast<float>(WINDOW_W), H = static_cast<float>(WINDOW_H);

    sf::VertexArray sky(sf::PrimitiveType::TriangleStrip, 4);
    sky[0] = sf::Vertex{{0.f, 0.f}, sf::Color(12, 16, 38)};
    sky[1] = sf::Vertex{{W, 0.f}, sf::Color(12, 16, 38)};
    sky[2] = sf::Vertex{{0.f, 520.f}, sf::Color(214, 110, 72)};
    sky[3] = sf::Vertex{{W, 520.f}, sf::Color(214, 110, 72)};
    target.draw(sky);

    for (int i = 0; i < 80; i++) {
        float x = static_cast<float>((i * 977 + 131) % WINDOW_W);
        float y = static_cast<float>((i * 389 + 17) % 260);
        Ui::circle(target, {x, y}, (i % 5 == 0) ? 1.6f : 1.f, sf::Color(255, 255, 255, static_cast<std::uint8_t>(70 + (i * 37) % 150)));
    }

    for (int i = 4; i >= 0; i--)
        Ui::circle(target, {W / 2.f, 400.f}, 90.f + i * 40.f, sf::Color(255, 190, 110, static_cast<std::uint8_t>(i == 0 ? 200 : 22)));

    drawRidge(target, 420.f, 50.f, 0.006f, 25.f, 0.017f, 0.8f, sf::Color(92, 70, 96));
    drawRidge(target, 470.f, 35.f, 0.009f, 18.f, 0.023f, 2.1f, sf::Color(48, 44, 70));

    drawCastle(target, W / 2.f, 500.f);
    drawRidge(target, 505.f, 8.f, 0.012f, 4.f, 0.05f, 0.3f, sf::Color(26, 36, 40));

    const sf::Vector2f castle(W / 2.f, 500.f);
    const sf::Vector2f outposts[] = {{70, 560}, {200, 515}, {340, 522}, {940, 522}, {1080, 515}, {1210, 560}};
    const int parent[] = {1, 2, -1, -1, 3, 4};
    for (int i = 0; i < 6; i++) {
        sf::Vector2f to = parent[i] < 0 ? castle : outposts[parent[i]];
        Ui::line(target, outposts[i], to, 2.5f, withAlpha(Accent, 150));
    }
    for (const sf::Vector2f& p : outposts) Ui::circle(target, p, 6.f, Accent, sf::Color(60, 40, 10), 2.f);

    sf::RectangleShape shade({W, H});
    shade.setFillColor(sf::Color(8, 10, 18, darken));
    target.draw(shade);
}

void Menus::drawMain(sf::RenderTarget& target, sf::Vector2f mouse, float time) const {
    drawBackground(target, 40);

    const std::string first = "TOWER DEFENCE: ", second = "SUPPLY LINES";
    const unsigned titleSize = 60;
    float w1 = Ui::textWidth(first, titleSize, true), w2 = Ui::textWidth(second, titleSize, true);
    float x = WINDOW_W / 2.f - (w1 + w2) / 2.f;
    Ui::text(target, first, {x + 3.f, 58.f + 4.f}, titleSize, sf::Color(0, 0, 0, 150), true);
    Ui::text(target, first, {x, 58.f}, titleSize, Text, true);
    Ui::text(target, second, {x + w1 + 3.f, 58.f + 4.f}, titleSize, sf::Color(0, 0, 0, 150), true);
    Ui::text(target, second, {x + w1, 58.f}, titleSize, Accent, true);
    Ui::textCentered(target, "Kruskal MST + Multiple Knapsack DP", {WINDOW_W / 2.f, 168.f}, 24, Text);

    sf::VertexArray band(sf::PrimitiveType::TriangleStrip, 4);
    band[0] = sf::Vertex{{0.f, 470.f}, sf::Color(8, 10, 18, 0)};
    band[1] = sf::Vertex{{static_cast<float>(WINDOW_W), 470.f}, sf::Color(8, 10, 18, 0)};
    band[2] = sf::Vertex{{0.f, static_cast<float>(WINDOW_H)}, sf::Color(8, 10, 18, 235)};
    band[3] = sf::Vertex{{static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)}, sf::Color(8, 10, 18, 235)};
    target.draw(band);

    Ui::textCentered(target, "Defend the Castle and keep its supply lines running.", {WINDOW_W / 2.f, 560.f}, 19, Text);

    for (int i = 0; i < 4; i++) {
        sf::FloatRect r = mainButton(i);
        bool hover = r.contains(mouse);
        float lift = hover ? 3.f : 0.f;
        r.position.y -= lift;
        if (i == 0) {
            float glow = 0.5f + 0.5f * std::sin(time * 3.f);
            Ui::roundedRect(target, {{r.position.x - 4.f, r.position.y - 4.f}, {r.size.x + 8.f, r.size.y + 8.f}}, 11.f,
                            Theme::withAlpha(Accent, static_cast<std::uint8_t>(40 + 50 * glow)));
        }
        Ui::roundedRect(target, r, 9.f, hover ? Accent : sf::Color(28, 34, 52, 240), Accent, 2.f);
        Ui::textCentered(target, MAIN_LABELS[i], {r.position.x + r.size.x / 2.f, r.position.y + r.size.y / 2.f}, 21,
                         hover ? sf::Color(30, 24, 10) : Text, true);
    }
    Ui::textCentered(target, "CT-363 Design and Analysis of Algorithms - Complex Engineering Problem",
                     {WINDOW_W / 2.f, WINDOW_H - 28.f}, 13, TextDim);
}

Menus::Action Menus::clickMain(sf::Vector2f mouse) const {
    const Action actions[] = {Action::Play, Action::HowToPlay, Action::AlgorithmView, Action::Quit};
    for (int i = 0; i < 4; i++)
        if (mainButton(i).contains(mouse)) return actions[i];
    return Action::None;
}

void Menus::drawHowToPlay(sf::RenderTarget& target, sf::Vector2f mouse) const {
    drawBackground(target, 215);
    Ui::textCentered(target, "HOW TO PLAY", {WINDOW_W / 2.f, 50.f}, 34, Accent, true);

    struct Section { const char* title; const char* lines[4]; };
    const Section sections[] = {
        {"1. Defend the Castle",
         {"Enemies walk from the left edge to the Castle on the right (A* finds their path).",
          "Place towers with the mouse; blocking towers force enemies to walk around.",
          "Each enemy that reaches the Castle costs a life. 0 lives = game over.",
          "Kills give money: Soldier $10, Runner $8, Tank $25, Shielded $15."}},
        {"2. Supply roads (Part A - greedy, Kruskal)",
         {"The blue roads are a minimum spanning tree that links every outpost to the Castle.",
          "Enemies bomb the supply roads near their path during each wave.",
          "3 roads per wave; the cheapest road that reconnects the tree replaces a bombed one. Grey = cut off.",
          "Press P to watch Kruskal step by step, V to replay the last raid step by step."}},
        {"3. Convoys (Part B - dynamic programming)",
         {"Between waves 3 convoys (Wagon 20, Cart 15, Mule 10) carry the requested items.",
          "A multiple 0/1 knapsack DP picks the most valuable load; cut-off outposts get nothing.",
          "Towers near a supplied outpost (green dot) fire 25% faster, near a cut-off one (red dot) 25% slower.",
          "Press L to see how the convoys were packed. Hover over anything for details."}},
    };
    float y = 104.f;
    for (const Section& s : sections) {
        sf::FloatRect box({140.f, y}, {WINDOW_W - 280.f, 170.f});
        Ui::panel(target, box, s.title);
        float ly = y + 40.f;
        for (const char* line : s.lines) {
            Ui::circle(target, {box.position.x + 22.f, ly + 9.f}, 3.f, Accent);
            Ui::text(target, line, {box.position.x + 34.f, ly}, 16, Text);
            ly += 30.f;
        }
        y += 186.f;
    }
    Ui::button(target, backButton(), "Back", mouse, false, 20);
}

Menus::Action Menus::clickHowToPlay(sf::Vector2f mouse) const {
    return backButton().contains(mouse) ? Action::Back : Action::None;
}

void Menus::drawPause(sf::RenderTarget& target, sf::Vector2f mouse, float openAmount) const {
    float t = Ui::easeOut(openAmount);
    sf::RectangleShape shade({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
    shade.setFillColor(sf::Color(6, 8, 14, static_cast<std::uint8_t>(170 * t)));
    target.draw(shade);

    float offset = -30.f * (1.f - t);
    sf::FloatRect box({WINDOW_W / 2.f - 190.f, 160.f + offset}, {380.f, 440.f});
    Ui::panel(target, box);
    Ui::textCentered(target, "PAUSED", {WINDOW_W / 2.f, 205.f + offset}, 34, Accent, true);
    for (int i = 0; i < 5; i++) {
        sf::FloatRect r = pauseButton(i);
        r.position.y += offset;
        Ui::button(target, r, PAUSE_LABELS[i], mouse, false, 19);
    }
}

Menus::Action Menus::clickPause(sf::Vector2f mouse) const {
    const Action actions[] = {Action::Resume, Action::Restart, Action::HowToPlay, Action::MainMenu, Action::Quit};
    for (int i = 0; i < 5; i++)
        if (pauseButton(i).contains(mouse)) return actions[i];
    return Action::None;
}

void Menus::drawGameOver(sf::RenderTarget& target, sf::Vector2f mouse, float age, int wave, int kills, int raids) const {
    float t = Ui::easeOut(age / 0.6f);
    sf::RectangleShape shade({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
    shade.setFillColor(sf::Color(20, 4, 6, static_cast<std::uint8_t>(200 * t)));
    target.draw(shade);

    float scale = 0.8f + 0.2f * t;
    Ui::textCentered(target, "GAME OVER", {WINDOW_W / 2.f, 220.f}, static_cast<unsigned>(84 * scale),
                     Theme::withAlpha(Bad, static_cast<std::uint8_t>(255 * t)), true);
    Ui::textCentered(target, "The Castle has fallen", {WINDOW_W / 2.f, 300.f}, 22, Theme::withAlpha(Text, static_cast<std::uint8_t>(255 * t)));

    const std::string labels[] = {"WAVE REACHED", "ENEMIES KILLED", "ROADS DESTROYED"};
    const int values[] = {wave, kills, raids};
    for (int i = 0; i < 3; i++) {
        sf::FloatRect tile({WINDOW_W / 2.f - 330.f + i * 230.f, 350.f}, {200.f, 110.f});
        Ui::panel(target, tile);
        Ui::textCentered(target, labels[i], {tile.position.x + 100.f, tile.position.y + 26.f}, 14, TextDim, true);
        Ui::textCentered(target, std::to_string(values[i]), {tile.position.x + 100.f, tile.position.y + 68.f}, 40, Accent, true);
    }
    Ui::button(target, gameOverButton(0), "Restart  (R)", mouse, false, 20);
    Ui::button(target, gameOverButton(1), "Main Menu", mouse, false, 20);
}

Menus::Action Menus::clickGameOver(sf::Vector2f mouse) const {
    if (gameOverButton(0).contains(mouse)) return Action::Restart;
    if (gameOverButton(1).contains(mouse)) return Action::MainMenu;
    return Action::None;
}
