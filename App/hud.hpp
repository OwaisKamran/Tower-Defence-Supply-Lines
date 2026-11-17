#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "tower.hpp"
#include "supply_network.hpp"

class AssetManager;

struct TowerInfo {
    TowerType type;
    std::string name;
    int cost;
    float range;
    std::string description;
    std::string hotkey;
};
const TowerInfo& towerInfo(TowerType type);

struct HudState {
    int money = 0, lives = 0, wave = 0;
    int speed = 1;
    bool muted = false;
    TowerType selected = TowerType::Barrier;
    bool algorithmMenuOpen = false;
    std::string waveButton;
};

enum class HudButton {
    None, Tower0, Tower1, Tower2, Tower3,
    Algorithms,
    ViewKruskal, ViewRaid, ViewConvoys,
    Speed, Sound, StartWave
};

class Hud {
public:
    explicit Hud(AssetManager& assets);

    void drawTopBar(sf::RenderTarget& target, const HudState& state, sf::Vector2f mouse) const;
    void drawAlgorithmMenu(sf::RenderTarget& target, sf::Vector2f mouse) const;
    void drawSidebar(sf::RenderTarget& target, const HudState& state, const SupplyNetwork& network,
                     sf::Vector2f mouse) const;
    void drawWaveButton(sf::RenderTarget& target, const HudState& state, sf::Vector2f mouse) const;
    void drawWaveBanner(sf::RenderTarget& target, const std::string& title, const std::string& subtitle,
                        float age) const;
    void drawToast(sf::RenderTarget& target, const std::string& text, sf::Color color, float age, float length) const;
    void drawTooltip(sf::RenderTarget& target, const std::vector<std::string>& lines, sf::Vector2f mouse) const;
    void drawHelp(sf::RenderTarget& target, float openAmount, sf::Vector2f mouse) const;

    HudButton buttonAt(sf::Vector2f mouse, const HudState& state) const;
    sf::FloatRect supplyBoxRect() const;
    sf::FloatRect helpCloseRect(float openAmount) const;

private:
    AssetManager& assets;
    void drawTowerIcon(sf::RenderTarget& target, TowerType type, sf::Vector2f centre, float size) const;
    sf::FloatRect cardRect(int index) const;
    sf::FloatRect buttonRect(HudButton button) const;
};
