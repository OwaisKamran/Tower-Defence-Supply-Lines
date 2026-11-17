#include "hud.hpp"
#include "asset_manager.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>

using namespace Theme;

const TowerInfo& towerInfo(TowerType type) {
    static const TowerInfo infos[4] = {
        {TowerType::Barrier,   "Barrier",   50,  0.f,   "Blocks a tile. Enemies walk around it.", "1"},
        {TowerType::Gatling,   "Gatling",   100, 120.f, "Fast shots at the nearest enemy.",       "2"},
        {TowerType::Frost,     "Frost",     150, 0.f,   "Ices a 5x5 area: enemies slow down.",     "3"},
        {TowerType::Artillery, "Artillery", 250, 200.f, "Slow shells, splash damage.",            "4"},
    };
    return infos[static_cast<int>(type)];
}

Hud::Hud(AssetManager& assets) : assets(assets) {}

sf::FloatRect Hud::buttonRect(HudButton button) const {
    switch (button) {
        case HudButton::Algorithms:  return {{1032.f, 8.f}, {144.f, 32.f}};
        case HudButton::Speed:       return {{1182.f, 8.f}, {42.f, 32.f}};
        case HudButton::Sound:       return {{1230.f, 8.f}, {42.f, 32.f}};
        case HudButton::ViewKruskal: return {{986.f, TOP_BAR_H + 10.f}, {190.f, 34.f}};
        case HudButton::ViewRaid:    return {{986.f, TOP_BAR_H + 48.f}, {190.f, 34.f}};
        case HudButton::ViewConvoys: return {{986.f, TOP_BAR_H + 86.f}, {190.f, 34.f}};
        case HudButton::StartWave:   return {{GRID_W / 2.f - 190.f, TOP_BAR_H + 12.f}, {380.f, 36.f}};
        default:                     return {};
    }
}

sf::FloatRect Hud::cardRect(int index) const {
    return {{SIDEBAR_X + 10.f, TOP_BAR_H + 40.f + index * 60.f}, {SIDEBAR_W - 20.f, 52.f}};
}

sf::FloatRect Hud::supplyBoxRect() const {
    return {{SIDEBAR_X + 10.f, TOP_BAR_H + 300.f}, {SIDEBAR_W - 20.f, 130.f}};
}

HudButton Hud::buttonAt(sf::Vector2f mouse, const HudState& state) const {
    if (state.algorithmMenuOpen)
        for (HudButton b : {HudButton::ViewKruskal, HudButton::ViewRaid, HudButton::ViewConvoys})
            if (buttonRect(b).contains(mouse)) return b;
    for (int i = 0; i < 4; i++)
        if (cardRect(i).contains(mouse)) return static_cast<HudButton>(static_cast<int>(HudButton::Tower0) + i);
    for (HudButton b : {HudButton::Algorithms, HudButton::Speed, HudButton::Sound})
        if (buttonRect(b).contains(mouse)) return b;
    if (!state.waveButton.empty() && buttonRect(HudButton::StartWave).contains(mouse)) return HudButton::StartWave;
    return HudButton::None;
}

void Hud::drawTowerIcon(sf::RenderTarget& target, TowerType type, sf::Vector2f centre, float size) const {
    auto drawTexture = [&](const std::string& key) {
        if (!assets.hasTexture(key)) return;
        sf::Sprite s(assets.getTexture(key));
        sf::Vector2u t = assets.getTexture(key).getSize();
        s.setOrigin({t.x / 2.f, t.y / 2.f});
        s.setScale({size / t.x, size / t.y});
        s.setPosition(centre);
        target.draw(s);
    };
    switch (type) {
        case TowerType::Barrier:   drawTexture("barrier_tower"); break;
        case TowerType::Gatling:   drawTexture("gatling_tower_base"); drawTexture("gatling_tower_shooter"); break;
        case TowerType::Frost:     drawTexture("frost_tower"); break;
        case TowerType::Artillery: drawTexture("artillery_tower_base"); drawTexture("artillery_tower_shooter"); break;
    }
}

static void coinIcon(sf::RenderTarget& target, sf::Vector2f c) {
    Ui::circle(target, c, 10.f, Accent, sf::Color(160, 110, 20), 2.f);
    Ui::textCentered(target, "$", {c.x, c.y - 1.f}, 13, sf::Color(110, 70, 10), true);
}

static void heartIcon(sf::RenderTarget& target, sf::Vector2f c) {
    Ui::circle(target, c + sf::Vector2f(-4.5f, -2.f), 5.5f, Bad);
    Ui::circle(target, c + sf::Vector2f(4.5f, -2.f), 5.5f, Bad);
    sf::ConvexShape tip(3);
    tip.setPoint(0, c + sf::Vector2f(-10.f, 0.f));
    tip.setPoint(1, c + sf::Vector2f(10.f, 0.f));
    tip.setPoint(2, c + sf::Vector2f(0.f, 10.f));
    tip.setFillColor(Bad);
    target.draw(tip);
}

static void speakerIcon(sf::RenderTarget& target, sf::Vector2f c, bool muted) {
    Ui::roundedRect(target, {{c.x - 10.f, c.y - 4.f}, {6.f, 8.f}}, 1.f, Text);
    sf::ConvexShape cone(4);
    cone.setPoint(0, c + sf::Vector2f(-5.f, -4.f));
    cone.setPoint(1, c + sf::Vector2f(1.f, -9.f));
    cone.setPoint(2, c + sf::Vector2f(1.f, 9.f));
    cone.setPoint(3, c + sf::Vector2f(-5.f, 4.f));
    cone.setFillColor(Text);
    target.draw(cone);
    if (muted) {
        Ui::line(target, c + sf::Vector2f(4.f, -5.f), c + sf::Vector2f(11.f, 5.f), 2.f, Bad);
        Ui::line(target, c + sf::Vector2f(4.f, 5.f), c + sf::Vector2f(11.f, -5.f), 2.f, Bad);
    } else {
        Ui::line(target, c + sf::Vector2f(5.f, -4.f), c + sf::Vector2f(5.f, 4.f), 2.f, Text);
        Ui::line(target, c + sf::Vector2f(9.f, -7.f), c + sf::Vector2f(9.f, 7.f), 2.f, Text);
    }
}

void Hud::drawTopBar(sf::RenderTarget& target, const HudState& s, sf::Vector2f mouse) const {
    sf::RectangleShape barShape({static_cast<float>(WINDOW_W), TOP_BAR_H});
    barShape.setFillColor(sf::Color(13, 16, 26));
    target.draw(barShape);
    sf::RectangleShape edge({static_cast<float>(WINDOW_W), 1.f});
    edge.setPosition({0.f, TOP_BAR_H - 1.f});
    edge.setFillColor(Border);
    target.draw(edge);

    float x = 14.f;
    auto stat = [&](const std::string& label, const std::string& value, sf::Color color) {
        Ui::text(target, label, {x + 28.f, 7.f}, 11, TextDim);
        Ui::text(target, value, {x + 28.f, 19.f}, 18, color, true);
        x += 120.f;
    };
    coinIcon(target, {x + 11.f, 24.f});
    stat("MONEY", "$" + std::to_string(s.money), Accent);
    heartIcon(target, {x + 11.f, 24.f});
    stat("LIVES", std::to_string(s.lives), s.lives <= 5 ? Bad : Text);
    Ui::circle(target, {x + 11.f, 24.f}, 8.f, sf::Color::Transparent, Main, 3.f);
    stat("WAVE", std::to_string(s.wave), Text);

    Ui::button(target, buttonRect(HudButton::Algorithms), "Algorithms", mouse, s.algorithmMenuOpen, 15);
    Ui::button(target, buttonRect(HudButton::Speed), std::to_string(s.speed) + "x", mouse, s.speed == 2, 14);
    sf::FloatRect sound = buttonRect(HudButton::Sound);
    Ui::button(target, sound, "", mouse, false, 14);
    speakerIcon(target, {sound.position.x + 20.f, sound.position.y + 16.f}, s.muted);
}

void Hud::drawAlgorithmMenu(sf::RenderTarget& target, sf::Vector2f mouse) const {
    sf::FloatRect first = buttonRect(HudButton::ViewKruskal);
    Ui::roundedRect(target, {{first.position.x - 6.f, first.position.y - 6.f}, {first.size.x + 12.f, 3 * 38.f + 8.f}}, 8.f,
                    Panel, Border, 1.5f);
    const HudButton items[] = {HudButton::ViewKruskal, HudButton::ViewRaid, HudButton::ViewConvoys};
    const char* names[] = {"Plan Network", "Raid Repair", "Load Convoys"};
    const char* keys[] = {"P", "V", "L"};
    for (int i = 0; i < 3; i++) {
        sf::FloatRect r = buttonRect(items[i]);
        Ui::roundedRect(target, r, 6.f, r.contains(mouse) ? PanelHover : PanelLight);
        Ui::text(target, names[i], {r.position.x + 12.f, r.position.y + 7.f}, 15, Text, true);
        Ui::textRight(target, keys[i], {r.position.x + r.size.x - 12.f, r.position.y + 8.f}, 14, TextDim, true);
    }
}

void Hud::drawSidebar(sf::RenderTarget& target, const HudState& s, const SupplyNetwork& network,
                      sf::Vector2f mouse) const {
    sf::RectangleShape back({SIDEBAR_W, GRID_H});
    back.setPosition({SIDEBAR_X, TOP_BAR_H});
    back.setFillColor(sf::Color(17, 21, 33));
    target.draw(back);

    Ui::text(target, "TOWERS", {SIDEBAR_X + 14.f, TOP_BAR_H + 12.f}, 15, Accent, true);
    for (int i = 0; i < 4; i++) {
        const TowerInfo& info = towerInfo(static_cast<TowerType>(i));
        sf::FloatRect r = cardRect(i);
        bool selected = s.selected == info.type;
        bool affordable = s.money >= info.cost;
        Ui::roundedRect(target, r, 8.f, r.contains(mouse) ? PanelHover : PanelLight, selected ? Accent : Border,
                        selected ? 2.5f : 1.f);
        drawTowerIcon(target, info.type, {r.position.x + 27.f, r.position.y + 26.f}, 38.f);
        Ui::text(target, info.name, {r.position.x + 56.f, r.position.y + 14.f}, 17, affordable ? Text : TextDim, true);
        Ui::textRight(target, "$" + std::to_string(info.cost), {r.position.x + r.size.x - 44.f, r.position.y + 14.f}, 17,
                      affordable ? Accent : Bad, true);
        sf::FloatRect key({r.position.x + r.size.x - 32.f, r.position.y + 15.f}, {22.f, 22.f});
        Ui::roundedRect(target, key, 4.f, sf::Color(12, 15, 24), Border, 1.f);
        Ui::textCentered(target, info.hotkey, {key.position.x + 11.f, key.position.y + 10.f}, 13, Text, true);
    }

    if (network.isLoaded()) {
        const Dataset& data = network.getData();
        const MstPlanner& planner = network.getPlanner();
        int outposts = 0, connected = 0;
        for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
            if (o == data.castle) continue;
            outposts++;
            if (planner.isConnectedToCastle(o)) connected++;
        }
        sf::FloatRect box = supplyBoxRect();
        Ui::roundedRect(target, box, 8.f, box.contains(mouse) ? PanelHover : PanelLight, Border, 1.f);
        Ui::text(target, "SUPPLY", {box.position.x + 14.f, box.position.y + 10.f}, 15, Accent, true);
        Ui::textRight(target, "hover for details", {box.position.x + box.size.x - 14.f, box.position.y + 12.f}, 12, TextDim);
        float y = box.position.y + 42.f;
        auto row = [&](const std::string& label, const std::string& value, sf::Color color) {
            Ui::text(target, label, {box.position.x + 14.f, y}, 15, TextDim);
            Ui::textRight(target, value, {box.position.x + box.size.x - 14.f, y - 2.f}, 18, color, true);
            y += 27.f;
        };
        row("Network cost", std::to_string(planner.getResult().totalCost), Text);
        row("Outposts connected", std::to_string(connected) + "/" + std::to_string(outposts),
            connected == outposts ? Good : Bad);
        row("Last convoy value", std::to_string(network.getReport().packing.totalValue), Accent);
    }

    Ui::text(target, "H  help        Esc  pause        F  speed", {SIDEBAR_X + 14.f, TOP_BAR_H + GRID_H - 30.f}, 13, TextDim);
}

void Hud::drawWaveButton(sf::RenderTarget& target, const HudState& s, sf::Vector2f mouse) const {
    if (s.waveButton.empty()) return;
    Ui::button(target, buttonRect(HudButton::StartWave), s.waveButton, mouse, false, 16);
}

void Hud::drawWaveBanner(sf::RenderTarget& target, const std::string& title, const std::string& subtitle, float age) const {
    const float length = 2.2f;
    if (age < 0.f || age > length) return;
    float in = Ui::easeOut(age / 0.3f);
    float out = 1.f - Ui::clamp01((age - (length - 0.5f)) / 0.5f);
    float alpha = std::min(in, out);
    float y = TOP_BAR_H + 150.f - 30.f * (1.f - in);

    sf::RectangleShape bg({GRID_W, 84.f});
    bg.setPosition({0.f, y});
    bg.setFillColor(sf::Color(10, 12, 20, static_cast<std::uint8_t>(180 * alpha)));
    target.draw(bg);
    Ui::textCentered(target, title, {GRID_W / 2.f, y + 30.f}, 36, Theme::withAlpha(Accent, static_cast<std::uint8_t>(255 * alpha)), true);
    Ui::textCentered(target, subtitle, {GRID_W / 2.f, y + 63.f}, 18, Theme::withAlpha(Text, static_cast<std::uint8_t>(255 * alpha)), false);
}

void Hud::drawToast(sf::RenderTarget& target, const std::string& text, sf::Color color, float age, float length) const {
    if (text.empty() || age < 0.f || age > length) return;
    float alpha = std::min(Ui::clamp01(age / 0.15f), Ui::clamp01((length - age) / 0.4f));
    auto a = [alpha](sf::Color c) { return Theme::withAlpha(c, static_cast<std::uint8_t>(c.a * alpha)); };
    float w = Ui::textWidth(text, 16, true) + 46.f;
    sf::FloatRect box({GRID_W / 2.f - w / 2.f, TOP_BAR_H + GRID_H - 62.f}, {w, 38.f});
    Ui::roundedRect(target, box, 19.f, a(sf::Color(13, 16, 26, 235)), a(color), 2.f);
    Ui::circle(target, {box.position.x + 19.f, box.position.y + 19.f}, 5.f, a(color));
    Ui::text(target, text, {box.position.x + 32.f, box.position.y + 8.f}, 16, a(Text), true);
}

void Hud::drawTooltip(sf::RenderTarget& target, const std::vector<std::string>& lines, sf::Vector2f mouse) const {
    if (lines.empty()) return;
    float width = 0.f;
    for (size_t i = 0; i < lines.size(); i++) width = std::max(width, Ui::textWidth(lines[i], i == 0 ? 15 : 13, i == 0));
    sf::Vector2f size(width + 24.f, 14.f + 22.f + (lines.size() - 1) * 18.f);
    sf::Vector2f pos = mouse + sf::Vector2f(18.f, 18.f);
    pos.x = std::min(pos.x, WINDOW_W - size.x - 6.f);
    pos.y = std::min(pos.y, WINDOW_H - size.y - 6.f);
    Ui::roundedRect(target, {pos + sf::Vector2f(3.f, 4.f), size}, 8.f, sf::Color(0, 0, 0, 90));
    Ui::roundedRect(target, {pos, size}, 8.f, sf::Color(20, 25, 40, 245), Border, 1.5f);
    Ui::text(target, lines[0], {pos.x + 12.f, pos.y + 8.f}, 15, Accent, true);
    for (size_t i = 1; i < lines.size(); i++)
        Ui::text(target, lines[i], {pos.x + 12.f, pos.y + 12.f + 4.f + i * 18.f}, 13, Text);
}

sf::FloatRect Hud::helpCloseRect(float openAmount) const {
    float x = WINDOW_W - (560.f + 20.f) * Ui::easeOut(openAmount);
    return {{x + 560.f - 110.f, TOP_BAR_H + 30.f + 640.f - 52.f}, {94.f, 36.f}};
}

void Hud::drawHelp(sf::RenderTarget& target, float openAmount, sf::Vector2f mouse) const {
    if (openAmount <= 0.f) return;
    float t = Ui::easeOut(openAmount);
    sf::RectangleShape shade({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
    shade.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(120 * t)));
    target.draw(shade);

    sf::FloatRect box({WINDOW_W - (560.f + 20.f) * t, TOP_BAR_H + 30.f}, {560.f, 640.f});
    Ui::panel(target, box, "HELP");
    float x = box.position.x + 20.f, y = box.position.y + 44.f;

    const char* keys[][2] = {
        {"1 - 4", "select a tower (or click a card)"},
        {"Left click", "place the selected tower"},
        {"Space", "start the next wave now"},
        {"P  V  L", "algorithm views (or the Algorithms button)"},
        {"F", "game speed 1x / 2x"},
        {"M", "show / hide the supply map"},
        {"D", "raid the next road now (demo)"},
        {"N", "mute / unmute sounds"},
        {"Esc", "pause menu, or close a view"},
        {"R", "restart (on the game over screen)"},
    };
    for (auto& k : keys) {
        float w = Ui::textWidth(k[0], 14, true) + 16.f;
        Ui::roundedRect(target, {{x, y - 2.f}, {w, 22.f}}, 5.f, PanelLight, Border, 1.f);
        Ui::text(target, k[0], {x + 8.f, y}, 14, Text, true);
        Ui::text(target, k[1], {x + 120.f, y}, 14, TextDim);
        y += 29.f;
    }

    y += 10.f;
    Ui::text(target, "Map", {x, y}, 15, Accent, true);
    y += 28.f;
    auto legend = [&](const std::string& what) {
        Ui::text(target, what, {x + 56.f, y}, 14, TextDim);
        y += 27.f;
    };
    Ui::line(target, {x, y + 9.f}, {x + 44.f, y + 9.f}, 4.f, Main);
    legend("supply road (minimum spanning tree)");
    Ui::line(target, {x, y + 9.f}, {x + 44.f, y + 9.f}, 2.f, Theme::withAlpha(Bad, 160), 8.f, 6.f);
    legend("destroyed road");
    Ui::circle(target, {x + 22.f, y + 9.f}, 7.f, Main, sf::Color(235, 240, 250), 2.f);
    legend("outpost (hover for its name and requests)");
    Ui::circle(target, {x + 22.f, y + 9.f}, 7.f, CutOff, sf::Color(90, 92, 100), 2.f);
    legend("outpost cut off from the Castle");
    Ui::circle(target, {x + 15.f, y + 9.f}, 5.f, Good, sf::Color(15, 18, 26), 1.5f);
    Ui::circle(target, {x + 30.f, y + 9.f}, 5.f, Bad, sf::Color(15, 18, 26), 1.5f);
    legend("tower near a supplied / cut-off outpost");
    Ui::text(target, "Hover over anything for details.", {x, y + 6.f}, 13, TextDim);

    Ui::button(target, helpCloseRect(openAmount), "Close", mouse);
}
