#include "algorithm_views.hpp"
#include "supply_overlay.hpp"
#include "theme.hpp"
#include "ui_draw.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

using namespace Theme;

static const float HEADER_H = 56.f;
static const float SIDE_X = 960.f;

static const char* TAB_NAMES[] = {"Plan Network (Kruskal)", "Raid Repair (incremental)", "Load Convoys (DP)"};
static const sf::Color CONVOY_COLORS[] = {{176, 120, 70}, {70, 160, 150}, {150, 110, 200}};

void AlgorithmViews::open(Tab newTab, const SupplyNetwork& network) {
    if (!network.isLoaded()) return;
    if (!isOpenFlag) openAnim = 0.f;
    isOpenFlag = true;
    tab = newTab;

    kruskalRoads = network.getPlanner().getRoads();
    kruskal = traceKruskal(network.getData(), kruskalRoads);

    loading = traceConvoyLoading(network.getData(), network.getReport());
    packing = network.getReport().packing;
    buildLoadTargets(network.getData());

    restartPlayback();
}

void AlgorithmViews::recordRaid(const MstPlanner& before, const RaidTrace& trace, const RoadDestroyedEvent& event) {
    raidBefore.emplace(before);
    raid = trace;
    raidEvent = event;
    if (isOpenFlag && tab == Tab::Raid) restartPlayback();
}

void AlgorithmViews::restartPlayback() {
    step = 0;
    progress = 0.f;
    playing = true;
}

int AlgorithmViews::stepCount() const {
    switch (tab) {
        case Tab::Kruskal: return static_cast<int>(kruskal.steps.size());
        case Tab::Raid:
            if (!raidBefore) return 0;
            return raid.wasInTree ? 3 + static_cast<int>(raid.scan.size()) : 1;
        case Tab::Convoys: return static_cast<int>(loading.size());
    }
    return 0;
}

float AlgorithmViews::stepDuration() const {
    return 1.8f * std::pow(0.1f, speed);
}

void AlgorithmViews::update(float realDt) {
    if (!isOpenFlag) return;
    openAnim = std::min(1.f, openAnim + realDt / 0.25f);
    if (!playing || step >= stepCount()) return;
    progress += realDt / stepDuration();
    if (progress >= 1.f) {
        step++;
        progress = 0.f;
        if (step >= stepCount()) playing = false;
    }
}

sf::FloatRect AlgorithmViews::tabRect(int index) const {
    return {{16.f + index * 262.f, 10.f}, {252.f, 36.f}};
}
sf::FloatRect AlgorithmViews::closeRect() const { return {{WINDOW_W - 146.f, 10.f}, {130.f, 36.f}}; }
sf::FloatRect AlgorithmViews::controlRect(int index) const {
    const float widths[] = {88.f, 56.f, 56.f, 82.f};
    float x = SIDE_X + 12.f;
    for (int i = 0; i < index; i++) x += widths[i] + 6.f;
    return {{x, WINDOW_H - 108.f}, {widths[index], 36.f}};
}
sf::FloatRect AlgorithmViews::sliderRect() const { return {{SIDE_X + 24.f, WINDOW_H - 36.f}, {SIDEBAR_W - 48.f, 16.f}}; }

sf::FloatRect AlgorithmViews::itemRowRect(int index) const {
    float rowH = loading.empty() ? 31.f : std::min(31.f, 600.f / loading.size());
    return {{16.f, HEADER_H + 46.f + index * rowH}, {316.f, rowH - 3.f}};
}

AlgorithmViews::Action AlgorithmViews::handleEvent(const sf::Event& event, sf::Vector2f mouse, const SupplyNetwork& network) {
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        switch (key->code) {
            case sf::Keyboard::Key::Escape: return Action::Close;
            case sf::Keyboard::Key::Space:  playing = !playing; if (step >= stepCount()) restartPlayback(); break;
            case sf::Keyboard::Key::Right:  if (step < stepCount()) { step++; progress = 0.f; playing = false; } break;
            case sf::Keyboard::Key::Left:   if (step > 0) { step--; progress = 0.f; playing = false; } break;
            case sf::Keyboard::Key::Num1:   open(Tab::Kruskal, network); break;
            case sf::Keyboard::Key::Num2:   open(Tab::Raid, network); break;
            case sf::Keyboard::Key::Num3:   open(Tab::Convoys, network); break;
            default: break;
        }
    }
    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button != sf::Mouse::Button::Left) return Action::None;
        if (closeRect().contains(mouse)) return Action::Close;
        for (int i = 0; i < 3; i++)
            if (tabRect(i).contains(mouse)) open(static_cast<Tab>(i), network);
        if (controlRect(0).contains(mouse)) {
            if (step >= stepCount()) restartPlayback();
            else playing = !playing;
        }
        if (controlRect(1).contains(mouse) && step > 0) { step--; progress = 0.f; playing = false; }
        if (controlRect(2).contains(mouse) && step < stepCount()) { step++; progress = 0.f; playing = false; }
        if (controlRect(3).contains(mouse)) restartPlayback();
        sf::FloatRect s = sliderRect();
        if (sf::FloatRect({s.position.x - 10.f, s.position.y - 10.f}, {s.size.x + 20.f, s.size.y + 20.f}).contains(mouse))
            draggingSlider = true;
    }
    if (event.is<sf::Event::MouseButtonReleased>()) draggingSlider = false;
    if (draggingSlider && (event.is<sf::Event::MouseMoved>() || event.is<sf::Event::MouseButtonPressed>())) {
        sf::FloatRect s = sliderRect();
        speed = Ui::clamp01((mouse.x - s.position.x) / s.size.x);
    }
    return Action::None;
}

void AlgorithmViews::drawFrame(sf::RenderTarget& target, sf::Vector2f mouse) const {
    sf::RectangleShape bg({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
    bg.setFillColor(Background);
    target.draw(bg);

    sf::RectangleShape mapBg({GRID_W, WINDOW_H - HEADER_H});
    mapBg.setPosition({0.f, HEADER_H});
    mapBg.setFillColor(sf::Color(14, 19, 30));
    target.draw(mapBg);
    sf::VertexArray gridLines(sf::PrimitiveType::Lines);
    sf::Color gl(255, 255, 255, 14);
    for (float x = 0.f; x <= GRID_W; x += CELL) {
        gridLines.append(sf::Vertex{{x, HEADER_H}, gl});
        gridLines.append(sf::Vertex{{x, static_cast<float>(WINDOW_H)}, gl});
    }
    for (float y = HEADER_H - 4.f; y <= WINDOW_H; y += CELL) {
        gridLines.append(sf::Vertex{{0.f, y}, gl});
        gridLines.append(sf::Vertex{{GRID_W, y}, gl});
    }
    target.draw(gridLines);

    sf::RectangleShape header({static_cast<float>(WINDOW_W), HEADER_H});
    header.setFillColor(sf::Color(12, 15, 24));
    target.draw(header);
    sf::RectangleShape edge({static_cast<float>(WINDOW_W), 2.f});
    edge.setPosition({0.f, HEADER_H - 2.f});
    edge.setFillColor(Theme::withAlpha(Accent, 140));
    target.draw(edge);
    for (int i = 0; i < 3; i++)
        Ui::button(target, tabRect(i), std::to_string(i + 1) + "  " + TAB_NAMES[i], mouse, static_cast<int>(tab) == i, 15);
    Ui::button(target, closeRect(), "Close  (Esc)", mouse, false, 15);

    sf::RectangleShape side({SIDEBAR_W, WINDOW_H - HEADER_H});
    side.setPosition({SIDE_X, HEADER_H});
    side.setFillColor(sf::Color(17, 21, 33));
    target.draw(side);
}

void AlgorithmViews::drawControls(sf::RenderTarget& target, sf::Vector2f mouse) const {
    int count = stepCount();
    std::string play = step >= count ? "Replay" : (playing ? "Pause" : "Play");
    Ui::button(target, controlRect(0), play, mouse, playing && step < count, 15);
    Ui::button(target, controlRect(1), "<", mouse, false, 16);
    Ui::button(target, controlRect(2), ">", mouse, false, 16);
    Ui::button(target, controlRect(3), "Restart", mouse, false, 15);
    Ui::text(target, "Step " + std::to_string(std::min(step, count)) + " / " + std::to_string(count),
             {SIDE_X + 12.f, WINDOW_H - 140.f}, 14, TextDim, true);
    Ui::bar(target, {{SIDE_X + 100.f, WINDOW_H - 134.f}, {SIDEBAR_W - 116.f, 8.f}},
            count > 0 ? (std::min(step, count) + (step < count ? progress : 0.f)) / count : 0.f, Accent);
    char label[64];
    std::snprintf(label, sizeof(label), "Speed: %.2f s per step   (Space, arrow keys)", stepDuration());
    Ui::slider(target, sliderRect(), speed, label);
}

static float wrappedText(sf::RenderTarget& target, const std::string& s, sf::Vector2f pos, float maxWidth,
                         unsigned size, sf::Color color, bool bold = false) {
    std::string line, word;
    float y = pos.y;
    auto flush = [&]() {
        if (!line.empty()) Ui::text(target, line, {pos.x, y}, size, color, bold);
        y += size + 5.f;
        line.clear();
    };
    for (size_t i = 0; i <= s.size(); i++) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch != ' ' && ch != '\n') { word += ch; continue; }
        std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && Ui::textWidth(candidate, size, bold) > maxWidth) {
            flush();
            line = word;
        } else {
            line = candidate;
        }
        word.clear();
        if (ch == '\n') flush();
    }
    if (!line.empty()) flush();
    return y;
}

void AlgorithmViews::draw(sf::RenderTarget& target, const SupplyNetwork& network, sf::Vector2f mouse) const {
    if (!isOpenFlag || !network.isLoaded()) return;
    drawFrame(target, mouse);

    sf::View defaultView = target.getView();
    sf::View mapView(sf::FloatRect({0.f, 4.f}, {GRID_W, WINDOW_H - HEADER_H}));
    mapView.setViewport(sf::FloatRect({0.f, HEADER_H / WINDOW_H}, {GRID_W / WINDOW_W, (WINDOW_H - HEADER_H) / WINDOW_H}));

    const Dataset& data = network.getData();
    switch (tab) {
        case Tab::Kruskal:
            target.setView(mapView);
            drawKruskal(target, data);
            break;
        case Tab::Raid:
            target.setView(mapView);
            drawRaid(target, data);
            break;
        case Tab::Convoys:
            drawConvoys(target, data);
            break;
    }
    target.setView(defaultView);
    drawControls(target, mouse);

    if (openAnim < 1.f) {
        sf::RectangleShape fade({static_cast<float>(WINDOW_W), static_cast<float>(WINDOW_H)});
        fade.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(255 * (1.f - Ui::easeOut(openAnim)))));
        target.draw(fade);
    }
}

void AlgorithmViews::drawKruskal(sf::RenderTarget& target, const Dataset& data) const {
    const std::vector<Road>& roads = kruskalRoads;
    int count = static_cast<int>(kruskal.steps.size());
    bool examining = step < count;
    const KruskalStep* current = examining ? &kruskal.steps[step] : nullptr;
    float reveal = Ui::clamp01((progress - 0.45f) / 0.4f);
    float pulse = 0.5f + 0.5f * std::sin(progress * 18.f);

    std::vector<int> position(roads.size(), -1);
    for (size_t p = 0; p < kruskal.sortedRoads.size(); p++) position[kruskal.sortedRoads[p]] = static_cast<int>(p);

    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        sf::Vector2f a = outpostCentre(data.outposts[roads[r].from]);
        sf::Vector2f b = outpostCentre(data.outposts[roads[r].to]);
        int p = position[r];
        if (p < 0) {
            Ui::line(target, a, b, 2.5f, sf::Color(230, 60, 60, 150), 8.f, 7.f);
            continue;
        }
        if (p < step) {
            if (kruskal.steps[p].added) {
                Ui::line(target, a, b, 7.f, sf::Color(20, 60, 30));
                Ui::line(target, a, b, 4.5f, Good);
            } else {
                Ui::line(target, a, b, 2.f, Skipped, 6.f, 5.f);
            }
        } else if (p == step && examining) {
            Ui::line(target, a, b, 11.f, sf::Color(255, 255, 255, static_cast<std::uint8_t>(60 + 80 * pulse)));
            sf::Color verdict = current->added ? Good : Skipped;
            Ui::line(target, a, b, 5.f, Theme::mix(Accent, verdict, reveal));
        } else {
            Ui::line(target, a, b, 1.5f, sf::Color(255, 255, 255, 55));
        }
        sf::Vector2f mid = (a + b) / 2.f;
        Ui::circle(target, mid, 9.f, sf::Color(12, 14, 22, 230), p == step && examining ? Accent : Border, 1.f);
        Ui::textCentered(target, std::to_string(roads[r].cost), {mid.x, mid.y - 1.f}, 11, Text, true);
    }

    const std::vector<int>& groupNow = step == 0 ? kruskal.startGroup : kruskal.steps[std::min(step, count) - 1].group;
    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
        sf::Color colour = groupColor(groupNow[o]);
        bool merging = examining && current->added && current->group[o] != groupNow[o];
        if (merging) colour = Theme::mix(colour, groupColor(current->group[o]), reveal);
        sf::Vector2f c = outpostCentre(data.outposts[o]);
        if (merging && reveal > 0.f)
            Ui::circle(target, c, 18.f + 10.f * reveal, sf::Color::Transparent,
                       Theme::withAlpha(groupColor(current->group[o]), static_cast<std::uint8_t>(200 * (1.f - reveal))), 3.f);
        drawOutpostMarker(target, c, data.outposts[o], o == data.castle, colour, false);
    }

    sf::View mapView = target.getView();
    target.setView(target.getDefaultView());
    sf::FloatRect box({12.f, WINDOW_H - 128.f}, {330.f, 116.f});
    Ui::panel(target, box);
    std::string explain;
    if (examining) {
        const Road& road = roads[current->road];
        explain = roadLabel(data, road) + "  (cost " + std::to_string(road.cost) + ", severity " +
                  std::to_string(data.roadSeverity(road)) + "): ";
        if (reveal <= 0.f) explain += "are the two ends in different groups?";
        else if (current->added) explain += "different groups -> ADD to the tree, merge the groups.";
        else explain += "same group already -> SKIP, it would form a cycle.";
    } else {
        explain = "Done. Tree cost " + std::to_string(kruskal.totalCost) + ". ";
        if (count < static_cast<int>(kruskal.sortedRoads.size()))
            explain += "The tree had n - 1 roads after " + std::to_string(count) + " of " +
                       std::to_string(kruskal.sortedRoads.size()) + " roads, so Kruskal stopped early.";
        else
            explain += "All " + std::to_string(count) + " roads were examined; each group left is one tree of the forest.";
    }
    wrappedText(target, explain, {box.position.x + 14.f, box.position.y + 10.f}, box.size.x - 28.f, 14, Text);
    (void)mapView;

    float x = SIDE_X + 12.f, y = HEADER_H + 10.f;
    Ui::text(target, "KRUSKAL'S ALGORITHM", {x, y}, 17, Accent, true);
    y += 26.f;
    long long cost = step == 0 ? 0 : kruskal.steps[std::min(step, count) - 1].costSoFar;
    std::set<int> groups(groupNow.begin(), groupNow.end());
    int inTree = 0;
    for (int p = 0; p < std::min(step, count); p++) if (kruskal.steps[p].added) inTree++;
    auto statTile = [&](float tx, const std::string& label, const std::string& value) {
        Ui::roundedRect(target, {{tx, y}, {94.f, 46.f}}, 6.f, PanelLight);
        Ui::text(target, label, {tx + 8.f, y + 4.f}, 11, TextDim);
        Ui::text(target, value, {tx + 8.f, y + 17.f}, 21, Accent, true);
    };
    statTile(x, "TREE COST", std::to_string(cost));
    statTile(x + 100.f, "ROADS IN TREE", std::to_string(inTree));
    statTile(x + 200.f, "GROUPS (DSU)", std::to_string(groups.size()));
    y += 56.f;

    Ui::text(target, "Sorted: cost asc, severity desc, id asc", {x, y}, 12, TextDim);
    y += 18.f;
    Ui::text(target, "road", {x, y}, 11, TextDim, true);
    Ui::text(target, "cost", {x + 188.f, y}, 11, TextDim, true);
    Ui::text(target, "sev", {x + 220.f, y}, 11, TextDim, true);
    Ui::text(target, "result", {x + 250.f, y}, 11, TextDim, true);
    y += 16.f;
    float rowH = std::min(18.f, 470.f / std::max<size_t>(1, kruskal.sortedRoads.size() + kruskal.blockedRoads.size()));
    for (size_t p = 0; p < kruskal.sortedRoads.size(); p++) {
        int r = kruskal.sortedRoads[p];
        int pi = static_cast<int>(p);
        std::string result;
        sf::Color colour = TextDim;
        if (pi < step && pi < count) {
            result = kruskal.steps[p].added ? "ADD" : "skip";
            colour = kruskal.steps[p].added ? Good : Skipped;
        } else if (pi == step && examining) {
            Ui::roundedRect(target, {{x - 4.f, y - 1.f}, {SIDEBAR_W - 16.f, rowH}}, 4.f, Theme::withAlpha(Accent, 60), Accent, 1.f);
            result = reveal > 0.f ? (current->added ? "ADD" : "skip") : "?";
            colour = Text;
        } else if (pi >= count && step >= count) {
            result = "unused";
        }
        std::string name = roadLabel(data, roads[r]);
        if (name.size() > 27) name = name.substr(0, 26) + ".";
        Ui::text(target, name, {x, y}, 12, colour);
        Ui::text(target, std::to_string(roads[r].cost), {x + 194.f, y}, 12, colour);
        Ui::text(target, std::to_string(data.roadSeverity(roads[r])), {x + 222.f, y}, 12, colour);
        Ui::text(target, result, {x + 250.f, y}, 12, colour, true);
        y += rowH;
    }
    for (int r : kruskal.blockedRoads) {
        Ui::text(target, roadLabel(data, roads[r]), {x, y}, 12, Theme::withAlpha(Bad, 200));
        Ui::text(target, "destroyed", {x + 230.f, y}, 12, Theme::withAlpha(Bad, 200), true);
        y += rowH;
    }
    target.setView(mapView);
}

static std::string scanVerdictText(ScanVerdict v) {
    switch (v) {
        case ScanVerdict::JustDestroyed: return "destroyed now";
        case ScanVerdict::InTree:        return "tree road";
        case ScanVerdict::Blocked:       return "destroyed";
        case ScanVerdict::SameSide:      return "same side";
        case ScanVerdict::OtherTree:     return "other tree";
        case ScanVerdict::Chosen:        return "CROSSES: chosen";
    }
    return "";
}

void AlgorithmViews::drawRaid(sf::RenderTarget& target, const Dataset& data) const {
    sf::View mapView = target.getView();
    if (!raidBefore) {
        target.setView(target.getDefaultView());
        Ui::textCentered(target, "No raid yet", {GRID_W / 2.f, 330.f}, 30, Accent, true);
        Ui::textCentered(target, "Press D to destroy the next road and watch the incremental repair.",
                         {GRID_W / 2.f, 372.f}, 17, Text);
        Ui::text(target, "INCREMENTAL REPAIR", {SIDE_X + 12.f, HEADER_H + 10.f}, 17, Accent, true);
        wrappedText(target, "When enemies destroy a tree road, the tree splits into two sides. The sorted list "
                            "from the last full build is scanned and the first road with one end on each side "
                            "reconnects them (cut property). No re-sort is needed: O(n + m) instead of O(m log n).",
                    {SIDE_X + 12.f, HEADER_H + 44.f}, SIDEBAR_W - 24.f, 14, TextDim);
        target.setView(mapView);
        return;
    }

    const MstPlanner& before = *raidBefore;
    const std::vector<Road>& roads = before.getRoads();
    int count = stepCount();
    int state = std::min(step, count);
    bool destroyed = state >= 1;
    bool sides = raid.wasInTree && state >= 2;
    int scanned = raid.wasInTree ? std::max(0, std::min<int>(state - 2, raid.scan.size())) : 0;
    bool finished = state >= count;
    float pulse = 0.5f + 0.5f * std::sin(progress * 18.f);

    std::vector<int> verdictOf(roads.size(), -1);
    for (int i = 0; i < scanned; i++) verdictOf[raid.scan[i].road] = static_cast<int>(raid.scan[i].verdict);
    int checking = (raid.wasInTree && state >= 2 && state - 2 < static_cast<int>(raid.scan.size()) && !finished)
                   ? raid.scan[state - 2].road : -1;

    for (int r = 0; r < static_cast<int>(roads.size()); r++) {
        sf::Vector2f a = outpostCentre(data.outposts[roads[r].from]);
        sf::Vector2f b = outpostCentre(data.outposts[roads[r].to]);
        bool isRaided = r == raid.road;
        if (isRaided && destroyed) {
            Ui::line(target, a, b, 3.f, Bad, 8.f, 6.f);
            sf::Vector2f m = (a + b) / 2.f;
            Ui::line(target, m + sf::Vector2f(-10.f, -10.f), m + sf::Vector2f(10.f, 10.f), 4.f, Bad);
            Ui::line(target, m + sf::Vector2f(-10.f, 10.f), m + sf::Vector2f(10.f, -10.f), 4.f, Bad);
            continue;
        }
        if (roads[r].blocked) {
            Ui::line(target, a, b, 2.5f, sf::Color(230, 60, 60, 130), 8.f, 7.f);
            continue;
        }
        if (before.isInTree(r)) {
            sf::Color colour = Accent;
            if (sides) {
                int s = raid.side[roads[r].from];
                colour = s == 0 ? SideA : s == 1 ? SideB : Skipped;
            }
            Ui::line(target, a, b, 4.5f, colour);
            if (isRaided) Ui::line(target, a, b, 12.f, sf::Color(255, 60, 60, static_cast<std::uint8_t>(80 + 120 * pulse)));
        } else {
            sf::Color colour(255, 255, 255, 55);
            float width = 1.5f;
            if (verdictOf[r] == static_cast<int>(ScanVerdict::SameSide) ||
                verdictOf[r] == static_cast<int>(ScanVerdict::OtherTree)) { colour = Skipped; width = 2.f; }
            if (verdictOf[r] == static_cast<int>(ScanVerdict::Chosen)) {
                Ui::line(target, a, b, 12.f, Theme::withAlpha(Good, 90));
                colour = Good;
                width = 6.f;
            }
            Ui::line(target, a, b, width, colour, colour == Skipped ? 6.f : 0.f, 5.f);
        }
        if (r == checking) Ui::line(target, a, b, 10.f, sf::Color(255, 255, 255, static_cast<std::uint8_t>(90 + 120 * pulse)));
        sf::Vector2f mid = (a + b) / 2.f;
        Ui::circle(target, mid, 9.f, sf::Color(12, 14, 22, 230), Border, 1.f);
        Ui::textCentered(target, std::to_string(roads[r].cost), {mid.x, mid.y - 1.f}, 11, Text, true);
    }

    for (int o = 0; o < static_cast<int>(data.outposts.size()); o++) {
        sf::Color colour = o == data.castle ? Info : sf::Color(90, 100, 120);
        if (sides && raid.side[o] >= 0) colour = raid.side[o] == 0 ? SideA : SideB;
        drawOutpostMarker(target, outpostCentre(data.outposts[o]), data.outposts[o], o == data.castle, colour,
                          !before.isConnectedToCastle(o));
    }

    target.setView(target.getDefaultView());
    if (finished) {
        sf::FloatRect box({12.f, WINDOW_H - 112.f}, {330.f, 100.f});
        Ui::roundedRect(target, box, 10.f, sf::Color(10, 12, 20, 225), Accent, 2.f);
        Ui::textCentered(target, "ops: " + std::to_string(raidEvent.incrementalOps),
                         {box.position.x + box.size.x / 2.f, box.position.y + 30.f}, 28, Accent, true);
        Ui::textCentered(target, "vs rebuild: " + std::to_string(raidEvent.rebuildOps),
                         {box.position.x + box.size.x / 2.f, box.position.y + 68.f}, 22, Text, true);
    }

    float x = SIDE_X + 12.f, y = HEADER_H + 10.f, w = SIDEBAR_W - 24.f;
    Ui::text(target, "INCREMENTAL REPAIR", {x, y}, 17, Accent, true);
    y += 28.f;
    Ui::text(target, "Destroyed: " + roadLabel(data, roads[raid.road]), {x, y}, 13, Bad, true);
    y += 22.f;

    std::string explain;
    if (state == 0) explain = "Enemies attack the road closest to their path (Castle roads are protected while other tree roads remain).";
    else if (!raid.wasInTree) explain = "The road was not in the tree, so the tree is still a minimum spanning tree. Cost: O(1).";
    else if (state == 1) explain = "A tree road is gone, so the tree has split into two parts.";
    else if (!finished) explain = "BFS from one end marks side A (blue); the rest is side B (pink). Scan the list sorted at build time for the first unused road with one end on each side.";
    else if (raid.replacement >= 0) explain = "The first crossing road is the cheapest way to reconnect (cut property), so it joins the tree. Same tree as a full rebuild.";
    else explain = "No road crosses the cut: the tree stays split and side A is cut off from the Castle.";
    y = wrappedText(target, explain, {x, y}, w, 14, Text) + 8.f;

    if (raid.wasInTree) {
        Ui::text(target, "Scan of the sorted list", {x, y}, 13, TextDim, true);
        y += 20.f;
        int first = std::max(0, scanned - 12);
        for (int i = first; i < scanned; i++) {
            const RaidScanStep& s = raid.scan[i];
            sf::Color c = s.verdict == ScanVerdict::Chosen ? Good : TextDim;
            std::string name = roadLabel(data, roads[s.road]);
            if (name.size() > 24) name = name.substr(0, 23) + ".";
            Ui::text(target, name, {x, y}, 12, c);
            Ui::textRight(target, scanVerdictText(s.verdict), {x + w, y}, 12, c, s.verdict == ScanVerdict::Chosen);
            y += 17.f;
        }
        if (checking >= 0) {
            Ui::roundedRect(target, {{x - 4.f, y - 1.f}, {w + 8.f, 18.f}}, 4.f, Theme::withAlpha(Accent, 60), Accent, 1.f);
            Ui::text(target, roadLabel(data, roads[checking]), {x, y}, 12, Text);
            Ui::textRight(target, "checking...", {x + w, y}, 12, Text, true);
            y += 17.f;
        }
    }

    if (finished) {
        y = std::max(y + 10.f, static_cast<float>(WINDOW_H) - 250.f);
        Ui::text(target, "Operations", {x, y}, 13, TextDim, true);
        y += 22.f;
        float frac = raidEvent.rebuildOps > 0 ? static_cast<float>(raidEvent.incrementalOps) / raidEvent.rebuildOps : 0.f;
        Ui::text(target, "incremental", {x, y}, 12, TextDim);
        Ui::bar(target, {{x + 80.f, y + 3.f}, {(w - 120.f) * frac, 10.f}}, 1.f, Good);
        Ui::text(target, std::to_string(raidEvent.incrementalOps), {x + 86.f + (w - 120.f) * frac, y}, 12, Good, true);
        y += 20.f;
        Ui::text(target, "full rebuild", {x, y}, 12, TextDim);
        Ui::bar(target, {{x + 80.f, y + 3.f}, {w - 120.f, 10.f}}, 1.f, Bad);
        Ui::text(target, std::to_string(raidEvent.rebuildOps), {x + 86.f + w - 120.f, y}, 12, Bad, true);
        y += 20.f;
        Ui::text(target, raidEvent.matchesRebuild ? "Result matches the full rebuild" : "DIFFERS from the rebuild",
                 {x, y}, 12, raidEvent.matchesRebuild ? Good : Bad, true);
    }
    target.setView(mapView);
}

void drawItemIcon(sf::RenderTarget& target, const std::string& name, sf::Vector2f c, float size) {
    float h = size / 2.f;
    if (name.find("Ammo") != std::string::npos) {
        Ui::roundedRect(target, {{c.x - h, c.y - h * 0.8f}, {size, size * 0.8f}}, 2.f, sf::Color(170, 115, 60), sf::Color(90, 55, 25), 1.5f);
        Ui::line(target, {c.x - h, c.y - h * 0.8f}, {c.x + h, c.y + h * 0.8f}, 1.5f, sf::Color(90, 55, 25));
        Ui::line(target, {c.x + h, c.y - h * 0.8f}, {c.x - h, c.y + h * 0.8f}, 1.5f, sf::Color(90, 55, 25));
    } else if (name.find("Medkit") != std::string::npos) {
        Ui::roundedRect(target, {{c.x - h, c.y - h}, {size, size}}, 3.f, sf::Color(240, 240, 245), sf::Color(150, 150, 160), 1.f);
        Ui::roundedRect(target, {{c.x - h * 0.2f, c.y - h * 0.65f}, {h * 0.4f, h * 1.3f}}, 1.f, Bad);
        Ui::roundedRect(target, {{c.x - h * 0.65f, c.y - h * 0.2f}, {h * 1.3f, h * 0.4f}}, 1.f, Bad);
    } else if (name.find("Repair") != std::string::npos) {
        Ui::roundedRect(target, {{c.x - h, c.y - h * 0.5f}, {size, size * 0.8f}}, 2.f, sf::Color(220, 120, 40), sf::Color(120, 60, 20), 1.5f);
        Ui::roundedRect(target, {{c.x - h * 0.4f, c.y - h * 0.85f}, {h * 0.8f, h * 0.4f}}, 2.f, sf::Color::Transparent, sf::Color(120, 60, 20), 2.f);
    } else if (name.find("Frost") != std::string::npos) {
        sf::ConvexShape gem(4);
        gem.setPoint(0, {c.x, c.y - h});
        gem.setPoint(1, {c.x + h * 0.7f, c.y});
        gem.setPoint(2, {c.x, c.y + h});
        gem.setPoint(3, {c.x - h * 0.7f, c.y});
        gem.setFillColor(Ice);
        gem.setOutlineColor(sf::Color(40, 120, 200));
        gem.setOutlineThickness(1.5f);
        target.draw(gem);
    } else if (name.find("Cannon") != std::string::npos) {
        for (int i = -1; i <= 1; i++)
            Ui::circle(target, {c.x + i * h * 0.6f, c.y + (i == 0 ? -h * 0.35f : h * 0.25f)}, h * 0.42f,
                       sf::Color(70, 74, 86), sf::Color(30, 30, 36), 1.f);
    } else {
        Ui::roundedRect(target, {{c.x - h, c.y - h}, {size, size}}, 3.f, sf::Color(150, 110, 200));
    }
}

void AlgorithmViews::buildLoadTargets(const Dataset& data) {
    loadTarget.clear();
    int used[CONVOY_COUNT] = {0, 0, 0};
    int behind = 0;
    for (const LoadStep& s : loading) {
        if (s.convoy >= 0) {
            float boxX = 372.f, boxW = 430.f, boxY = HEADER_H + 64.f + s.convoy * 150.f;
            float cap = static_cast<float>(data.convoys[s.convoy].capacity);
            loadTarget.push_back({{boxX + boxW * used[s.convoy] / cap + 1.f, boxY + 4.f},
                                  {boxW * s.item.weight / cap - 2.f, 62.f}});
            used[s.convoy] += s.item.weight;
        } else {
            loadTarget.push_back({{360.f + (behind % 4) * 148.f, HEADER_H + 548.f + (behind / 4) * 46.f}, {142.f, 40.f}});
            behind++;
        }
    }
}

void AlgorithmViews::drawConvoys(sf::RenderTarget& target, const Dataset& data) const {
    int count = static_cast<int>(loading.size());
    int done = std::min(step, count);
    bool moving = step < count && playing;
    float t = Ui::easeOut(progress);

    Ui::text(target, "REQUESTS  (backtracking order i = 0..N-1)", {16.f, HEADER_H + 14.f}, 13, Accent, true);
    for (int i = 0; i < count; i++) {
        const LoadStep& s = loading[i];
        sf::FloatRect row = itemRowRect(i);
        bool current = i == step;
        Ui::roundedRect(target, row, 5.f, current ? Theme::withAlpha(Accent, 50) : PanelLight,
                        current ? Accent : sf::Color::Transparent, 1.f);
        sf::Color c = i < done ? TextDim : Text;
        if (!(i < done || (current && moving && t > 0.05f)))
            drawItemIcon(target, s.item.name, {row.position.x + 15.f, row.position.y + row.size.y / 2.f}, 18.f);
        Ui::text(target, "I" + std::to_string(s.item.id) + " " + s.item.name, {row.position.x + 32.f, row.position.y + 4.f}, 13, c, true);
        Ui::text(target, "w" + std::to_string(s.item.weight) + " v" + std::to_string(s.item.value),
                 {row.position.x + 168.f, row.position.y + 4.f}, 13, c);
        std::string where = data.outposts[s.item.outpost].name;
        if (i < done) where = s.convoy >= 0 ? "-> " + data.convoys[s.convoy].name : leftReasonText(s.reason);
        sf::Color whereColour = i < done ? (s.convoy >= 0 ? Good : Bad) : TextDim;
        Ui::textRight(target, where, {row.position.x + row.size.x - 8.f, row.position.y + 4.f}, 12, whereColour);
    }

    int load[CONVOY_COUNT] = {0, 0, 0}, value[CONVOY_COUNT] = {0, 0, 0};
    for (int i = 0; i < done; i++)
        if (loading[i].convoy >= 0) {
            load[loading[i].convoy] += loading[i].item.weight;
            value[loading[i].convoy] += loading[i].item.value;
        }
    for (int j = 0; j < CONVOY_COUNT; j++) {
        float y = HEADER_H + 40.f + j * 150.f;
        const Convoy& convoy = data.convoys[j];
        Ui::text(target, convoy.name, {372.f, y}, 17, Text, true);
        Ui::text(target, "capacity " + std::to_string(convoy.capacity), {372.f + Ui::textWidth(convoy.name, 17, true) + 10.f, y + 3.f}, 13, TextDim);
        Ui::textRight(target, "load " + std::to_string(load[j]) + " / " + std::to_string(convoy.capacity) +
                      "    value " + std::to_string(value[j]), {802.f, y + 2.f}, 14, Accent, true);

        sf::FloatRect box({372.f, y + 24.f}, {430.f, 70.f});
        Ui::roundedRect(target, box, 6.f, sf::Color(26, 32, 48), CONVOY_COLORS[j], 3.f);
        float fill = static_cast<float>(load[j]) / convoy.capacity;
        Ui::roundedRect(target, {{box.position.x, box.position.y + box.size.y + 4.f}, {box.size.x * fill, 6.f}}, 3.f,
                        fill >= 1.f ? Good : Accent);
        Ui::roundedRect(target, {{808.f, y + 44.f}, {74.f, 50.f}}, 8.f, CONVOY_COLORS[j], sf::Color(15, 18, 26), 2.f);
        Ui::roundedRect(target, {{842.f, y + 50.f}, {32.f, 20.f}}, 4.f, sf::Color(170, 210, 240));
        for (float wx : {410.f, 520.f, 740.f, 848.f})
            Ui::circle(target, {wx, y + 114.f}, 10.f, sf::Color(30, 30, 36), sf::Color(120, 120, 130), 3.f);
    }

    auto drawPlaced = [&](int i, sf::FloatRect r) {
        const LoadStep& s = loading[i];
        if (s.convoy >= 0) {
            Ui::roundedRect(target, r, 4.f, Theme::withAlpha(CONVOY_COLORS[s.convoy], 120), CONVOY_COLORS[s.convoy], 1.f);
            drawItemIcon(target, s.item.name, {r.position.x + r.size.x / 2.f, r.position.y + 22.f}, std::min(24.f, r.size.x - 6.f));
            Ui::textCentered(target, std::to_string(s.item.weight), {r.position.x + r.size.x / 2.f, r.position.y + 48.f}, 12, Text, true);
        } else {
            Ui::roundedRect(target, r, 5.f, PanelLight, Theme::withAlpha(Bad, 160), 1.f);
            drawItemIcon(target, s.item.name, {r.position.x + 16.f, r.position.y + r.size.y / 2.f}, 18.f);
            Ui::text(target, s.item.name, {r.position.x + 32.f, r.position.y + 3.f}, 12, Text, true);
            Ui::text(target, leftReasonText(s.reason), {r.position.x + 32.f, r.position.y + 20.f}, 12, Bad);
        }
    };
    Ui::text(target, "LEFT BEHIND", {360.f, HEADER_H + 524.f}, 13, Bad, true);
    for (int i = 0; i < done; i++) drawPlaced(i, loadTarget[i]);

    if (step < count && (moving || progress > 0.f)) {
        sf::FloatRect from = itemRowRect(step);
        sf::FloatRect to = loadTarget[step];
        sf::Vector2f a(from.position.x + 15.f, from.position.y + from.size.y / 2.f);
        sf::Vector2f b(to.position.x + to.size.x / 2.f, to.position.y + to.size.y / 2.f);
        sf::Vector2f control((a.x + b.x) / 2.f, std::min(a.y, b.y) - 80.f);
        sf::Vector2f p = a * (1 - t) * (1 - t) + control * 2.f * t * (1 - t) + b * t * t;
        Ui::circle(target, p, 18.f, Theme::withAlpha(Accent, 60));
        drawItemIcon(target, loading[step].item.name, p, 24.f + 6.f * std::sin(t * 3.14159f));
    }

    float x = SIDE_X + 12.f, y = HEADER_H + 10.f, w = SIDEBAR_W - 24.f;
    Ui::text(target, "MULTIPLE KNAPSACK DP", {x, y}, 17, Accent, true);
    y += 28.f;
    int total = 0;
    for (int j = 0; j < CONVOY_COUNT; j++) total += value[j];
    Ui::roundedRect(target, {{x, y}, {w, 64.f}}, 8.f, PanelLight);
    Ui::text(target, "TOTAL VALUE DELIVERED", {x + 10.f, y + 6.f}, 12, TextDim, true);
    Ui::text(target, std::to_string(total), {x + 10.f, y + 22.f}, 32, Accent, true);
    Ui::textRight(target, "optimum " + std::to_string(packing.totalValue), {x + w - 10.f, y + 38.f}, 13, TextDim);
    y += 76.f;

    std::string explain;
    if (step < count) {
        const LoadStep& s = loading[step];
        explain = "I" + std::to_string(s.item.id) + " " + s.item.name + " (w " + std::to_string(s.item.weight) +
                  ", v " + std::to_string(s.item.value) + ") for " + data.outposts[s.item.outpost].name + ": ";
        if (s.reason == LeftReason::CutOff) explain += "the outpost is cut off from the Castle, so the DP never sees this request.";
        else if (s.reason == LeftReason::TooHeavy) explain += "heavier than every convoy; only 'skip' is possible.";
        else if (s.convoy < 0) explain += "dp[i][c] = dp[i+1][c], so the best plan skips it (the space is worth more to other items).";
        else explain += "dp[i][c] = v + dp[i+1][c - w on " + data.convoys[s.convoy].name + "], so it goes on the " +
                        data.convoys[s.convoy].name + ".";
    } else {
        explain = "Done. Walking the table from i = 0 with the full capacities rebuilt the optimal plan: total value " +
                  std::to_string(packing.totalValue) + ".";
    }
    y = wrappedText(target, explain, {x, y}, w, 14, Text) + 12.f;

    Ui::text(target, "State: dp[i][c1][c2][c3]", {x, y}, 13, TextDim, true);
    y += 20.f;
    Ui::text(target, "Table cells: " + std::to_string(packing.states), {x, y}, 13, TextDim);
    y += 18.f;
    Ui::text(target, "Choices examined: " + std::to_string(packing.operations), {x, y}, 13, TextDim);
    y += 18.f;
    Ui::text(target, "Each item is decided once (one layer i),", {x, y}, 13, TextDim);
    y += 18.f;
    Ui::text(target, "so it is never loaded twice.", {x, y}, 13, TextDim);
}
