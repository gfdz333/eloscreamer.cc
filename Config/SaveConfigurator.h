#pragma once

#include <array>
#include <JsonForward.h>
#include <nlohmann/json.hpp>

#include "Configurable.h"

template <typename T>
struct SaveHandler {
    SaveHandler(const char* name, const T& variable, json& j)
        : name{ name }, variable{ variable }, j{ j }
    {
    }

    SaveHandler& def(const T& defaultValue)
    {
        if (variable == defaultValue)
            variableHasDefaultValue = true;
        return *this;
    }

    template <typename Functor>
    SaveHandler& loadString(Functor&&)
    {
        return *this;
    }

    template <typename Functor>
    SaveHandler& save(Functor&& functor)
    {
        if (!variableHasDefaultValue) {
            j.emplace(name, functor());
            saved = true;
        }
        return *this;
    }

    ~SaveHandler() noexcept
    {
        if (!variableHasDefaultValue && !saved)
            j.emplace(name, variable);
    }

private:
    const char* name;
    const T& variable;
    json& j;
    bool variableHasDefaultValue = false;
    bool saved = false;
};

template <typename T, std::size_t N>
struct SaveHandler<std::array<T, N>> {
    SaveHandler(const char* name, std::array<T, N>& variable, json& j)
        : name{ name }, variable{ variable }, j{ j }
    {
    }

    ~SaveHandler() noexcept;

private:
    const char* name;
    std::array<T, N>& variable;
    json& j;
};

struct SaveConfigurator {
    template <typename T>
    auto operator()(const char* name, T& variable)
    {
        if constexpr (Configurable<T, SaveConfigurator>) {
            SaveConfigurator configurator;
            variable.configure(configurator);
            j.emplace(name, configurator.getJson());
        } else {
            return SaveHandler<T>{ name, variable, j };
        }
    }

    [[nodiscard]] json getJson() const
    {
        return j;
    }

private:
    json j;
};

template <typename T, std::size_t N>
SaveHandler<std::array<T, N>>::~SaveHandler() noexcept
{
    auto arr = json::array();
    bool allEmpty = true;
    for (auto& element : variable) {
        if constexpr (Configurable<T, SaveConfigurator>) {
            SaveConfigurator configurator;
            element.configure(configurator);
            auto elementJson = configurator.getJson();
            if (!elementJson.empty())
                allEmpty = false;
            arr.push_back(std::move(elementJson));
        } else {
            arr.push_back(element);
        }
    }

    if (!allEmpty)
        j.emplace(name, std::move(arr));
}
