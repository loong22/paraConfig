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

#include "advanced_module_system.h"
#include "preGrid.h"
#include "solve.h"
#include "post.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char* argv[]) {
    
#ifdef _WIN32
    // 设置控制台代码页为 UTF-8
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#endif

    // 初始化模块系统
    PREGRID::exportToGlobalRegistry();

    SOLVE::exportToGlobalRegistry();

    POST::exportToGlobalRegistry();

    // 获取配置
    ModuleSystem::getConfig(argc, argv);
    
    // 验证通过后执行配置
    //ModuleSystem::run();
    
    // 测试特定引擎的特定操作
    ModuleSystem::test("PreGrid", "create");
    ModuleSystem::test("PreGrid", "initialize");
    ModuleSystem::test("PreGrid", "execute");
    ModuleSystem::test("PreGrid", "release");

    // 测试主引擎的操作，这将包括其所有子引擎
    ModuleSystem::test("mainProcess", "create");
    ModuleSystem::test("mainProcess", "initialize");
    ModuleSystem::test("mainProcess", "execute");
    ModuleSystem::test("mainProcess", "release");

    return 0;
}