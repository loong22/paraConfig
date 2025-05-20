#pragma once
#include "advanced_module_system.h"

/**
 * @namespace SOLVE
 * @brief 网格预处理组件模块集合
 */
namespace SOLVE {
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

}

// ModuleParamTraits 特化必须放在 ModuleSystem 命名空间中
namespace ModuleSystem {
    /**
     * @brief EulerSolver模块的参数特性特化
     */
    template <> struct ModuleParamTraits<SOLVE::EulerSolver> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };

    /**
     * @brief SASolver模块的参数特性特化
     */
    template <> struct ModuleParamTraits<SOLVE::SASolver> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };

    /**
     * @brief SSTSolver模块的参数特性特化
     */
    template <> struct ModuleParamTraits<SOLVE::SSTSolver> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };
}