#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <stdexcept>
#include <variant>
#include <type_traits>
#include "nlohmann/json.hpp"

// 简单的基类/派生类定义
class Dog {
public:
    Dog(nlohmann::json& j) {
        std::cout << "Dog created with config: " << j.dump() << "\n";
    }
    void bark() { std::cout << "Woof!\n"; }
};

class Cat {
public:
    Cat(nlohmann::json& j) {
        std::cout << "Cat created with config: " << j.dump() << "\n";
    }
    void meow() { std::cout << "Meow!\n"; }
};

// --- 通用工厂 ---
template <typename VariantType>
class GenericFactory {
private:
    using FactoryFunction = std::function<VariantType(nlohmann::json&)>;
    
    std::map<std::string, FactoryFunction> factories_;
    std::map<std::string, VariantType> stored_objects_;

public:
    template <typename T>
    void registerType(const std::string& typeName) {
        static_assert(std::is_constructible_v<VariantType, std::shared_ptr<T>>,
                      "The provided type T is not a valid alternative in the VariantType.");

        factories_[typeName] = [](nlohmann::json& config) -> VariantType {
            return std::make_shared<T>(config);
        };
    }

    VariantType& createAndStore(const std::string& name, nlohmann::json& config) {
        if (stored_objects_.count(name)) {
            throw std::runtime_error("An instance for type '" + name + "' already exists.");
        }

        auto factory_it = factories_.find(name);
        if (factory_it == factories_.end()) {
            throw std::runtime_error("Unknown type: " + name);
        }

        VariantType obj = factory_it->second(config);

        auto [it, inserted] = stored_objects_.emplace(name, std::move(obj));
        if (!inserted) {
            throw std::runtime_error("Failed to insert object for type '" + name + "'.");
        }
        return it->second;
    }

    template <typename T>
    std::shared_ptr<T> get(const std::string& name) {
        auto it = stored_objects_.find(name);
        if (it == stored_objects_.end()) {
            return nullptr;
        }
        if (auto* ptr_to_shared_ptr = std::get_if<std::shared_ptr<T>>(&it->second)) {
            return *ptr_to_shared_ptr;
        }
        return nullptr;
    }
};

// ---- 使用示例 ----
int main() {
    using MyVariant = std::variant<std::shared_ptr<Dog>, std::shared_ptr<Cat>>;
    GenericFactory<MyVariant> factory;

    factory.registerType<Dog>("Dog");
    factory.registerType<Cat>("Cat");

    nlohmann::json configDog = { {"name", "Buddy"} };
    nlohmann::json configCat = { {"name", "Kitty"} };

    factory.createAndStore("Dog", configDog);
    factory.createAndStore("Cat", configCat);

    if (auto d = factory.get<Dog>("Dog")) {
        d->bark();
    }
    if (auto c = factory.get<Cat>("Cat")) {
        c->meow();
    }
}
