/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2021+ WarheadCore <https://github.com/WarheadCore>
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "Define.h"
#include <string>
#include <vector>
#include <stdexcept>

class ConfigMgr
{
    ConfigMgr() = default;
    ConfigMgr(ConfigMgr const&) = delete;
    ConfigMgr& operator=(ConfigMgr const&) = delete;
    ~ConfigMgr() = default;

public:
    bool LoadAppConfigs();
    bool LoadModulesConfigs();
    void Configure(std::string const& initFileName, std::vector<std::string> args, std::string const& modulesConfigList = "");

    static ConfigMgr* instance();

    bool Reload();

    std::string const& GetFilename();
    std::string const GetConfigPath();
    std::vector<std::string> const& GetArguments() const;
    std::vector<std::string> GetKeysByString(std::string const& name);

    template<class T>
    T GetOption(std::string const& name, T const& def, bool showLogs = true) const;

    /*
     * Deprecated geters. This geters will be deleted
     */

    // @deprecated DO NOT USE - use GetOption<std::string> instead.
    std::string GetStringDefault(std::string const& name, const std::string& def, bool showLogs = true);

    // @deprecated DO NOT USE - use GetOption<bool> instead.
    bool GetBoolDefault(std::string const& name, bool def, bool showLogs = true);

    // @deprecated DO NOT USE - use GetOption<int32> instead.
    int GetIntDefault(std::string const& name, int def, bool showLogs = true);

    // @deprecated DO NOT USE - use GetOption<float> instead.
    float GetFloatDefault(std::string const& name, float def, bool showLogs = true);

    /*
     * End deprecated geters
     */

    bool isDryRun() { return dryRun; }
    void setDryRun(bool mode) { dryRun = mode; }

private:
    /// Method used only for loading main configuration files (authserver.conf and worldserver.conf)
    bool LoadInitial(std::string const& file);
    bool LoadAdditionalFile(std::string file);

    template<class T>
    T GetValueDefault(std::string const& name, T const& def, bool showLogs = true) const;

    bool dryRun = false;
};

class ConfigException : public std::length_error
{
public:
    explicit ConfigException(std::string const& message) : std::length_error(message) { }
};

#define sConfigMgr ConfigMgr::instance()

#endif
