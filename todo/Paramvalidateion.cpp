#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <nlohmann/json.hpp>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

using json = nlohmann::json;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::exception;
using std::min;

// 验证参数函数
bool ParameterValidation(const json& defaultJson, json& instanceJson) {
    try {
        std::vector<string> errorMessages; // 用于收集所有错误信息
        
        // 遍历defaultJson中的所有根元素
        for (auto schemaIt = defaultJson.begin(); schemaIt != defaultJson.end(); ++schemaIt) {
            const string& rootKey = schemaIt.key();
            
            // 检查根元素是否存在于instanceJson中
            if (!instanceJson.contains(rootKey)) {
                cout << "错误: 文件中缺少根节点 " << rootKey << endl;
                return false; // 如果根节点缺失, 直接返回错误
            }
            
            // 验证子节点
            const auto& schemaValue = schemaIt.value();
            if (!schemaValue.contains("childNode")) {
                // 如果默认配置中没有childNode, 则不需要对这部分进行验证
                cout << "信息: " << rootKey << " 在模板中不包含childNode节点, 跳过此节点验证" << endl;
                continue;
            }
            
            const json& schemaChildNodes = schemaValue["childNode"];
            
            // 确保instanceJson中有childNode字段
            auto& instanceJsonRoot = instanceJson[rootKey];
            if (!instanceJsonRoot.contains("childNode")) {
                cout << "警告: " << rootKey << " 缺少childNode, 跳过该节点验证" << endl;
                continue; // 跳过childNode的验证
            }
            
            // 验证childNode是否为对象类型
            if (!instanceJsonRoot["childNode"].is_object()) {
                errorMessages.push_back("错误: " + rootKey + " 的childNode不是对象类型");
                continue;
            }
            
            // 获取childNode对象引用
            auto& childNodeObj = instanceJsonRoot["childNode"];
            
            // 验证每个子节点
            for (auto childIt = schemaChildNodes.begin(); childIt != schemaChildNodes.end(); ++childIt) {
                const string& childKey = childIt.key();
                const json& childSchema = childIt.value();
                
                // 提取类型信息, 减少重复访问
                string nodeType;
                if (childSchema.contains("type")) {
                    nodeType = childSchema["type"].get<string>();
                }
                
                // 处理engineEnum类型 - 需要特殊验证
                if (nodeType == "engineEnum") {
                    if (!childSchema.contains("engineEnumList")) {
                        errorMessages.push_back("错误: " + childKey + " 的枚举列表未定义");
                        continue;
                    }
                    
                    // 检查childNode对象是否包含此键
                    if (!childNodeObj.contains(childKey)) {
                        if (childSchema.contains("default")) {
                            cout << "警告: 子节点 " << childKey << " 不存在, 添加默认值" << endl;
                            childNodeObj[childKey] = childSchema["default"];
                        } else {
                            errorMessages.push_back("错误: 缺少子节点 " + childKey);
                        }
                        continue;
                    }
                    
                    // 检查值是否是有效的枚举
                    const auto& enumList = childSchema["engineEnumList"];
                    const auto& actualValue = childNodeObj[childKey];
                    
                    // 值必须是字符串类型
                    if (!actualValue.is_string()) {
                        errorMessages.push_back("错误: 子节点 " + childKey + " 的值必须是字符串类型");
                        continue;
                    }
                    
                    bool validEnum = false;
                    const string& valueStr = actualValue.get<string>();
                    
                    // 检查是否是有效的枚举值
                    for (const auto& enumValue : enumList) {
                        if (valueStr == enumValue.get<string>()) {
                            validEnum = true;
                            break;
                        }
                    }
                    
                    if (!validEnum) {
                        string errorMsg = "错误: " + childKey + " 的值 '" + valueStr + "' 无效, 必须是以下值之一: ";
                        for (size_t i = 0; i < enumList.size(); ++i) {
                            errorMsg += enumList[i].get<string>();
                            if (i < enumList.size() - 1) {
                                errorMsg += ", ";
                            }
                        }
                        errorMessages.push_back(errorMsg);
                    }
                } else if (nodeType == "egineBool") {
                    // 处理布尔类型节点
                    if (!childNodeObj.contains(childKey)) {
                        if (childSchema.contains("default")) {
                            cout << "警告: 子节点 " << childKey << " 不存在, 添加默认值" << endl;
                            childNodeObj[childKey] = childSchema["default"];
                        } else {
                            errorMessages.push_back("错误: 缺少子节点 " + childKey);
                        }
                        continue;
                    }
                    
                    // 检查值是否是布尔类型
                    if (!childNodeObj[childKey].is_boolean()) {
                        errorMessages.push_back("错误: 子节点 " + childKey + " 的值必须是布尔类型");
                    }
                }
                // 可以根据需要添加其他类型的处理
            }
            
            // 验证engineConfig (保持原有逻辑)
            if (schemaValue.contains("engineConfig")) {
                const json& schemaConfig = schemaIt.value()["engineConfig"];

                std::cout << schemaConfig.dump(4) << std::endl; // 输出schemaConfig内容用于调试

                  // 检查instanceJson中是否有engineConfig字段
                if (!instanceJsonRoot.contains("engineConfig")) {
                    cout << "警告: " << rootKey << " 缺少engineConfig, 将使用默认值" << endl;
                    instanceJsonRoot["engineConfig"] = json::object();
                }
                
                auto& fileConfig = instanceJsonRoot["engineConfig"];
                
                // 验证每个配置参数
                for (auto configIt = schemaConfig.begin(); configIt != schemaConfig.end(); ++configIt) {
                    const string& configKey = configIt.key();
                    const json& configSchema = configIt.value();
                    
                    // 检查参数是否存在
                    if (!fileConfig.contains(configKey)) {
                        if (configSchema.contains("default")) {
                            cout << "警告: 参数 " << configKey << " 不存在, 使用默认值" << endl;
                            fileConfig[configKey] = configSchema["default"];
                        } else {
                            errorMessages.push_back("错误: 参数 " + configKey + " 不存在且没有默认值");
                            continue;
                        }
                    }
                    
                    // 获取实际参数值
                    json& configValue = fileConfig[configKey];
                    
                    // 如果不包含类型定义, 跳过类型检查
                    if (!configSchema.contains("type")) {
                        continue;
                    }
                    
                    const string& type = configSchema["type"].get<string>();
                    
                    // 使用switch-case替代多个if-else, 使逻辑更清晰
                    if (type == "number") {
                        // 验证number类型
                        if (!configValue.is_number()) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 应为number类型");
                            continue;
                        }
                        
                        // 验证最大值
                        if (configSchema.contains("maxValue") && configSchema["maxValue"].is_number() &&
                            configValue > configSchema["maxValue"]) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 超出最大值 " + 
                                                   configSchema["maxValue"].dump());
                        }
                        
                        // 验证最小值
                        if (configSchema.contains("minValue") && configSchema["minValue"].is_number() &&
                            configValue < configSchema["minValue"]) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 小于最小值 " + 
                                                   configSchema["minValue"].dump());
                        }
                    } else if (type == "string") {
                        // 验证string类型
                        if (!configValue.is_string()) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 应为string类型");
                        }
                    } else if (type == "valueBool" || type == "egineBool") {
                        // 验证布尔类型
                        if (!configValue.is_boolean()) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 应为布尔类型");
                        }
                    } else if (type == "array") {
                        // 验证数组类型
                        if (!configValue.is_array()) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 应为数组类型");
                            continue;
                        }
                        
                        // 检查数组每个元素的范围
                        if (configSchema.contains("maxValue") && configSchema["maxValue"].is_array() && 
                            configSchema.contains("minValue") && configSchema["minValue"].is_array()) {
                            
                            const auto& maxValues = configSchema["maxValue"];
                            const auto& minValues = configSchema["minValue"];
                            size_t maxSize = min(configValue.size(), maxValues.size());
                            
                            for (size_t i = 0; i < maxSize; i++) {
                                if (!configValue[i].is_number()) {
                                    errorMessages.push_back("错误: 数组 " + configKey + " 中元素 " + 
                                                          std::to_string(i) + " 不是数值类型");
                                    continue;
                                }
                                
                                const auto& currentValue = configValue[i];
                                const auto& maxValue = maxValues[i];
                                const auto& minValue = minValues[i];
                                
                                if (currentValue > maxValue) {
                                    errorMessages.push_back("错误: 数组 " + configKey + " 中元素 " + 
                                                          std::to_string(i) + " 超出最大值 " + maxValue.dump());
                                }
                                
                                if (currentValue < minValue) {
                                    errorMessages.push_back("错误: 数组 " + configKey + " 中元素 " + 
                                                          std::to_string(i) + " 小于最小值 " + minValue.dump());
                                }
                            }
                        }
                    } else if (type == "enumList") {
                        // 验证枚举列表类型
                        if (!configValue.is_string() && !configValue.is_array()) {
                            errorMessages.push_back("错误: 参数 " + configKey + " 应为字符串或数组类型的枚举值");
                            continue;
                        }
                        
                        // 如果包含枚举值列表, 验证枚举值的有效性
                        if (!configSchema.contains("engineEnumList")) {
                            continue;
                        }
                        
                        const auto& enumList = configSchema["engineEnumList"];
                        
                        // 创建枚举值集合用于快速查找
                        std::unordered_set<string> validEnumSet;
                        for (const auto& enumValue : enumList) {
                            validEnumSet.insert(enumValue.get<string>());
                        }
                        
                        if (configValue.is_string()) {
                            const string& valueStr = configValue.get<string>();
                            if (validEnumSet.find(valueStr) == validEnumSet.end()) {
                                errorMessages.push_back("错误: " + valueStr + " 不是有效的枚举值");
                            }
                        } else if (configValue.is_array()) {
                            for (const auto& item : configValue) {
                                const string& valueStr = item.get<string>();
                                if (validEnumSet.find(valueStr) == validEnumSet.end()) {
                                    errorMessages.push_back("错误: " + valueStr + " 不是有效的枚举值");
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // 检查是否收集到错误信息, 如果有则输出所有错误并返回false
        if (!errorMessages.empty()) {
            cerr << "\n验证过程中发现 " << errorMessages.size() << " 个错误：" << endl;
            for (size_t i = 0; i < errorMessages.size(); ++i) {
                cerr << (i + 1) << ". " << errorMessages[i] << endl;
            }
            return false;
        }
        
        return true;
    } catch (const exception& e) {
        cerr << "验证过程中发生异常: " << e.what() << endl;
        return false;
    }
}

// 创建示例schema JSON
json createdefaultJson() {
    return {{
        "Name", {
            {"description", "description"},
            {"type", "engine"},
            {"childNode", {
                {"engine1", {
                    {"description", "description"},
                    {"type", "engineEnum"},
                    {"engineEnumList", {"module1", "module2"}},
                    {"default", "module1"}}},
                {"module3", {
                    {"description", "description"},
                    {"type", "egineBool"},
                    {"default", true}}},
                {"engine3", {
                    {"description", "description"},
                    {"type", "egineBool"},
                    {"default", true}}}
            }},
            {"engineConfig", {
                {"numberOfvalue", {
                    {"description", "number of value"},
                    {"type", "number"},
                    {"default", 1},
                    {"maxValue", 999},
                    {"minValue", 0}}},
                {"numberOfvalue1", {
                    {"description", "string of value"},
                    {"type", "string"},
                    {"default", "strnumberOfvalue1"}}},
                {"numberOfvalue2", {
                    {"description", "number of value"},
                    {"type", "array"},
                    {"maxValue", {10, 10, 10, 10}},
                    {"minValue", {0, 0, 0, 0}},
                    {"default", {1, 2, 3, 4}}}}
            }}
        }}
    };
}

int main() {
#ifdef _WIN32
    // Set console code page to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_BINARY);
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#endif    // 创建schema JSON
    json defaultJson = createdefaultJson();
    
    // 读取JSON文件
    json instanceJson;
    try {
        std::ifstream file("config.json");
        if (!file.is_open()) {
            cerr << "无法打开文件 config.json, 将使用默认值" << endl;
            // 使用新的默认示例 - 使用对象形式的childNode
            instanceJson = {
                {"Name", {
                    {"childNode", {
                        {"engine1", "module1"},
                        {"module3", true},
                        {"engine3", true}
                    }},
                    {"engineConfig", {
                        {"numberOfvalue", 1},
                        {"numberOfvalue1", "strnumberOfvalue1"}
                    }}
                }}
            };
        } else {
            file >> instanceJson;
        }
    } catch (const json::parse_error& e) {
        cerr << "解析JSON文件时出错: " << e.what() << endl;
        return 1;
    } catch (const std::exception& e) {
        cerr << "读取文件时出错: " << e.what() << endl;
        return 1;
    }
      // 执行参数验证
    bool isValid = ParameterValidation(defaultJson, instanceJson);
    
    if (isValid) {
        cout << "验证通过！" << endl;
        
        // 只有验证通过才打印验证后的JSON
        cout << "验证后的JSON:" << endl;
        cout << instanceJson.dump(4) << endl;
    } else {
        cout << "验证失败！" << endl;
        return 1; // 验证失败时直接退出程序
    }
    
    return 0;
}
