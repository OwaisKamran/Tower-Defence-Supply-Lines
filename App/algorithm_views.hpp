#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include "algorithm_trace.hpp"
#include "supply_network.hpp"

class AlgorithmViews {
public:
    enum class Tab { Kruskal, Raid, Convoys };
    enum class Action { None, Close };

    void open(Tab tab, const SupplyNetwork& network);
    void close() { isOpenFlag = false; }
    bool isOpen() const { return isOpenFlag; }
    Tab getTab() const { return tab; }

    void recordRaid(const MstPlanner& before, const RaidTrace& trace, const RoadDestroyedEvent& event);
    void clearRaid() { raidBefore.reset(); }

    Action handleEvent(const sf::Event& event, sf::Vector2f mouse, const SupplyNetwork& network);
    void update(float realDt);
    void draw(sf::RenderTarget& target, const SupplyNetwork& network, sf::Vector2f mouse) const;

private:
    bool isOpenFlag = false;
    Tab tab = Tab::Kruskal;
    float openAnim = 0.f;

    int step = 0;
    float progress = 0.f;
    bool playing = true;
    float speed = 0.35f;
    bool draggingSlider = false;

    KruskalTrace kruskal;
    std::vector<Road> kruskalRoads;
    std::optional<MstPlanner> raidBefore;
    RaidTrace raid;
    RoadDestroyedEvent raidEvent;
    std::vector<LoadStep> loading;
    PackingResult packing;
    std::vector<sf::FloatRect> loadTarget;

    int stepCount() const;
    float stepDuration() const;
    void restartPlayback();
    void buildLoadTargets(const Dataset& data);

    sf::FloatRect tabRect(int index) const;
    sf::FloatRect closeRect() const;
    sf::FloatRect controlRect(int index) const;
    sf::FloatRect sliderRect() const;
    sf::FloatRect itemRowRect(int index) const;

    void drawFrame(sf::RenderTarget& target, sf::Vector2f mouse) const;
    void drawControls(sf::RenderTarget& target, sf::Vector2f mouse) const;
    void drawKruskal(sf::RenderTarget& target, const Dataset& data) const;
    void drawRaid(sf::RenderTarget& target, const Dataset& data) const;
    void drawConvoys(sf::RenderTarget& target, const Dataset& data) const;
};

void drawItemIcon(sf::RenderTarget& target, const std::string& name, sf::Vector2f centre, float size);
