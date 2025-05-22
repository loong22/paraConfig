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
#include "advanced_module_system.h"

/**
 * @namespace POST
 * @brief 网格预处理组件模块集合
 */
namespace POST {
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
}

// ModuleParamTraits 特化必须放在 ModuleSystem 命名空间中
namespace ModuleSystem {
    /**
     * @brief PostCGNS 模块的参数特性特化
     */
    template <> struct ModuleParamTraits<POST::PostCGNS> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };

    /**
     * @brief PostPlot3D 模块的参数特性特化
     */
    template <> struct ModuleParamTraits<POST::PostPlot3D> {
        /**
         * @brief 获取参数架构
         * @return 参数架构JSON对象
         */
        static nlohmann::json GetParamSchema();
    };
}