#include "dataset_loader.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

static std::string cleanLine(const std::string& line) {
    std::string s = line.substr(0, line.find('#'));
    size_t first = s.find_first_not_of(" \t\r");
    if (first == std::string::npos) return "";
    size_t last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

static bool fail(std::string& error, int lineNo, const std::string& message) {
    error = "line " + std::to_string(lineNo) + ": " + message;
    return false;
}

bool loadDataset(const std::string& path, Dataset& out, std::string& error) {
    std::ifstream file(path);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    return parseDataset(file, out, error);
}

bool parseDataset(std::istream& in, Dataset& out, std::string& error) {
    out = Dataset();
    std::unordered_map<int, int> outpostIndex;
    std::unordered_set<int> roadIds, convoyIds, itemIds;
    std::vector<int> roadLines, itemLines;
    int castleId = -1, castleLine = 0;

    std::string raw, section;
    int lineNo = 0;
    while (std::getline(in, raw)) {
        lineNo++;
        std::string line = cleanLine(raw);
        if (line.empty()) continue;

        if (line == "OUTPOSTS" || line == "ROADS" || line == "CONVOYS" || line == "ITEMS") {
            section = line;
            continue;
        }

        std::istringstream ss(line);
        if (line.rfind("CASTLE", 0) == 0) {
            std::string keyword;
            if (!(ss >> keyword >> castleId)) return fail(error, lineNo, "expected: CASTLE <outpostId>");
            castleLine = lineNo;
            continue;
        }

        if (section == "OUTPOSTS") {
            Outpost o;
            if (!(ss >> o.id >> o.name >> o.gridX >> o.gridY >> o.severity >> o.population))
                return fail(error, lineNo, "expected: id name gridX gridY severity population");
            if (outpostIndex.count(o.id)) return fail(error, lineNo, "duplicate outpost id");
            if (o.severity < 0 || o.severity > 10) return fail(error, lineNo, "severity must be 0-10");
            if (o.population < 0) return fail(error, lineNo, "population must be >= 0");
            outpostIndex[o.id] = static_cast<int>(out.outposts.size());
            out.outposts.push_back(o);
        }
        else if (section == "ROADS") {
            Road r;
            int blocked;
            if (!(ss >> r.id >> r.from >> r.to >> r.cost >> blocked))
                return fail(error, lineNo, "expected: id fromOutpostId toOutpostId cost blocked(0/1)");
            if (!roadIds.insert(r.id).second) return fail(error, lineNo, "duplicate road id");
            if (r.cost < 0) return fail(error, lineNo, "road cost must be >= 0");
            if (blocked != 0 && blocked != 1) return fail(error, lineNo, "blocked must be 0 or 1");
            r.blocked = (blocked == 1);
            out.roads.push_back(r);
            roadLines.push_back(lineNo);
        }
        else if (section == "CONVOYS") {
            Convoy c;
            if (!(ss >> c.id >> c.name >> c.capacity))
                return fail(error, lineNo, "expected: id name capacity");
            if (!convoyIds.insert(c.id).second) return fail(error, lineNo, "duplicate convoy id");
            if (c.capacity <= 0) return fail(error, lineNo, "capacity must be > 0");
            out.convoys.push_back(c);
        }
        else if (section == "ITEMS") {
            SupplyItem it;
            if (!(ss >> it.id >> it.name >> it.weight >> it.value >> it.outpost))
                return fail(error, lineNo, "expected: id name weight value requestingOutpostId");
            if (!itemIds.insert(it.id).second) return fail(error, lineNo, "duplicate item id");
            if (it.weight <= 0) return fail(error, lineNo, "weight must be > 0");
            if (it.value < 0) return fail(error, lineNo, "value must be >= 0");
            out.items.push_back(it);
            itemLines.push_back(lineNo);
        }
        else {
            return fail(error, lineNo, "data line outside a section");
        }
    }

    for (size_t i = 0; i < out.roads.size(); i++) {
        Road& r = out.roads[i];
        auto a = outpostIndex.find(r.from);
        auto b = outpostIndex.find(r.to);
        if (a == outpostIndex.end() || b == outpostIndex.end())
            return fail(error, roadLines[i], "road refers to an unknown outpost");
        if (a->second == b->second)
            return fail(error, roadLines[i], "road connects an outpost to itself");
        r.from = a->second;
        r.to = b->second;
    }
    for (size_t i = 0; i < out.items.size(); i++) {
        auto o = outpostIndex.find(out.items[i].outpost);
        if (o == outpostIndex.end())
            return fail(error, itemLines[i], "item requested by an unknown outpost");
        out.items[i].outpost = o->second;
    }

    if (castleLine == 0) {
        error = "missing CASTLE line";
        return false;
    }
    auto c = outpostIndex.find(castleId);
    if (c == outpostIndex.end()) return fail(error, castleLine, "CASTLE refers to an unknown outpost");
    out.castle = c->second;
    if (out.outposts[out.castle].severity != 0)
        return fail(error, castleLine, "the Castle's severity must be 0");

    if (static_cast<int>(out.convoys.size()) != CONVOY_COUNT) {
        error = "exactly " + std::to_string(CONVOY_COUNT) + " convoys are required";
        return false;
    }
    return true;
}

Dataset withoutOutposts(const Dataset& data, const std::vector<int>& removed) {
    std::vector<bool> isRemoved(data.outposts.size(), false);
    for (int index : removed) isRemoved[index] = true;

    std::vector<int> newIndex(data.outposts.size(), -1);
    Dataset result;
    for (size_t i = 0; i < data.outposts.size(); i++) {
        if (isRemoved[i]) continue;
        newIndex[i] = static_cast<int>(result.outposts.size());
        result.outposts.push_back(data.outposts[i]);
    }

    for (Road r : data.roads) {
        if (newIndex[r.from] < 0 || newIndex[r.to] < 0) continue;
        r.from = newIndex[r.from];
        r.to = newIndex[r.to];
        result.roads.push_back(r);
    }

    for (SupplyItem it : data.items) {
        if (newIndex[it.outpost] < 0) continue;
        it.outpost = newIndex[it.outpost];
        result.items.push_back(it);
    }

    result.convoys = data.convoys;
    result.castle = newIndex[data.castle];
    return result;
}
