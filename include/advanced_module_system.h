/********************************************************************
MIT License

Copyright (c) 2025 loong22

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*********************************************************************/

#pragma once
#include <typeindex>
#include <typeinfo>
#include <type_traits>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace ModuleSystem {

// 前向声明所有需要的类
class AdvancedRegistry;
class engineContext;
class Nestedengine;

template <typename T>
struct ModuleParamTraits;

// 生命周期阶段枚举
enum class LifecycleStage {
    CONSTRUCTED,
    INITIALIZED,
    EXECUTED,
    RELEASED
};

// 模块特征模板增强
template <typename T>
struct ModuleTraits {
    static T* Create(const nlohmann::json& params) {
        return new T(params);
    }
  
    static void Initialize(T* module) {
        if constexpr (requires { module->initialize(); }) {
            module->initialize();
        }
    }
  
    static void Execute(T* module) {
        module->execute();
    }
  
    static void Release(T* module) {
        if constexpr (requires { module->release(); }) {
            module->release();
        }
    }
};

// 模块工厂类 - 负责模块的创建和注册
class ModuleFactory {
private:
    // 存储模块类型和它们的创建函数
    std::unordered_map<std::string, std::function<bool(AdvancedRegistry*, const std::string&)>> moduleCreators_;

    ModuleFactory() = default;
    ~ModuleFactory() = default;

    // 允许模块工厂集合和单例访问私有成员
    friend class ModuleFactoryCollection;
    friend class std::shared_ptr<ModuleFactory>;

public:
    static ModuleFactory& instance() {
        static ModuleFactory instance;
        return instance;
    }

    void registerModuleType(const std::string& moduleName, 
                           std::function<bool(AdvancedRegistry*, const std::string&)> creator) {
        moduleCreators_[moduleName] = creator;
    }

    bool registerModule(AdvancedRegistry* reg, const std::string& moduleName) const {
        auto it = moduleCreators_.find(moduleName);
        if (it != moduleCreators_.end()) {
            return it->second(reg, moduleName);
        }
        return false;
    }
    
    // 获取单个模块创建函数
    std::function<bool(AdvancedRegistry*, const std::string&)> getModuleCreator(const std::string& moduleName) const {
        auto it = moduleCreators_.find(moduleName);
        if (it != moduleCreators_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    // 获取所有模块创建函数
    const std::unordered_map<std::string, std::function<bool(AdvancedRegistry*, const std::string&)>>& 
    getAllModuleCreators() const {
        return moduleCreators_;
    }
};

// 模块工厂集合类
class ModuleFactoryCollection {
private:
    // 存储多个命名的模块工厂
    std::unordered_map<std::string, std::shared_ptr<ModuleFactory>> factories_;
    // 默认工厂名称
    std::string defaultFactoryName_ = "default";
    
    // 使用自定义创建共享指针的方式
    std::shared_ptr<ModuleFactory> createModuleFactoryPtr() {
        ModuleFactory* factory = new ModuleFactory();
        return std::shared_ptr<ModuleFactory>(factory, [](ModuleFactory* ptr) { 
            delete ptr; 
        });
    }
    
    ModuleFactoryCollection() {
        // 创建默认工厂
        factories_["default"] = createModuleFactoryPtr();
    }
    
public:
    static ModuleFactoryCollection& instance() {
        static ModuleFactoryCollection instance;
        return instance;
    }
    
    // 根据名称获取模块工厂，如不存在则创建
    std::shared_ptr<ModuleFactory> getFactory(const std::string& factoryName = "") {
        std::string name = factoryName.empty() ? defaultFactoryName_ : factoryName;
        
        if (factories_.find(name) == factories_.end()) {
            factories_[name] = createModuleFactoryPtr();
        }
        
        return factories_[name];
    }
    
    // 设置默认工厂名称
    void setDefaultFactoryName(const std::string& name) {
        defaultFactoryName_ = name;
        if (factories_.find(name) == factories_.end()) {
            factories_[name] = createModuleFactoryPtr();
        }
    }
    
    // 检查工厂是否存在
    bool hasFactory(const std::string& name) const {
        return factories_.find(name) != factories_.end();
    }
    
    // 根据配置添加模块到指定工厂
    bool addModuleToFactory(const std::string& factoryName, const std::string& moduleName) {
        auto factory = getFactory(factoryName);
        // 从全局模块类型注册中获取模块创建函数
        auto moduleCreator = ModuleFactory::instance().getModuleCreator(moduleName);
        if (moduleCreator) {
            factory->registerModuleType(moduleName, moduleCreator);
            return true;
        }
        return false;
    }
    
    // 根据配置文件初始化工厂
    void initializeFactoriesFromConfig(const nlohmann::json& config);
    
    // 获取所有工厂名称
    std::vector<std::string> getAllFactoryNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : factories_) {
            names.push_back(name);
        }
        return names;
    }
};

// 模块工厂初始化类
class ModuleFactoryInitializer {
public:
    static void init();
};

struct ModuleMeta {
    std::function<void*(const nlohmann::json&)> construct;
    std::function<void(void*)> initialize;
    std::function<void(void*)> execute;
    std::function<void(void*)> release;
    std::type_index type;
    nlohmann::json param_schema;

    ModuleMeta() : type(typeid(void)) {}

    template<typename T>
    static T* GetTyped(void* mod) {
        return static_cast<T*>(mod);
    }

    template<typename T>
    static ModuleMeta Create() {
        ModuleMeta meta;
        meta.construct = [](const nlohmann::json& params) -> void* { 
            return ModuleTraits<T>::Create(params); 
        };
        meta.initialize = [](void* mod) { 
            ModuleTraits<T>::Initialize(static_cast<T*>(mod)); 
        };
        meta.execute = [](void* mod) { 
            ModuleTraits<T>::Execute(static_cast<T*>(mod)); 
        };
        meta.release = [](void* mod) { 
            ModuleTraits<T>::Release(static_cast<T*>(mod));
            delete static_cast<T*>(mod);
        };
        meta.type = typeid(T);
        meta.param_schema = ModuleParamTraits<T>::GetParamSchema();
        return meta;
    }
};

class AdvancedRegistry {
public:
    template <typename T>
    void Register(const std::string& name) {
        modules_[name] = ModuleMeta::Create<T>();
    }

    void* Create(const std::string& name, const nlohmann::json& params);
    void Initialize(void* mod);
    void Execute(void* mod);
    void Release(void* mod);

    // 检查未释放的模块
    std::vector<std::string> checkLeakedModules() const;

private:
    std::unordered_map<std::string, ModuleMeta> modules_;
    using LifecycleData = std::tuple<std::string, LifecycleStage, std::type_index>;
    std::unordered_map<void*, LifecycleData> lifecycle_;
};

class engineContext : public std::enable_shared_from_this<engineContext> {
public:
    engineContext(AdvancedRegistry& reg, Nestedengine* engine = nullptr);
    
    // 添加引擎名称和模块工厂权限检查
    void setEngineName(const std::string& name) { engineName_ = name; }
    const std::string& getEngineName() const { return engineName_; }
    
    // 设置当前引擎可以访问的模块列表
    void setAllowedModules(const std::unordered_set<std::string>& modules) {
        allowedModules_ = modules;
    }
    
    // 检查模块权限
    bool canAccessModule(const std::string& moduleName) const {
        // 如果没有设置允许的模块列表，或者模块在允许列表中，则允许访问
        return allowedModules_.empty() || allowedModules_.count(moduleName) > 0;
    }
    
    template<typename T>
    T getParameter(const std::string& name) const {
        auto it = parameters_.find(name);
        if (it == parameters_.end()) {
            throw std::runtime_error("Parameter not found: " + name);
        }
        return it->get<T>();
    }
    
    void setParameter(const std::string& name, const nlohmann::json& value);
    void executeSubProcess(const std::string& name);

    void* createModule(const std::string& name, const nlohmann::json& params);
    void initializeModule(const std::string& name);
    void executeModule(const std::string& name);
    void releaseModule(const std::string& name);

    friend class Nestedengine;

private:
    AdvancedRegistry& registry_;
    nlohmann::json parameters_;
    Nestedengine* engine_;
    std::unordered_map<std::string, void*> modules_;
    std::string engineName_; // 存储引擎名称
    std::unordered_set<std::string> allowedModules_; // 存储当前引擎允许访问的模块列表
};

class Nestedengine {
private:
    AdvancedRegistry& registry_;
    std::unordered_map<std::string, std::function<void(engineContext&)>> enginePool_;
    nlohmann::json parameters_;
    std::unordered_map<std::string, std::string> engineFactoryBindings_; // 存储引擎和工厂的绑定关系
    
public:
    explicit Nestedengine(AdvancedRegistry& reg);
    void Build(const nlohmann::json& config);
    void defineengine(const std::string& name, 
                     std::function<void(engineContext&)> Engine);
    const auto& getengines() const;
    nlohmann::json getDefaultConfig() const;
    void executeengine(const std::string& name, std::shared_ptr<engineContext> parentContext = nullptr);
    
    // 绑定引擎到指定的模块工厂
    void bindEngineToFactory(const std::string& engineName, const std::string& factoryName);
    
    // 获取引擎绑定的工厂名称，若未绑定则返回默认工厂
    std::string getEngineFactoryBinding(const std::string& engineName) const;
    
    // 从配置初始化引擎和工厂的绑定
    void initializeEngineFactoryBindings(const nlohmann::json& config);
};

struct ModuleTypeInfo {
    std::string name;
    std::function<nlohmann::json()> getParamSchemaFunc;
    
    ModuleTypeInfo(const std::string& n, std::function<nlohmann::json()> f);
};

class ModuleTypeRegistry {
public:
    static ModuleTypeRegistry& instance() {
        static ModuleTypeRegistry registry_instance;
        return registry_instance;
    }
    
    void registerType(const std::string& name, std::function<nlohmann::json()> schemaFunc);
    const std::vector<ModuleTypeInfo>& getModuleTypes() const;
    
private:
    ModuleTypeRegistry() = default;
    std::vector<ModuleTypeInfo> moduleTypes_;
};

template <typename T>
struct ModuleParamTraits {
    static nlohmann::json GetParamSchema() {
        return nlohmann::json::object();
    }
};

class ModuleRegistryInitializer {
public:
    ModuleRegistryInitializer();
    static ModuleRegistryInitializer& init();
private:
    ModuleRegistryInitializer(const ModuleRegistryInitializer&) = delete;
    ModuleRegistryInitializer& operator=(const ModuleRegistryInitializer&) = delete;
};

// 导出的函数声明
nlohmann::json createRegistryInfo();
nlohmann::json createengineInfo();
nlohmann::json createDefaultConfig();

void getConfigInfo(std::shared_ptr<ModuleSystem::AdvancedRegistry> registry, 
                   std::unique_ptr<ModuleSystem::Nestedengine>& engine, 
                   const std::string& outputFile);

void saveUsedConfig(const nlohmann::json& moduleConfig, 
                    const std::unordered_set<std::string>& enabledModules, 
                    const nlohmann::json& engine,
                    const nlohmann::json& globalParams,
                    const std::string& outputFile);

std::string validateParam(const nlohmann::json& paramSchema, const nlohmann::json& value);

void validateModuleParams(const nlohmann::json& moduleParams, 
                          const std::string& moduleName, 
                          int rank = 0);

class engineExecutionengine {
public:
    engineExecutionengine(const nlohmann::json& engines, 
                        ModuleSystem::engineContext& context);
    bool executeengine(const std::string& engineName);

private:
    const nlohmann::json& engines_;
    std::unordered_set<std::string> visitedengines_;
    ModuleSystem::engineContext& context_;
};

nlohmann::json getEffectiveModuleParams(
    const nlohmann::json& moduleConfig, 
    const std::string& moduleName,
    const nlohmann::json& engineSpecificParams);

void runWithConfig(const nlohmann::json& config, const std::string& outputFile = "");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 模块类声明
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// LaminarSolverEuler 模块
class LaminarSolverEuler {
public:
    explicit LaminarSolverEuler(const nlohmann::json& params);
    void initialize() {} // 空实现
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    std::string euler_type_;
    double euler_value_;
};

// TurbulenceSolverSA 模块
class TurbulenceSolverSA {
public:
    explicit TurbulenceSolverSA(const nlohmann::json& params);
    void initialize() {} // 空实现
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    std::string sa_type_;
    double value_;
};

// ThermalSolver 模块
class ThermalSolver {
public:
    explicit ThermalSolver(const nlohmann::json& params);
    void initialize() {} // 空实现
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    double delta_t_;
};

// FluidSolver 模块
class FluidSolver {
public:
    explicit FluidSolver(const nlohmann::json& params);
    void initialize() {} // 空实现
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    std::string solver_type_;
    double convergence_criteria_;
    int max_iterations_;
};

// 模块参数特化
template <> struct ModuleParamTraits<LaminarSolverEuler> {
    static nlohmann::json GetParamSchema();
};

template <> struct ModuleParamTraits<TurbulenceSolverSA> {
    static nlohmann::json GetParamSchema();
};

template <> struct ModuleParamTraits<ThermalSolver> {
    static nlohmann::json GetParamSchema();
};

template <> struct ModuleParamTraits<FluidSolver> {
    static nlohmann::json GetParamSchema();
};

} // namespace ModuleSystem