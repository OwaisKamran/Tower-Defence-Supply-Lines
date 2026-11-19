#include "supply_overlay.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>

sf::Vector2f outpostCentre(const Outpost& outpost) {
    return {outpost.gridX * Theme::CELL + Theme::CELL / 2.f, outpost.gridY * Theme::CELL + Theme::CELL / 2.f};
}

void drawOutpostMarker(sf::RenderTarget& target, sf::Vector2f c, const Outpost& op, bool isCastle,
                       sf::Color fill, bool cutOff, float scale) {
    float r = (isCastle ? 17.f : 14.f) * scale;
    Ui::circle(target, c + sf::Vector2f(0.f, 2.f), r, sf::Color(0, 0, 0, 110));
    Ui::circle(target, c, r, fill, sf::Color(15, 18, 26), 2.f);
    Ui::circle(target, c, r - 4.f, sf::Color::Transparent, sf::Color(255, 255, 255, 60), 1.5f);

    if (isCastle) {
        for (int i = -1; i <= 1; i++)
            Ui::roundedRect(target, {{c.x + i * 6.f - 2.f, c.y - 7.f}, {4.f, 5.f}}, 1.f, sf::Color::White);
        Ui::roundedRect(target, {{c.x - 8.f, c.y - 3.f}, {16.f, 9.f}}, 2.f, sf::Color::White);
    } else {
        Ui::textCentered(target, std::to_string(op.severity), {c.x, c.y - 1.f}, static_cast<unsigned>(14 * scale),
                         sf::Color::White, true);
    }

    if (cutOff) {
        Ui::line(target, c + sf::Vector2f(-11.f, -11.f), c + sf::Vector2f(11.f, 11.f), 3.5f, Theme::Bad);
        Ui::line(target, c + sf::Vector2f(-11.f, 11.f), c + sf::Vector2f(11.f, -11.f), 3.5f, Theme::Bad);
    }

    unsigned size = static_cast<unsigned>(12 * scale);
    float w = Ui::textWidth(op.name, size, true);
    float x = std::max(2.f, c.x - w / 2.f);
    Ui::roundedRect(target, {{x - 4.f, c.y + r + 2.f}, {w + 8.f, size + 6.f}}, 4.f, sf::Color(10, 12, 20, 190));
    Ui::text(target, op.name, {x, c.y + r + 2.f}, size, cutOff ? sf::Color(255, 140, 140) : Theme::Text, true);
}

void SupplyOverlay::reset() {
    visible = true;
    raidPlaying = false;
    raidTime = 0.f;
}

void SupplyOverlay::startRaidAnimation(const RoadDestroyedEvent& event) {
    raidEvent = event;
    raidTime = 0.f;
    raidPlaying = true;
}

void SupplyOverlay::update(float realDt) {
    if (!raidPlaying) return;
    raidTime += realDt;
    if (raidTime > RAID_LENGTH) raidPlaying = false;
}

static void drawCastleIcon(sf::RenderTarget& target, sf::Vector2f c) {
    const sf::Color dark(30, 24, 10);
    Ui::roundedRect(target, {{c.x - 12.f, c.y - 8.f}, {24.f, 18.f}}, 3.f, Theme::Accent, dark, 2.f);
    for (int i = -1; i <= 1; i++)
        Ui::roundedRect(target, {{c.x + i * 8.f - 3.f, c.y - 14.f}, {6.f, 6.f}}, 1.f, Theme::Accent, dark, 2.f);
    Ui::roundedRect(target, {{c.x - 3.f, c.y + 1.f}, {6.f, 9.f}}, 2.f, dark);
}

void SupplyOverlay::drawMap(sf::RenderTarget& target, const SupplyNetwork& network, int hoveredRoad,
                            int hoveredOutpost) const {
    if (!visible || !network.isLoaded()) return;
    const Dataset& data = network.getData();
    const MstPlanner& planner = network.getPlanner();
    const std::vector<Road>& roads = planner.getRoads();

    auto ends = [&](int r) {
        return std::make_pair(outpostCentre(data.outposts[roads[r].from]), outpostCentre(data.outposts[roads[r].to]));
    };

    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        if (!roads[r].blocked) continue;
        auto [a, b] = ends(r);
        bool flashing = raidPlaying && r == raidEvent.road && raidTime < FLASH_TIME;
        if (flashing) {
            if (std::fmod(raidTime, 0.25f) < 0.17f) Ui::line(target, a, b, 7.f, sf::Color(255, 70, 60, 230));
        } else {
            Ui::line(target, a, b, 2.f, Theme::withAlpha(Theme::Bad, 120), 8.f, 6.f);
        }
        if (r == hoveredRoad) Ui::line(target, a, b, 8.f, sf::Color(255, 255, 255, 70));
    }

    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        if (roads[r].blocked || !planner.isInTree(r)) continue;
        auto [a, b] = ends(r);
        sf::Color color = Theme::Main;
        if (raidPlaying && r == raidEvent.replacement) {
            if (raidTime < FLASH_TIME) continue;
            float fade = Ui::clamp01((raidTime - (RAID_LENGTH - 0.5f)) / 0.5f);
            color = Theme::mix(Theme::Good, Theme::Main, fade);
        }
        Ui::line(target, a, b, 4.f, color);
        if (r == hoveredRoad) Ui::line(target, a, b, 8.f, sf::Color(255, 255, 255, 70));
    }

    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
        sf::Vector2f c = outpostCentre(data.outposts[o]);
        if (o == hoveredOutpost) Ui::circle(target, c, 14.f, sf::Color(255, 255, 255, 50), sf::Color::White, 2.f);
        if (o == data.castle) {
            drawCastleIcon(target, c);
        } else {
            bool connected = planner.isConnectedToCastle(o);
            Ui::circle(target, c, 7.f, connected ? Theme::Main : Theme::CutOff,
                       connected ? sf::Color(235, 240, 250) : sf::Color(90, 92, 100), 2.f);
        }
    }
}

void SupplyOverlay::drawTowerBadges(sf::RenderTarget& target, const std::vector<std::unique_ptr<Tower>>& towers) const {
    for (const auto& tower : towers) {
        int effect = tower->getSupplyEffect();
        if (effect == 0) continue;
        sf::Vector2f p = tower->getPosition() + sf::Vector2f(15.f, -15.f);
        Ui::circle(target, p, 5.f, effect > 0 ? Theme::Good : Theme::Bad, sf::Color(15, 18, 26), 1.5f);
    }
}

int SupplyOverlay::roadAt(const SupplyNetwork& network, sf::Vector2f world) const {
    if (!visible || !network.isLoaded()) return -1;
    const Dataset& data = network.getData();
    const MstPlanner& planner = network.getPlanner();
    const std::vector<Road>& roads = planner.getRoads();
    int best = -1;
    float bestDistance = 7.f;
    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        if (!roads[r].blocked && !planner.isInTree(r)) continue;
        float d = Ui::distanceToSegment(world, outpostCentre(data.outposts[roads[r].from]),
                                        outpostCentre(data.outposts[roads[r].to]));
        if (d < bestDistance) {
            best = r;
            bestDistance = d;
        }
    }
    return best;
}

int SupplyOverlay::outpostAt(const SupplyNetwork& network, sf::Vector2f world) const {
    if (!visible || !network.isLoaded()) return -1;
    const Dataset& data = network.getData();
    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
        sf::Vector2f c = outpostCentre(data.outposts[o]);
        if (std::hypot(world.x - c.x, world.y - c.y) <= 14.f) return o;
    }
    return -1;
}

void drawBomb(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, float t, float time) {
    t = Ui::clamp01(t);
    float height = std::max(50.f, 0.35f * std::hypot(to.x - from.x, to.y - from.y));
    auto arc = [&](float s) {
        sf::Vector2f p = from + (to - from) * s;
        p.y -= height * 4.f * s * (1.f - s);
        return p;
    };

    float pulse = 0.5f + 0.5f * std::sin(time * 12.f);
    Ui::circle(target, to, 11.f + 4.f * pulse, sf::Color(255, 70, 60, 40), sf::Color(255, 70, 60, 220), 2.5f);

    for (int i = 1; i <= 14; i++)
        Ui::circle(target, arc(t * i / 14.f), 2.6f, sf::Color(170, 45, 25, static_cast<std::uint8_t>(60 + 12 * i)));
    Ui::circle(target, from + (to - from) * t, 4.f + 3.f * t, sf::Color(0, 0, 0, 80));

    sf::Vector2f p = arc(t);
    Ui::circle(target, p, 9.f, sf::Color(38, 38, 46), sf::Color(255, 130, 60), 2.f);
    Ui::circle(target, p + sf::Vector2f(5.f, -9.f), 2.5f + 2.f * pulse, sf::Color(255, 220, 90));
}
