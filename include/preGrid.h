#pragma once
#include "advanced_module_system.h"

/**
 * @namespace PREGRID
 * @brief 网格预处理组件模块集合
 */
namespace PREGRID {
    /// 本地类型注册表指针 - 指向全局单例
    extern ModuleSystem::ModuleTypeRegistry* localTypeRegistry;
    
    /// 本地模块工厂指针 - 指向全局单例
    extern ModuleSystem::ModuleFactory* localFactory;

    /**
     * @brief 注册模块类型
     * 将所有预处理网格相关的模块注册到本地注册表中
     */
    void registerTypes();

    /**
     * @brief 获取本地注册的模块类型集合
     * @return 模块类型信息集合
     */
    const auto& getTypes();
    
    /**
     * @brief 获取本地模块工厂的创建函数集合
     * @return 模块创建函数映射
     */
    const auto& getFactories();

    /**
     * @brief 将本地注册表中的类型和工厂导出到全局注册表
     * 此函数应在主程序中调用，用于将本组件中的模块合并到全局系统中
     */
    void exportToGlobalRegistry();

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
}

// ModuleParamTraits 特化必须放在 ModuleSystem 命名空间中
namespace ModuleSystem {
    /**
     * @brief PreCGNS模块的参数特性特化
     */
    template <> struct ModuleParamTraits<PREGRID::PreCGNS> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };

    /**
     * @brief PrePlot3D模块的参数特性特化
     */
    template <> struct ModuleParamTraits<PREGRID::PrePlot3D> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };
}