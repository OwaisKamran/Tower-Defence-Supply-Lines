#pragma once
#include <istream>
#include <string>
#include <vector>
#include "dataset.hpp"

bool loadDataset(const std::string& path, Dataset& out, std::string& error);
bool parseDataset(std::istream& in, Dataset& out, std::string& error);

Dataset withoutOutposts(const Dataset& data, const std::vector<int>& removed);
