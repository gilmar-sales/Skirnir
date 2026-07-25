#pragma once

#include <Skirnir/Hotswap/module/Tracker.hpp>

#include <string>

class IPlugin
{
  public:
    virtual ~IPlugin() = default;
    virtual std::string Greet() = 0;
};

class Plugin : public IPlugin
{
    SKR_TRACK(Plugin, "hot_swap_demo::Plugin");

  public:
    skr_virtual ~Plugin() override = default;
    Plugin() = default;
    skr_virtual std::string Greet() override;
};
