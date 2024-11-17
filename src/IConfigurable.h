#pragma once
#include "Config.h"

class IConfigurable {
protected:
    Config& config;

    IConfigurable() : config(Config::getInstance()) {}
};