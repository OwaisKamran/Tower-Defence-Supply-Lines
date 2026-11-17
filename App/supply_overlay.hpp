#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>
#include "tower.hpp"
#include "supply_network.hpp"

class SupplyOverlay {
private:
    bool visible = true;

    bool raidPlaying = false;
    float raidTime = 0.f;
    RoadDestroyedEvent raidEvent;
    static constexpr float FLASH_TIME = 0.8f;
    static constexpr float RAID_LENGTH = 2.0f;

public:
    void toggle() { visible = !visible; }
    bool isVisible() const { return visible; }
    void reset();
    void update(float realDt);

    void startRaidAnimation(const RoadDestroyedEvent& event);
    bool isRaidPlaying() const { return raidPlaying; }

    void drawMap(sf::RenderTarget& target, const SupplyNetwork& network, int hoveredRoad, int hoveredOutpost) const;
    void drawTowerBadges(sf::RenderTarget& target, const std::vector<std::unique_ptr<Tower>>& towers) const;

    int roadAt(const SupplyNetwork& network, sf::Vector2f world) const;
    int outpostAt(const SupplyNetwork& network, sf::Vector2f world) const;
};

void drawOutpostMarker(sf::RenderTarget& target, sf::Vector2f centre, const Outpost& outpost, bool isCastle,
                       sf::Color fill, bool cutOff, float scale = 1.f);
sf::Vector2f outpostCentre(const Outpost& outpost);
