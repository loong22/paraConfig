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

/**
 * @file advanced_module_system.h
 * @brief 高级模块系统的头文件
 * @author loong22
 * @date 2025
 */

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

/**
 * @namespace ModuleSystem
 * @brief 包含模块系统的所有组件
 */
namespace ModuleSystem {

// 前向声明所有需要的类
class AdvancedRegistry;
class engineContext;
class Nestedengine;


/**
 * @brief 模块参数特性模板（前向声明）
 * @tparam T 模块类型
 */
template <typename T>
struct ModuleParamTraits;

/**
 * @brief 模块生命周期阶段枚举
 */
enum class LifecycleStage {
    CONSTRUCTED, /**< 模块已构造 */
    INITIALIZED, /**< 模块已初始化 */
    EXECUTED,    /**< 模块已执行 */
    RELEASED     /**< 模块已释放 */
};

/**
 * @brief 用于记录所有引擎中启用的模块信息
 */
struct ModuleExecInfo {
    std::string engineName;    /**< 引擎名称 */
    std::string moduleName;    /**< 模块名称 */
    nlohmann::json moduleParams; /**< 模块参数 */
};

/**
 * @brief 辅助函数：收集引擎中所有模块信息
 * @param config 配置文件JSON对象
 * @param engineName 引擎名称
 * @param visitedEngines 已访问的引擎集合（用于检测循环依赖）
 * @return 收集是否成功
 */
bool collectModulesFromConfig(
    const nlohmann::json& config, 
    const std::string& engineName, 
    std::unordered_set<std::string>& visitedEngines
);

/**
 * @brief 模块特征模板增强
 * @tparam T 模块类型
 */
template <typename T>
struct ModuleTraits {
    /**
     * @brief 创建模块实例
     * @param params 模块参数
     * @return 模块实例指针
     */
    static T* Create(const nlohmann::json& params) {
        return new T(params);
    }
  
    /**
     * @brief 初始化模块
     * @param module 模块实例指针
     */
    static void Initialize(T* module) {
        if constexpr (requires { module->initialize(); }) {
            module->initialize();
        }
    }
  
    /**
     * @brief 执行模块
     * @param module 模块实例指针
     */
    static void Execute(T* module) {
        module->execute();
    }
  
    /**
     * @brief 释放模块
     * @param module 模块实例指针
     */
    static void Release(T* module) {
        if constexpr (requires { module->release(); }) {
            module->release();
        }
    }
};

/**
 * @brief 模块工厂类 - 负责模块的创建和注册
 */
class ModuleFactory {
public:
    /**
     * @brief 获取模块工厂单例实例
     * @return 模块工厂实例的引用
     */
    static ModuleFactory& instance() {
        static ModuleFactory instance;
        return instance;
    }

    /**
     * @brief 注册一个模块类型
     * @param moduleName 模块名称
     * @param creator 模块创建函数
     */
    void registerModuleType(
        const std::string& moduleName, 
        std::function<bool(AdvancedRegistry*, const std::string&)> creator
    ) {
        moduleCreators_[moduleName] = creator;
    }

    /**
     * @brief 向注册表注册一个模块
     * @param reg 高级注册表指针
     * @param moduleName 模块名称
     * @return 注册是否成功
     */
    bool registerModule(AdvancedRegistry* reg, const std::string& moduleName) const {
        auto it = moduleCreators_.find(moduleName);
        if (it != moduleCreators_.end()) {
            return it->second(reg, moduleName);
        }
        return false;
    }
    
    /**
     * @brief 获取单个模块创建函数
     * @param moduleName 模块名称
     * @return 模块创建函数，如果未找到则返回nullptr
     */
    std::function<bool(AdvancedRegistry*, const std::string&)> getModuleCreator(
        const std::string& moduleName
    ) const {
        auto it = moduleCreators_.find(moduleName);
        if (it != moduleCreators_.end()) {
            return it->second;
        }
        return nullptr;
    }
    
    /**
     * @brief 获取所有模块创建函数
     * @return 模块名称到创建函数的映射
     */
    const std::unordered_map<
        std::string, 
        std::function<bool(AdvancedRegistry*, const std::string&)>
    >& getAllModuleCreators() const {
        return moduleCreators_;
    }

private:
    /// 存储模块类型和它们的创建函数
    std::unordered_map<
        std::string, 
        std::function<bool(AdvancedRegistry*, const std::string&)>
    > moduleCreators_;

    /// 默认构造函数
    ModuleFactory() = default;
    
    /// 默认析构函数
    ~ModuleFactory() = default;

    // 允许模块工厂集合和单例访问私有成员
    friend class ModuleFactoryCollection;
    friend class std::shared_ptr<ModuleFactory>;
};

/**
 * @brief 模块工厂集合类
 */
class ModuleFactoryCollection {
public:
    /**
     * @brief 获取模块工厂集合单例实例
     * @return 模块工厂集合实例的引用
     */
    static ModuleFactoryCollection& instance() {
        static ModuleFactoryCollection instance;
        return instance;
    }
    
    /**
     * @brief 根据名称获取模块工厂，如不存在则创建
     * @param factoryName 工厂名称，默认为空字符串
     * @return 模块工厂的共享指针
     */
    std::shared_ptr<ModuleFactory> getFactory(const std::string& factoryName = "") {
        std::string name = factoryName.empty() ? defaultFactoryName_ : factoryName;
        
        if (factories_.find(name) == factories_.end()) {
            factories_[name] = createModuleFactoryPtr();
        }
        
        return factories_[name];
    }
    
    /**
     * @brief 设置默认工厂名称
     * @param name 工厂名称
     */
    void setDefaultFactoryName(const std::string& name) {
        defaultFactoryName_ = name;
        if (factories_.find(name) == factories_.end()) {
            factories_[name] = createModuleFactoryPtr();
        }
    }
    
    /**
     * @brief 检查工厂是否存在
     * @param name 工厂名称
     * @return 工厂是否存在
     */
    bool hasFactory(const std::string& name) const {
        return factories_.find(name) != factories_.end();
    }
    
    /**
     * @brief 根据配置添加模块到指定工厂
     * @param factoryName 工厂名称
     * @param moduleName 模块名称
     * @return 添加是否成功
     */
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
    
    /**
     * @brief 获取所有工厂名称
     * @return 工厂名称列表
     */
    std::vector<std::string> getAllFactoryNames() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : factories_) {
            names.push_back(name);
        }
        return names;
    }
    
private:
    /// 存储多个命名的模块工厂
    std::unordered_map<std::string, std::shared_ptr<ModuleFactory>> factories_;
    
    /// 默认工厂名称
    std::string defaultFactoryName_ = "default";
    
    /**
     * @brief 使用自定义创建共享指针的方式
     * @return 模块工厂的共享指针
     */
    std::shared_ptr<ModuleFactory> createModuleFactoryPtr() {
        ModuleFactory* factory = new ModuleFactory();
        return std::shared_ptr<ModuleFactory>(factory, [](ModuleFactory* ptr) { 
            delete ptr; 
        });
    }
    
    /**
     * @brief 构造函数，创建默认工厂
     */
    ModuleFactoryCollection() {
        // 创建默认工厂
        factories_["default"] = createModuleFactoryPtr();
    }
};

/**
 * @brief 模块工厂初始化类
 */
class ModuleFactoryInitializer {
public:
    /**
     * @brief 初始化模块工厂
     */
    static void init();
};

/**
 * @brief 模块元数据结构
 */
struct ModuleMeta {
    std::function<void*(const nlohmann::json&)> construct; /**< 模块构造函数 */
    std::function<void(void*)> initialize;                /**< 模块初始化函数 */
    std::function<void(void*)> execute;                   /**< 模块执行函数 */
    std::function<void(void*)> release;                   /**< 模块释放函数 */
    std::type_index type;                                /**< 模块类型索引 */
    nlohmann::json param_schema;                         /**< 模块参数架构 */

    /**
     * @brief 默认构造函数
     */
    ModuleMeta() : type(typeid(void)) {}

    /**
     * @brief 获取指定类型的模块指针
     * @tparam T 模块类型
     * @param mod 模块指针
     * @return 指定类型的模块指针
     */
    template<typename T>
    static T* GetTyped(void* mod) {
        return static_cast<T*>(mod);
    }

    /**
     * @brief 创建指定类型的模块元数据
     * @tparam T 模块类型
     * @return 模块元数据
     */
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

/**
 * @brief 模块引擎映射类
 */
class EngineModuleMapping {
public:
    /**
     * @brief 获取单例实例
     * @return 引用到单例实例
     */
    static EngineModuleMapping& instance() {
        static EngineModuleMapping instance;
        return instance;
    }
    
    /**
     * @brief 将模块关联到引擎
     * @param moduleName 模块名称
     * @param engineName 引擎名称
     */
    void assignModuleToEngine(const std::string& moduleName, const std::string& engineName) {
        moduleToEngine_[moduleName] = engineName;
    }
    
    /**
     * @brief 获取模块所属的引擎
     * @param moduleName 模块名称
     * @return 引擎名称，如果模块未绑定则返回空字符串
     */
    std::string getModuleEngine(const std::string& moduleName) const {
        auto it = moduleToEngine_.find(moduleName);
        return (it != moduleToEngine_.end()) ? it->second : "";
    }
    
    /**
     * @brief 获取引擎关联的所有模块
     * @param engineName 引擎名称
     * @return 该引擎关联的模块名称集合
     */
    std::vector<std::string> getEngineModules(const std::string& engineName) const {
        std::vector<std::string> modules;
        for (const auto& [modName, engName] : moduleToEngine_) {
            if (engName == engineName) {
                modules.push_back(modName);
            }
        }
        return modules;
    }
    
    /**
     * @brief 检查模块是否绑定到任何引擎
     * @param moduleName 模块名称
     * @return 如果模块已绑定到任何引擎则返回true，否则返回false
     */
    bool isModuleBoundToEngine(const std::string& moduleName) const {
        return moduleToEngine_.find(moduleName) != moduleToEngine_.end();
    }
    
private:
    std::unordered_map<std::string, std::string> moduleToEngine_;
};

/**
 * @brief 辅助函数：将模块关联到引擎
 * @param moduleName 模块名称
 * @param engineName 引擎名称
 */
inline void assignModuleToEngine(const std::string& moduleName, const std::string& engineName) {
    EngineModuleMapping::instance().assignModuleToEngine(moduleName, engineName);
}

/**
 * @brief 模块接口检查器
 */
template <typename T>
class ModuleInterfaceChecker {
private:
    // 用SFINAE技术检查类是否有特定方法
    
    // 检查构造函数是否接受json参数
    template <typename C>
    static auto hasJsonConstructor(int) 
        -> decltype(C(std::declval<const nlohmann::json&>()), std::true_type());
    
    template <typename C>
    static std::false_type hasJsonConstructor(...);
    
    // 检查initialize方法
    template <typename C>
    static auto hasInitialize(int) 
        -> decltype(std::declval<C>().initialize(), std::true_type());
    
    template <typename C>
    static std::false_type hasInitialize(...);
    
    // 检查execute方法
    template <typename C>
    static auto hasExecute(int) 
        -> decltype(std::declval<C>().execute(), std::true_type());
    
    template <typename C>
    static std::false_type hasExecute(...);
    
    // 检查release方法
    template <typename C>
    static auto hasRelease(int) 
        -> decltype(std::declval<C>().release(), std::true_type());
    
    template <typename C>
    static std::false_type hasRelease(...);
    
    // 检查静态GetParamSchema方法
    template <typename C>
    static auto hasGetParamSchema(int) 
        -> decltype(C::GetParamSchema(), std::true_type());
    
    template <typename C>
    static std::false_type hasGetParamSchema(...);

public:
    // 编译时检查接口
    static constexpr bool validateInterface() {
        static_assert(decltype(hasJsonConstructor<T>(0))::value, 
                      "Module must implement constructor accepting nlohmann::json parameter");
        static_assert(decltype(hasInitialize<T>(0))::value, 
                      "Module must implement initialize() method");
        static_assert(decltype(hasExecute<T>(0))::value, 
                      "Module must implement execute() method");
        static_assert(decltype(hasRelease<T>(0))::value, 
                      "Module must implement release() method");
        static_assert(decltype(hasGetParamSchema<T>(0))::value,
                      "Module must implement static GetParamSchema() method");
        return true;
    }
};

/**
 * @brief 高级注册表类
 */
class AdvancedRegistry {
public:
    /**
     * @brief 注册一个模块类型
     * @tparam T 模块类型
     * @param name 模块名称
     */
    template <typename T>
    void Register(const std::string& name) {
        // 编译时检查模块接口
        constexpr bool valid = ModuleInterfaceChecker<T>::validateInterface();
        static_assert(valid, "Module interface validation failed");
    
        modules_[name] = ModuleMeta::Create<T>();
    }

    /**
     * @brief 创建模块实例
     * @param name 模块名称
     * @param params 模块参数
     * @return 模块实例指针
     */
    void* Create(const std::string& name, const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     * @param mod 模块实例指针
     */
    void Initialize(void* mod);
    
    /**
     * @brief 执行模块
     * @param mod 模块实例指针
     */
    void Execute(void* mod);
    
    /**
     * @brief 释放模块
     * @param mod 模块实例指针
     */
    void Release(void* mod);

    /**
     * @brief 检查未释放的模块
     * @return 未释放的模块列表
     */
    std::vector<std::string> checkLeakedModules() const;

private:
    /// 模块名称到模块元数据的映射
    std::unordered_map<std::string, ModuleMeta> modules_;
    
    /// 模块生命周期数据类型定义
    using LifecycleData = std::tuple<std::string, LifecycleStage, std::type_index>;
    
    /// 模块实例到生命周期数据的映射
    std::unordered_map<void*, LifecycleData> lifecycle_;
};

/**
 * @brief 引擎上下文类
 */
class engineContext : public std::enable_shared_from_this<engineContext> {
public:
    /**
     * @brief 构造函数
     * @param reg 高级注册表引用
     * @param engine 嵌套引擎指针，默认为nullptr
     */
    engineContext(AdvancedRegistry& reg, Nestedengine* engine = nullptr);
    
    /**
     * @brief Gets the parameters stored in the context.
     * @return A const reference to the parameters JSON object.
     */
    const nlohmann::json& getParameters() const {
        return parameters_;
    }

    /**
     * @brief 设置引擎名称
     * @param name 引擎名称
     */
    void setEngineName(const std::string& name) { 
        engineName_ = name; 
    }
    
    /**
     * @brief 获取引擎名称
     * @return 引擎名称
     */
    const std::string& getEngineName() const { 
        return engineName_; 
    }
    
    /**
     * @brief 设置当前引擎可以访问的模块列表
     * @param modules 模块名称集合
     */
    void setAllowedModules(const std::unordered_set<std::string>& modules) {
        allowedModules_ = modules;
    }
    
    /**
     * @brief 检查模块权限
     * @param moduleName 模块名称
     * @return 是否可以访问该模块
     */
    bool canAccessModule(const std::string& moduleName) const;
    
    /**
     * @brief 获取参数
     * @tparam T 参数类型
     * @param name 参数名称
     * @return 参数值
     * @throws std::runtime_error 如果参数不存在
     */
    template<typename T>
    T getParameter(const std::string& name) const {
        auto it = parameters_.find(name);
        if (it == parameters_.end()) {
            throw std::runtime_error("Parameter not found: " + name);
        }
        return it.value().get<T>();
    }
    
    /**
     * @brief 设置参数
     * @param name 参数名称
     * @param value 参数值
     */
    void setParameter(const std::string& name, const nlohmann::json& value);

    /**
     * @brief 创建模块实例
     * @param name 模块名称
     * @param params 模块参数
     * @return 模块实例指针
     */
    void* createModule(const std::string& name, const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     * @param name 模块名称
     */
    void initializeModule(const std::string& name);
    
    /**
     * @brief 执行模块
     * @param name 模块名称
     */
    void executeModule(const std::string& name);
    
    /**
     * @brief 释放模块
     * @param name 模块名称
     */
    void releaseModule(const std::string& name);
    
    friend class Nestedengine;
    friend class engineExecutionEngine;

private:
    AdvancedRegistry& registry_;  /**< 高级注册表引用 */
    nlohmann::json parameters_;   /**< 参数映射 */
    Nestedengine* engine_;        /**< 嵌套引擎指针 */
    std::unordered_map<std::string, void*> modules_; /**< 模块名称到实例的映射 */
    std::string engineName_;      /**< 存储引擎名称 */
    std::unordered_set<std::string> allowedModules_; /**< 存储当前引擎允许访问的模块列表 */
};

/**
 * @brief 嵌套引擎类
 */
class Nestedengine {
public:
    /**
     * @brief 构造函数
     * @param reg 高级注册表引用
     */
    explicit Nestedengine(AdvancedRegistry& reg);
    
    /**
     * @brief 构建配置
     * @param config 配置JSON对象
     */
    void Build(const nlohmann::json& config);
    
    /**
     * @brief 定义引擎
     * @param name 引擎名称
     * @param Engine 引擎函数
     */
    void defineengine(
        const std::string& name,
        std::function<void(engineContext&)> Engine
    );
    
    /**
     * @brief 获取引擎列表
     * @return 引擎列表
     */
    const auto& getengines() const;
    
    /**
     * @brief 获取默认配置
     * @return 默认配置JSON对象
     */
    nlohmann::json getDefaultConfig() const;
    
    /**
     * @brief 获取静态引擎池
     * @return 静态引擎池的JSON对象引用
     */
    static const nlohmann::json& getStaticEnginePool() {
        return staticEnginePool_;
    }
    
private:
    AdvancedRegistry& registry_; /**< 高级注册表引用 */
    std::unordered_map<std::string, std::function<void(engineContext&)>> enginePool_; /**< 引擎池 */
    nlohmann::json parameters_; /**< 参数 */
    
    /// 静态成员变量 - 存储引擎池
    static nlohmann::json staticEnginePool_;
};

/**
 * @brief 模块类型信息结构
 */
struct ModuleTypeInfo {
    std::string name; /**< 模块名称 */
    std::function<nlohmann::json()> getParamSchemaFunc; /**< 获取参数架构的函数 */
    
    /**
     * @brief 构造函数
     * @param n 模块名称
     * @param f 获取参数架构的函数
     */
    ModuleTypeInfo(const std::string& n, std::function<nlohmann::json()> f);
};

/**
 * @brief 模块类型注册表
 */
class ModuleTypeRegistry {
public:
    /**
     * @brief 获取模块类型注册表单例实例
     * @return 模块类型注册表实例的引用
     */
    static ModuleTypeRegistry& instance() {
        static ModuleTypeRegistry registry_instance;
        return registry_instance;
    }
    
    /**
     * @brief 注册一个模块类型
     * @param name 模块名称
     * @param schemaFunc 获取参数架构的函数
     */
    void registerType(const std::string& name, std::function<nlohmann::json()> schemaFunc);
    
    /**
     * @brief 获取所有模块类型信息
     * @return 模块类型信息列表
     */
    const std::vector<ModuleTypeInfo>& getModuleTypes() const;
    
private:
    /// 默认构造函数
    ModuleTypeRegistry() = default;
    
    /// 存储模块类型信息
    std::vector<ModuleTypeInfo> moduleTypes_;
};

/**
 * @brief 模块参数特性模板
 * @tparam T 模块类型
 */
template <typename T>
struct ModuleParamTraits {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema() {
        return nlohmann::json::object();
    }
};

/**
 * @brief 模块注册初始化类
 */
class ModuleRegistryInitializer {
public:
    ModuleRegistryInitializer();
    
    static ModuleRegistryInitializer& init();
    
    // 添加新方法来指定模块所属的引擎
    void assignModuleToEngine(const std::string& moduleName, const std::string& engineName) {
        EngineModuleMapping::instance().assignModuleToEngine(moduleName, engineName);
    }
    
private:
    ModuleRegistryInitializer(const ModuleRegistryInitializer&) = delete;
    ModuleRegistryInitializer& operator=(const ModuleRegistryInitializer&) = delete;
};

/**
 * @brief 引擎执行引擎类
 */
class engineExecutionEngine {
public:
    /**
     * @brief 构造函数
     * @param engines 引擎配置JSON对象
     * @param context 引擎上下文引用
     */
    engineExecutionEngine(
        const nlohmann::json& engines, 
        ModuleSystem::engineContext& context
    );
    
    /**
     * @brief 执行指定引擎
     * @param engineName 引擎名称
     * @return 执行是否成功
     */
    bool executeengine(const std::string& engineName,  engineContext& parentContext);

private:
    const nlohmann::json& engines_; /**< 引擎配置JSON对象 */
    std::unordered_set<std::string> visitedengines_; /**< 已访问的引擎集合 */
    ModuleSystem::engineContext& context_; /**< 引擎上下文引用 */
};

/**
 * @brief 配置存储类
 */
class ConfigurationStorage {
public:
    /**
     * @brief 获取配置存储单例实例
     * @return 配置存储实例的引用
     */
    static ConfigurationStorage& instance() {
        static ConfigurationStorage instance;
        return instance;
    }
    
    void initializeEngineContexts();

    nlohmann::json config; /**< 验证过的配置 */
    
    // 预处理的关键数据
    nlohmann::json moduleConfig; /**< 模块配置 */
    nlohmann::json globalParams; /**< 全局参数 */
    std::unordered_set<std::string> knownModules; /**< 已知模块集合 */
    std::unordered_set<std::string> enabledModules; /**< 已启用的模块集合 */
    std::unordered_set<std::string> usedEngineNames; /**< 使用的引擎名称集合 */
    
    // 核心组件
    std::shared_ptr<AdvancedRegistry> registry; /**< 高级注册表 */
    std::unique_ptr<Nestedengine> engine; /**< 嵌套引擎 */
    std::shared_ptr<engineContext> mainContext; /**< 主上下文 */
    
    // 引擎上下文集合
    std::unordered_map<std::string, std::shared_ptr<engineContext>> engineContexts;

    // 存储已收集的所有模块信息，按引擎组织
    std::unordered_map<std::string, std::vector<ModuleExecInfo>> engineModules;
    
    // 存储引擎执行顺序
    std::vector<std::string> engineExecutionOrder;
    
    bool enginesAreDefined = false; /**< 引擎定义状态 */
    
    /**
     * @brief 清除配置数据
     */
    void clear() {
        config = nlohmann::json();
        moduleConfig = nlohmann::json();
        globalParams = nlohmann::json();
        knownModules.clear();
        enabledModules.clear();
        usedEngineNames.clear();
        engineContexts.clear();
        engineModules.clear();
        engineExecutionOrder.clear();
        enginesAreDefined = false;
        // 保留 registry、engine 和 mainContext 不清除实例，只重置状态
    }
    
    /**
     * @brief 初始化注册表和引擎
     */
    void initializeRegistryAndEngine() {
        if (!registry) {
            registry = std::make_shared<AdvancedRegistry>();
        }
        
        if (!engine) {
            engine = std::make_unique<Nestedengine>(*registry);
        }
    }

private:
    /// 默认构造函数
    ConfigurationStorage() = default;
    
    /// 删除拷贝构造函数
    ConfigurationStorage(const ConfigurationStorage&) = delete;
    
    /// 删除赋值运算符
    ConfigurationStorage& operator=(const ConfigurationStorage&) = delete;
};

/**
 * @brief 创建注册表信息
 * @return 注册表信息JSON对象
 */
nlohmann::json createRegistryInfo();

/**
 * @brief 创建引擎信息
 * @return 引擎信息JSON对象
 */
nlohmann::json createengineInfo();

/**
 * @brief 获取配置
 * @param configFile 配置文件路径
 */
void getConfig(const std::string& configFile);

/**
 * @brief 获取配置
 * @param argc 参数个数
 * @param argv 参数值数组 
 */
void getConfig(int argc, char* argv[]);

/**
 * @brief 获取配置信息
 * @param registry 高级注册表共享指针
 * @param engine 嵌套引擎唯一指针
 * @param outputFile 输出文件路径
 */
void getConfigInfo(
    std::shared_ptr<ModuleSystem::AdvancedRegistry> registry, 
    std::unique_ptr<ModuleSystem::Nestedengine>& engine, 
    const std::string& outputFile
);

/**
 * @brief 保存使用的配置
 * @param moduleConfig 模块配置JSON对象
 * @param enabledModules 已启用的模块集合
 * @param engine 引擎JSON对象
 * @param globalParams 全局参数JSON对象
 * @param outputFile 输出文件路径
 */
void saveUsedConfig(
    const nlohmann::json& moduleConfig, 
    const std::unordered_set<std::string>& enabledModules, 
    const nlohmann::json& engine,
    const nlohmann::json& globalParams,
    const std::string& outputFile
);

/**
 * @brief 验证参数
 * @param paramSchema 参数架构JSON对象
 * @param value 参数值JSON对象
 * @return 错误信息，如果没有错误则为空字符串
 */
std::string validateParam(const nlohmann::json& paramSchema, const nlohmann::json& value);

/**
 * @brief 验证模块参数
 * @param moduleParams 模块参数JSON对象
 * @param moduleName 模块名称
 * @param rank 模块等级，默认为0
 */
void validateModuleParams(
    const nlohmann::json& moduleParams, 
    const std::string& moduleName, 
    int rank = 0
);

/**
 * @brief 获取有效的模块参数
 * @param moduleConfig 模块配置JSON对象
 * @param moduleName 模块名称
 * @param engineSpecificParams 引擎特定参数JSON对象
 * @return 有效的模块参数JSON对象
 */
nlohmann::json getEffectiveModuleParams(
    const nlohmann::json& moduleConfig, 
    const std::string& moduleName,
    const nlohmann::json& engineSpecificParams
);

/**
 * @brief 运行模块系统
 */
void run();

/**
 * @brief 参数验证
 * @param config 配置JSON对象
 * @return 验证是否通过
 */
bool paramValidation(const nlohmann::json& config);

/**
 * @brief 保存使用的配置
 * @param config 配置JSON对象
 * @param outputFile 输出文件路径
 */
void saveUsedConfig(const nlohmann::json& config, const std::string& outputFile);

/**
 * @brief 测试模块系统
 */
void test();

/**
 * @brief 预处理CGNS模块
 */
class PreCGNS {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit PreCGNS(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string cgns_type_; /**< CGNS类型 */
    double cgns_value_;     /**< CGNS值 */
};

/**
 * @brief PreCGNS模块的参数特性特化
 */
template <> struct ModuleParamTraits<PreCGNS> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief 预处理Plot3D模块
 */
class PrePlot3D {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit PrePlot3D(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string plot3_type_; /**< Plot3D类型 */
    double plot3d_value_;    /**< Plot3D值 */
};

/**
 * @brief PrePlot3D模块的参数特性特化
 */
template <> struct ModuleParamTraits<PrePlot3D> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief 欧拉求解器模块
 */
class EulerSolver {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit EulerSolver(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string euler_type_; /**< 欧拉类型 */
    double euler_value__;    /**< 欧拉值 */
};

/**
 * @brief EulerSolver模块的参数特性特化
 */
template <> struct ModuleParamTraits<EulerSolver> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief SA求解器模块
 */
class SASolver {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit SASolver(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string sa_type_;          /**< SA类型 */
    double convergence_criteria_;  /**< 收敛标准 */
    int max_iterations_;           /**< 最大迭代次数 */
};

/**
 * @brief SASolver模块的参数特性特化
 */
template <> struct ModuleParamTraits<SASolver> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief SST求解器模块
 */
class SSTSolver {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit SSTSolver(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string sst_type_;         /**< SST类型 */
    double convergence_criteria_;  /**< 收敛标准 */
    int max_iterations_;           /**< 最大迭代次数 */
};

/**
 * @brief SSTSolver模块的参数特性特化
 */
template <> struct ModuleParamTraits<SSTSolver> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief 后处理CGNS模块
 */
class PostCGNS {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit PostCGNS(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string cgns_type_; /**< CGNS类型 */
    double cgns_value_;     /**< CGNS值 */
};

/**
 * @brief PostCGNS模块的参数特性特化
 */
template <> struct ModuleParamTraits<PostCGNS> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/**
 * @brief 后处理Plot3D模块
 */
class PostPlot3D {
public:
    /**
     * @brief 构造函数
     * @param params 模块参数
     */
    explicit PostPlot3D(const nlohmann::json& params);
    
    /**
     * @brief 初始化模块
     */
    void initialize();
    
    /**
     * @brief 执行模块
     */
    void execute();
    
    /**
     * @brief 释放模块
     */
    void release();
    
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
    
private:
    std::string plot3d_type_; /**< Plot3D类型 */
    double plot3d_value_;     /**< Plot3D值 */
};

/**
 * @brief PostPlot3D模块的参数特性特化
 */
template <> struct ModuleParamTraits<PostPlot3D> {
    /**
     * @brief 获取参数架构
     * @return 参数架构JSON对象
     */
    static nlohmann::json GetParamSchema();
};

/** @} */ // end of Modules group

} // namespace ModuleSystem