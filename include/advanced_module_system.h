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
// 删除 MPI 头文件
// #include <mpi.h>

namespace ModuleSystem {

// 前向声明所有需要的类
class AdvancedRegistry;  // 添加这行，确保在 ModuleFactory 前声明
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
    static T Create(const nlohmann::json& params) {
        return T(params);
    }
  
    static void Initialize(T& module) {
        if constexpr (requires { module.initialize(); }) {
            module.initialize();
        }
    }
  
    static void Execute(T& module) {
        module.execute();
    }
  
    static void Release(T& module) {
        if constexpr (requires { module.release(); }) {
            module.release();
        }
    }
};

// 模块工厂类 - 负责模块的创建和注册
class ModuleFactory {
    public:
        // 单例模式
        static ModuleFactory& instance() {
            static ModuleFactory factory;
            return factory;
        }
    
        // 注册模块创建函数
        using RegisterFunc = std::function<void(AdvancedRegistry*, const std::string&)>;
        
        // 注册一个模块类型
        void registerModuleType(const std::string& name, RegisterFunc func) {
            registry_[name] = func;
        }
        
        // 通过名称创建模块
        bool registerModule(AdvancedRegistry* registry, const std::string& name) {
            auto it = registry_.find(name);
            if (it != registry_.end()) {
                it->second(registry, name);
                return true;
            }
            return false;
        }
    
    private:
        ModuleFactory() = default;
        ~ModuleFactory() = default;
        
        // 禁止拷贝和移动
        ModuleFactory(const ModuleFactory&) = delete;
        ModuleFactory& operator=(const ModuleFactory&) = delete;
        ModuleFactory(ModuleFactory&&) = delete;
        ModuleFactory& operator=(ModuleFactory&&) = delete;
        
        std::unordered_map<std::string, RegisterFunc> registry_;
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
    static T& GetTyped(void* mod) {
        return *static_cast<T*>(mod);
    }

    template<typename T>
    static ModuleMeta Create() {
        ModuleMeta meta;
        meta.construct = [](const nlohmann::json& params) -> void* { 
            return new T(ModuleTraits<T>::Create(params)); 
        };
        meta.initialize = [](void* mod) { 
            ModuleTraits<T>::Initialize(*static_cast<T*>(mod)); 
        };
        meta.execute = [](void* mod) { 
            ModuleTraits<T>::Execute(*static_cast<T*>(mod)); 
        };
        meta.release = [](void* mod) { 
            ModuleTraits<T>::Release(*static_cast<T*>(mod));
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

    // 新增方法，检查未释放的模块
    std::vector<std::string> checkLeakedModules() const;

private:
    std::unordered_map<std::string, ModuleMeta> modules_;
    using LifecycleData = std::tuple<std::string, LifecycleStage, std::type_index>;
    std::unordered_map<void*, LifecycleData> lifecycle_;
};

class Nestedengine; // Already forward declared, ensure it's before engineContext usage

class engineContext : public std::enable_shared_from_this<engineContext> {
public:
    // Modified constructor to accept rank
    engineContext(AdvancedRegistry& reg, Nestedengine* engine = nullptr);
    
    template<typename T>
    T getParameter(const std::string& name) const {
        auto it = parameters_.find(name);
        if (it == parameters_.end()) {
            throw std::runtime_error("Parameter not found: " + name);
        }
        return it.value().get<T>();
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
    int rank_; // Added rank for conditional logging
};

class Nestedengine {
public:
    explicit Nestedengine(AdvancedRegistry& reg);
    void Build(const nlohmann::json& config);
    void defineengine(const std::string& name, 
                       std::function<void(engineContext&)> Engine);
    const auto& getengines() const;
    nlohmann::json getDefaultConfig() const;
    void executeengine(const std::string& name, std::shared_ptr<engineContext> parentContext = nullptr);

private:
    AdvancedRegistry& registry_;
    std::unordered_map<std::string, std::function<void(engineContext&)>> enginePool_;
    nlohmann::json parameters_;
};

struct ModuleTypeInfo {
    std::string name;
    std::function<nlohmann::json()> getParamSchemaFunc;
    
    ModuleTypeInfo(const std::string& n, std::function<nlohmann::json()> f);
};

class ModuleTypeRegistry {
public:
    static ModuleTypeRegistry& instance() {
        static ModuleTypeRegistry registry_instance; // Renamed to avoid conflict
        return registry_instance;
    }
    
    void registerType(const std::string& name, std::function<nlohmann::json()> schemaFunc);
    const std::vector<ModuleTypeInfo>& getModuleTypes() const;
    
private:
    ModuleTypeRegistry() = default; // Private constructor for singleton
    std::vector<ModuleTypeInfo> moduleTypes_;
};

class ModuleRegistryInitializer; // Forward declaration

template <typename T>
struct ModuleParamTraits {
    static nlohmann::json GetParamSchema() { // Default implementation
        return nlohmann::json::array();
    }
};

class ModuleRegistryInitializer {
public:
    ModuleRegistryInitializer();
    static ModuleRegistryInitializer& init();
private:
    // Ensure it's not copyable/movable if it manages global state implicitly
    ModuleRegistryInitializer(const ModuleRegistryInitializer&) = delete;
    ModuleRegistryInitializer& operator=(const ModuleRegistryInitializer&) = delete;
};

// Declarations from main.h, now part of ModuleSystem
nlohmann::json createRegistryInfo();
nlohmann::json createengineInfo();
// 删除 broadcastJson 函数声明，不再使用 MPI_Comm
// void broadcastJson(nlohmann::json& config, MPI_Comm comm);

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
                          const std::string& moduleName);

class engineExecutionengine {
public:
    // 修改构造函数，移除 rank 参数
    engineExecutionengine(const nlohmann::json& engines, 
                            ModuleSystem::engineContext& context);
    bool executeengine(const std::string& engineName);

private:
    const nlohmann::json& engines_;
    std::unordered_set<std::string> visitedengines_;
    ModuleSystem::engineContext& context_;
    // int rank_; // 删除 rank 成员变量
};

nlohmann::json getEffectiveModuleParams(
    const nlohmann::json& moduleConfig, 
    const std::string& moduleName,
    const nlohmann::json& engineSpecificParams);

void runWithConfig(const nlohmann::json& config, const std::string& outputFile = "");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 模块类声明
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LaminarSolverEuler {
public:
    explicit LaminarSolverEuler(const nlohmann::json& params);
    void initialize();
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    std::string euler_type_;
    double euler_value_;
};

template <>
struct ModuleParamTraits<LaminarSolverEuler> {
    static nlohmann::json GetParamSchema();
};

class TurbulenceSolverSA {
public:
    explicit TurbulenceSolverSA(const nlohmann::json& params);
    void initialize();
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    std::string sa_type_;
    double value_;
};

template <>
struct ModuleParamTraits<TurbulenceSolverSA> {
    static nlohmann::json GetParamSchema();
};

class ThermalSolver {
public:
    explicit ThermalSolver(const nlohmann::json& params);
    void initialize();
    void execute();
    void release();
    static nlohmann::json GetParamSchema();
private:
    double delta_t_;
};

template <>
struct ModuleParamTraits<ThermalSolver> {
    static nlohmann::json GetParamSchema();
};

// 示例：添加一个新的流体求解器模块
class FluidSolver {
    public:
        explicit FluidSolver(const nlohmann::json& params);
        void initialize();
        void execute();
        void release();
        static nlohmann::json GetParamSchema();
    private:
        std::string solver_type_;
        double convergence_criteria_;
        int max_iterations_;
};

// 为新模块特化 ModuleParamTraits 模板
template <>
struct ModuleParamTraits<FluidSolver> {
    static nlohmann::json GetParamSchema();
};

} // namespace ModuleSystem
