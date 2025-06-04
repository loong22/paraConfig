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

#include "paraConfig.h"
#include <regex> // Required for GetBaseName
#include <filesystem> // Required for WriteJsonFile, LoadGlobalConfig

// --- Helper Function Implementations ---

/**
 * @brief Extracts the base name from an instance name.
 * For example, "ModuleClass_1" becomes "ModuleClass". This is used to map
 * an instance name (e.g., "ModulePreCGNS_custom") to its base type ("ModulePreCGNS")
 * for schema lookup or factory instantiation.
 * @param instance_name The full instance name, potentially with a suffix like "_1" or "_custom".
 * @return The base name of the component.
 */
static std::string GetBaseName(const std::string& instance_name) {
    std::regex pattern("(_[0-9]+)$"); // Matches _N at the end of the string
    return std::regex_replace(instance_name, pattern, "");
}

void WriteJsonFile(const std::string& dir_path, const std::string& filename, const nlohmann::json& j_obj) {
    std::filesystem::path full_path = std::filesystem::path(dir_path) / filename;
    std::filesystem::create_directories(full_path.parent_path()); // Ensure directory exists
    std::ofstream ofs(full_path);
    if (!ofs.is_open()) {
        // Consider throwing an exception for better error handling by caller
        std::cerr << "Error: Cannot open file for writing: " << full_path << std::endl;
        return;
    }
    ofs << j_obj.dump(4); // Pretty print with 4 spaces
    // std::cout << "Generated config file: " << full_path << std::endl; // Logging can be optional or handled by caller
}

nlohmann::json GenerateDefaultConfigContentFromSchema(const nlohmann::json& schema) {
    nlohmann::json default_content;
    for (auto const& [key, val_schema] : schema.items()) {
        // Skip complex fields that describe nested schemas rather than direct parameters of this component.
        if (key == "module_parameters_schemas" || key == "sub_engine_parameters_schemas") {
            continue;
        }
        if (val_schema.is_object() && val_schema.contains("default")) {
            default_content[key] = val_schema["default"];
        }
    }
    return default_content;
}

void GenerateRegistrationFile(const std::string& dir_path) {
    nlohmann::json register_json;
    
    // Modules
    register_json["ModulePreCGNS"] = ModulePreCGNS::GetParamSchema();
    register_json["ModulePrePlot3D"] = ModulePrePlot3D::GetParamSchema();
    register_json["ModulePreTecplot"] = ModulePreTecplot::GetParamSchema();
    register_json["ModuleSA"] = ModuleSA::GetParamSchema();
    register_json["ModuleSST"] = ModuleSST::GetParamSchema();
    register_json["ModuleSSTWDF"] = ModuleSSTWDF::GetParamSchema();
    register_json["ModulePostCGNS"] = ModulePostCGNS::GetParamSchema();
    register_json["ModulePostPlot3D"] = ModulePostPlot3D::GetParamSchema();
    register_json["ModulePostTecplot"] = ModulePostTecplot::GetParamSchema();

    // Engines
    register_json["EnginePreGrid"] = EnginePreGrid::GetParamSchema();
    register_json["EngineTurbulence"] = EngineTurbulence::GetParamSchema();
    register_json["EngineFlowField"] = EngineFlowField::GetParamSchema();
    register_json["EnginePre"] = EnginePre::GetParamSchema();
    register_json["EngineSolve"] = EngineSolve::GetParamSchema();
    register_json["EnginePost"] = EnginePost::GetParamSchema();
    register_json["EngineMainProcess"] = EngineMainProcess::GetParamSchema();

    WriteJsonFile(dir_path, "register.json", register_json);
    std::cout << "Generated registration file: " << (std::filesystem::path(dir_path) / "register.json") << std::endl;
}

void WriteDefaultConfigs(const std::string& dir_path) {
    std::filesystem::create_directories(dir_path); // Ensure base directory exists
    GenerateRegistrationFile(dir_path); 

    nlohmann::json combined_config;

    // Helper lambda to add component's default config based on its schema
    auto add_component_defaults = 
        [&](const std::string& component_name, const nlohmann::json& schema) {
        combined_config[component_name] = GenerateDefaultConfigContentFromSchema(schema);
    };

    // Add defaults for all base modules and their documented reuses
    add_component_defaults("ModulePreCGNS", ModulePreCGNS::GetParamSchema());
    // Example of how one might handle a reused instance if it needs specific default section
    // add_component_defaults("ModulePreCGNS_1", ModulePreCGNS::GetParamSchema()); // For config.json structure
    add_component_defaults("ModulePrePlot3D", ModulePrePlot3D::GetParamSchema());
    add_component_defaults("ModulePreTecplot", ModulePreTecplot::GetParamSchema());
    add_component_defaults("ModuleSA", ModuleSA::GetParamSchema());
    add_component_defaults("ModuleSST", ModuleSST::GetParamSchema());
    add_component_defaults("ModuleSSTWDF", ModuleSSTWDF::GetParamSchema());
    add_component_defaults("ModulePostCGNS", ModulePostCGNS::GetParamSchema());
    add_component_defaults("ModulePostPlot3D", ModulePostPlot3D::GetParamSchema());
    add_component_defaults("ModulePostPlot3D_1", ModulePostPlot3D::GetParamSchema()); 
    add_component_defaults("ModulePostPlot3D_2", ModulePostPlot3D::GetParamSchema()); 
    add_component_defaults("ModulePostTecplot", ModulePostTecplot::GetParamSchema());

    // Add defaults for all base engines and their documented reuses
    add_component_defaults("EnginePreGrid", EnginePreGrid::GetParamSchema());
    // add_component_defaults("EnginePreGrid_1", EnginePreGrid::GetParamSchema());
    add_component_defaults("EngineTurbulence", EngineTurbulence::GetParamSchema());
    // add_component_defaults("EngineTurbulence_1", EngineTurbulence::GetParamSchema());
    add_component_defaults("EngineFlowField", EngineFlowField::GetParamSchema());
    // add_component_defaults("EngineFlowField_1", EngineFlowField::GetParamSchema());
    add_component_defaults("EnginePre", EnginePre::GetParamSchema());
    add_component_defaults("EngineSolve", EngineSolve::GetParamSchema());
    add_component_defaults("EnginePost", EnginePost::GetParamSchema());
    add_component_defaults("EnginePost_1", EnginePost::GetParamSchema()); 
    add_component_defaults("EnginePost_2", EnginePost::GetParamSchema()); 
    
    // The main process itself also has parameters (like its own execution_order)
    add_component_defaults("EngineMainProcess", EngineMainProcess::GetParamSchema());

    WriteJsonFile(dir_path, "config.json", combined_config);
    std::cout << "Generated combined default config file: " << (std::filesystem::path(dir_path) / "config.json") << std::endl;
}

void LoadGlobalConfig(const std::filesystem::path& file_path, nlohmann::json& global_config) {
    if (!std::filesystem::exists(file_path)) {
        throw std::runtime_error("Config file not found: " + file_path.string());
    }
    if (std::filesystem::is_directory(file_path)) {
        throw std::runtime_error("Config path is a directory, but a file was expected: " + file_path.string());
    }

    std::ifstream ifs(file_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open config file: " + file_path.string());
    }
    
    try {
        ifs >> global_config;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + file_path.string() + ": " + e.what());
    }
    // ifs.close(); // RAII handles closing
}


// --- ModulePreCGNS ---
ModulePreCGNS::ModulePreCGNS(const nlohmann::json& params) {
    try {
        if (!params.contains("cgns_type")) {
            throw std::runtime_error("ModulePreCGNS: Parameter 'cgns_type' is missing.");
        }
        if (!params.at("cgns_type").is_string()) {
            throw std::runtime_error("ModulePreCGNS: Parameter 'cgns_type' must be a string.");
        }
        cgns_type_ = params.at("cgns_type").get<std::string>();

        if (!params.contains("cgns_value")) {
            throw std::runtime_error("ModulePreCGNS: Parameter 'cgns_value' is missing.");
        }
        if (!params.at("cgns_value").is_number()) {
            throw std::runtime_error("ModulePreCGNS: Parameter 'cgns_value' must be a number.");
        }
        cgns_value_ = params.at("cgns_value").get<double>();
        std::cout << "ModulePreCGNS configured with type: " << cgns_type_ << ", value: " << cgns_value_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePreCGNS configuration error: " + std::string(e.what()));
    }
}
void ModulePreCGNS::Initialize() { std::cout << "ModulePreCGNS initializing..." << std::endl; }
void ModulePreCGNS::Execute() { std::cout << "ModulePreCGNS executing..." << std::endl; }
void ModulePreCGNS::Release() { std::cout << "ModulePreCGNS releasing..." << std::endl; }
nlohmann::json ModulePreCGNS::GetParamSchema() {
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "CGNS文件类型"},
            {"enum", {"HDF5", "ADF", "XML"}},
            {"default", "HDF5"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "CGNS参数值"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 15.0}
        }}
    };
}

// --- ModulePrePlot3D ---
ModulePrePlot3D::ModulePrePlot3D(const nlohmann::json& params) {
    try {
        if (!params.contains("plot3d_option")) {
            throw std::runtime_error("ModulePrePlot3D: Parameter 'plot3d_option' is missing.");
        }
        if (!params.at("plot3d_option").is_string()) {
            throw std::runtime_error("ModulePrePlot3D: Parameter 'plot3d_option' must be a string.");
        }
        plot3d_option_ = params.at("plot3d_option").get<std::string>();
        std::cout << "ModulePrePlot3D configured with option: " << plot3d_option_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePrePlot3D configuration error: " + std::string(e.what()));
    }
}
void ModulePrePlot3D::Initialize() { std::cout << "ModulePrePlot3D initializing..." << std::endl; }
void ModulePrePlot3D::Execute() { std::cout << "ModulePrePlot3D executing..." << std::endl; }
void ModulePrePlot3D::Release() { std::cout << "ModulePrePlot3D releasing..." << std::endl; }
nlohmann::json ModulePrePlot3D::GetParamSchema() {
    return {
        {"plot3d_option", {
            {"type", "string"},
            {"description", "Plot3D specific option"},
            {"default", "default_option"}
        }}
    };
}

// --- ModulePreTecplot ---
ModulePreTecplot::ModulePreTecplot(const nlohmann::json& params) {
    try {
        if (!params.contains("tecplot_binary_format")) {
            throw std::runtime_error("ModulePreTecplot: Parameter 'tecplot_binary_format' is missing.");
        }
        if (!params.at("tecplot_binary_format").is_boolean()) {
            throw std::runtime_error("ModulePreTecplot: Parameter 'tecplot_binary_format' must be a boolean.");
        }
        tecplot_binary_format_ = params.at("tecplot_binary_format").get<bool>();
        std::cout << "ModulePreTecplot configured with binary_format: " << tecplot_binary_format_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePreTecplot configuration error: " + std::string(e.what()));
    }
}
void ModulePreTecplot::Initialize() { std::cout << "ModulePreTecplot initializing..." << std::endl; }
void ModulePreTecplot::Execute() { std::cout << "ModulePreTecplot executing..." << std::endl; }
void ModulePreTecplot::Release() { std::cout << "ModulePreTecplot releasing..." << std::endl; }
nlohmann::json ModulePreTecplot::GetParamSchema() {
    return {
        {"tecplot_binary_format", {
            {"type", "boolean"},
            {"description", "Use Tecplot binary format"},
            {"default", true}
        }}
    };
}


// --- ModuleSA ---
ModuleSA::ModuleSA(const nlohmann::json& params) { 
    try {
        if (params.contains("sa_constant") && !params.at("sa_constant").is_number()) {
            throw std::runtime_error("ModuleSA: Parameter 'sa_constant' must be a number.");
        }
        sa_constant_ = params.value("sa_constant", kSaMaxValue_); // Default to kSaMaxValue_ if not specified
        std::cout << "ModuleSA configured with sa_constant: " << sa_constant_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModuleSA configuration error: " + std::string(e.what()));
    }
}
void ModuleSA::Initialize() { std::cout << "ModuleSA initializing..." << std::endl; }
void ModuleSA::Execute() { std::cout << "ModuleSA executing..." << std::endl; }
void ModuleSA::Release() { std::cout << "ModuleSA releasing..." << std::endl; }
nlohmann::json ModuleSA::GetParamSchema() {
    return {
        {"sa_constant", {
            {"type", "number"}, {"description", "SA model constant"}, {"default", 0.41} // kSaMaxValue_
        }}
    };
}

// --- ModuleSST ---
ModuleSST::ModuleSST(const nlohmann::json& params) { 
    try {
        if (params.contains("sst_iterations") && !params.at("sst_iterations").is_number_integer()) {
            throw std::runtime_error("ModuleSST: Parameter 'sst_iterations' must be an integer.");
        }
        sst_iterations_ = params.value("sst_iterations", 100); 
        std::cout << "ModuleSST configured with sst_iterations: " << sst_iterations_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModuleSST configuration error: " + std::string(e.what()));
    }
}
void ModuleSST::Initialize() { std::cout << "ModuleSST initializing..." << std::endl; }
void ModuleSST::Execute() { std::cout << "ModuleSST executing..." << std::endl; }
void ModuleSST::Release() { std::cout << "ModuleSST releasing..." << std::endl; }
nlohmann::json ModuleSST::GetParamSchema() {
    return {
        {"sst_iterations", {
            {"type", "integer"}, {"description", "SST model iterations"}, {"default", 100}
        }}
    };
}

// --- ModuleSSTWDF ---
ModuleSSTWDF::ModuleSSTWDF(const nlohmann::json& params) { 
    try {
        if (params.contains("wdf_model_name") && !params.at("wdf_model_name").is_string()) {
            throw std::runtime_error("ModuleSSTWDF: Parameter 'wdf_model_name' must be a string.");
        }
        wdf_model_name_ = params.value("wdf_model_name", "StandardWDF"); 
        std::cout << "ModuleSSTWDF configured with wdf_model_name: " << wdf_model_name_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModuleSSTWDF configuration error: " + std::string(e.what()));
    }
}
void ModuleSSTWDF::Initialize() { std::cout << "ModuleSSTWDF initializing..." << std::endl; }
void ModuleSSTWDF::Execute() { std::cout << "ModuleSSTWDF executing..." << std::endl; }
void ModuleSSTWDF::Release() { std::cout << "ModuleSSTWDF releasing..." << std::endl; }
nlohmann::json ModuleSSTWDF::GetParamSchema() {
    return {
        {"wdf_model_name", {
            {"type", "string"}, {"description", "Wall Damping Function model name"}, {"default", "StandardWDF"}
        }}
    };
}

// --- ModulePostCGNS ---
ModulePostCGNS::ModulePostCGNS(const nlohmann::json& params) { 
    try {
        if (params.contains("output_cgns_name") && !params.at("output_cgns_name").is_string()) {
            throw std::runtime_error("ModulePostCGNS: Parameter 'output_cgns_name' must be a string.");
        }
        output_cgns_name_ = params.value("output_cgns_name", "results.cgns"); 
        std::cout << "ModulePostCGNS configured with output_cgns_name: " << output_cgns_name_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePostCGNS configuration error: " + std::string(e.what()));
    }
}
void ModulePostCGNS::Initialize() { std::cout << "ModulePostCGNS initializing..." << std::endl; }
void ModulePostCGNS::Execute() { std::cout << "ModulePostCGNS executing..." << std::endl; }
void ModulePostCGNS::Release() { std::cout << "ModulePostCGNS releasing..." << std::endl; }
nlohmann::json ModulePostCGNS::GetParamSchema() {
    return {
        {"output_cgns_name", {
            {"type", "string"}, {"description", "Output CGNS filename"}, {"default", "results.cgns"}
        }}
    };
}

// --- ModulePostPlot3D ---
ModulePostPlot3D::ModulePostPlot3D(const nlohmann::json& params) { 
    try {
        if (params.contains("write_q_file") && !params.at("write_q_file").is_boolean()) {
            throw std::runtime_error("ModulePostPlot3D: Parameter 'write_q_file' must be a boolean.");
        }
        write_q_file_ = params.value("write_q_file", true); 
        std::cout << "ModulePostPlot3D configured with write_q_file: " << write_q_file_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePostPlot3D configuration error: " + std::string(e.what()));
    }
}
void ModulePostPlot3D::Initialize() { 
    std::cout << "ModulePostPlot3D initializing..." << std::endl;
    if (write_q_file_) {
        std::cout << "ModulePostPlot3D will write Q-file." << std::endl;
    } else {
        std::cout << "ModulePostPlot3D will not write Q-file." << std::endl;
    }
}
void ModulePostPlot3D::Execute() { std::cout << "ModulePostPlot3D executing..." << std::endl; }
void ModulePostPlot3D::Release() { std::cout << "ModulePostPlot3D releasing..." << std::endl; }
nlohmann::json ModulePostPlot3D::GetParamSchema() {
    return {
        {"write_q_file", {
            {"type", "boolean"}, {"description", "Write Q-file for Plot3D"}, {"default", true}
        }}
    };
}

// --- ModulePostTecplot ---
ModulePostTecplot::ModulePostTecplot(const nlohmann::json& params) { 
    try {
        if (params.contains("tecplot_zone_title") && !params.at("tecplot_zone_title").is_string()) {
            throw std::runtime_error("ModulePostTecplot: Parameter 'tecplot_zone_title' must be a string.");
        }
        tecplot_zone_title_ = params.value("tecplot_zone_title", "DefaultZone"); 
        std::cout << "ModulePostTecplot configured with tecplot_zone_title: " << tecplot_zone_title_ << std::endl;
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("ModulePostTecplot configuration error: " + std::string(e.what()));
    }
}
void ModulePostTecplot::Initialize() { std::cout << "ModulePostTecplot initializing..." << std::endl; }
void ModulePostTecplot::Execute() { std::cout << "ModulePostTecplot executing..." << std::endl; }
void ModulePostTecplot::Release() { std::cout << "ModulePostTecplot releasing..." << std::endl; }
nlohmann::json ModulePostTecplot::GetParamSchema() {
    return {
        {"tecplot_zone_title", {
            {"type", "string"}, {"description", "Tecplot zone title"}, {"default", "DefaultZone"}
        }}
    };
}


// --- EnginePreGrid ---
EnginePreGrid::EnginePreGrid(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    const auto& config = instance_params; 

    if (!config.contains("module_execution_order")) {
        throw std::runtime_error("EnginePreGrid: 'module_execution_order' key is missing in instance parameters.");
    }
    try {
        module_execution_order_ = config.at("module_execution_order").get<std::vector<std::string>>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("EnginePreGrid: 'module_execution_order' must be an array of strings. Error: " + std::string(e.what()));
    }
    std::cout << "EnginePreGrid configuring with module order: ";
    for(const auto& key : module_execution_order_) { std::cout << key << " "; }
    std::cout << std::endl;

    for (const std::string& module_instance_name : module_execution_order_) {
        if (!global_config.contains(module_instance_name)) {
            throw std::runtime_error("EnginePreGrid: Parameters for module instance '" + module_instance_name + "' not found in global_config.");
        }
        const auto& current_module_params = global_config.at(module_instance_name);
        std::cout << "EnginePreGrid: Loading module " << module_instance_name << " with params: " << current_module_params.dump(2) << std::endl;
        
        std::string base_module_name = GetBaseName(module_instance_name);

        auto it_map = module_key_map_.find(base_module_name);
        if (it_map == module_key_map_.end()) {
            throw std::runtime_error("EnginePreGrid: Unknown base module key: " + base_module_name + " (from instance " + module_instance_name + ")");
        }

        switch (it_map->second) {
            case 0: // ModulePreCGNS
                active_modules_[module_instance_name] = std::make_unique<ModulePreCGNS>(current_module_params);
                break;
            case 1: // ModulePrePlot3D
                active_modules_[module_instance_name] = std::make_unique<ModulePrePlot3D>(current_module_params);
                break;
            case 2: // ModulePreTecplot
                active_modules_[module_instance_name] = std::make_unique<ModulePreTecplot>(current_module_params);
                break;
            default: 
                throw std::runtime_error("EnginePreGrid: Unknown module key in execution order: " + module_instance_name);
        }
    }
}
void EnginePreGrid::Initialize() {
    std::cout << "EnginePreGrid initializing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Initialize(); }, it->second);
        }
    }
}
void EnginePreGrid::Execute() {
    std::cout << "EnginePreGrid executing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Execute(); }, it->second);
        }
    }
}
void EnginePreGrid::Release() {
    std::cout << "EnginePreGrid releasing modules in (reverse) order..." << std::endl;
    for (auto key_it = module_execution_order_.rbegin(); key_it != module_execution_order_.rend(); ++key_it) {
        const auto& module_key = *key_it;
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EnginePreGrid::GetParamSchema() {
    return {
        {"description", "PreGrid Engine: executes a list of preprocessing modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "chooseOne"}}, {"default", "chooseOne"}}},
        {"module_execution_order", { 
            {"type", "array"},
            {"description", "Ordered list of module instance keys to execute. Modules are executed in the given order."},
            {"items", {
                {"type", "string"},
                // Enum can list known base types and common reuses for GUI/validation aid
                {"enum", {"ModulePreCGNS", "ModulePrePlot3D", "ModulePreTecplot", "ModulePreCGNS_1"}} 
            }},
            {"default", {"ModulePreCGNS"}} 
        }},
        {"module_parameters_schemas", {
            {"ModulePreCGNS", ModulePreCGNS::GetParamSchema()},
            {"ModulePrePlot3D", ModulePrePlot3D::GetParamSchema()},
            {"ModulePreTecplot", ModulePreTecplot::GetParamSchema()},
            {"ModulePreCGNS_1", ModulePreCGNS::GetParamSchema()} // Example of reused instance schema
        }}
    };
}

// --- EngineTurbulence ---
EngineTurbulence::EngineTurbulence(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    const auto& config = instance_params;

    if (!config.contains("module_execution_order")) {
        throw std::runtime_error("EngineTurbulence: 'module_execution_order' key is missing in instance parameters.");
    }
    try {
        module_execution_order_ = config.at("module_execution_order").get<std::vector<std::string>>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("EngineTurbulence: 'module_execution_order' must be an array of strings. Error: " + std::string(e.what()));
    }

    std::cout << "EngineTurbulence configuring with module order: ";
    for(const auto& key : module_execution_order_) { std::cout << key << " "; }
    std::cout << std::endl;

    for (const std::string& module_instance_name : module_execution_order_) {
        if (!global_config.contains(module_instance_name)) {
            throw std::runtime_error("EngineTurbulence: Parameters for module instance '" + module_instance_name + "' not found in global_config.");
        }
        const auto& current_module_params = global_config.at(module_instance_name);
        std::cout << "EngineTurbulence: Loading module " << module_instance_name << " with params: " << current_module_params.dump(2) << std::endl;
        
        std::string base_module_name = GetBaseName(module_instance_name);

        if (base_module_name == "ModuleSA") {
            active_modules_[module_instance_name] = std::make_unique<ModuleSA>(current_module_params);
        } else if (base_module_name == "ModuleSST") {
            active_modules_[module_instance_name] = std::make_unique<ModuleSST>(current_module_params);
        } else if (base_module_name == "ModuleSSTWDF") {
            active_modules_[module_instance_name] = std::make_unique<ModuleSSTWDF>(current_module_params);
        } else {
            throw std::runtime_error("EngineTurbulence: Unknown base module key: " + base_module_name + " (from instance " + module_instance_name + ")");
        }
    }
}
void EngineTurbulence::Initialize() { 
    std::cout << "EngineTurbulence initializing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Initialize(); }, it->second);
        }
    }
}
void EngineTurbulence::Execute() { 
    std::cout << "EngineTurbulence executing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Execute(); }, it->second);
        }
    }
}
void EngineTurbulence::Release() { 
    std::cout << "EngineTurbulence releasing modules in (reverse) order..." << std::endl;
    for (auto key_it = module_execution_order_.rbegin(); key_it != module_execution_order_.rend(); ++key_it) {
        const auto& module_key = *key_it;
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EngineTurbulence::GetParamSchema() {
     return {
        {"description", "Turbulence Engine: executes a list of turbulence modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "chooseOne"}}, {"default", "chooseOne"}}},
        {"module_execution_order", {
            {"type", "array"},
            {"description", "Ordered list of module instance keys to execute for Turbulence."},
            {"items", {
                {"type", "string"},
                {"enum", {"ModuleSA", "ModuleSST", "ModuleSSTWDF", "ModuleSA_1"}}
            }},
            {"default", {"ModuleSA"}} 
        }},
        {"module_parameters_schemas", {
            {"ModuleSA", ModuleSA::GetParamSchema()},
            {"ModuleSST", ModuleSST::GetParamSchema()},
            {"ModuleSSTWDF", ModuleSSTWDF::GetParamSchema()},
            {"ModuleSA_1", ModuleSA::GetParamSchema()} // Example of reused instance schema
        }}
    };
}

// --- EngineFlowField ---
EngineFlowField::EngineFlowField(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    const auto& config = instance_params;

    if (!config.contains("module_execution_order")) {
        throw std::runtime_error("EngineFlowField: 'module_execution_order' key is missing in instance parameters.");
    }
    try {
        module_execution_order_ = config.at("module_execution_order").get<std::vector<std::string>>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("EngineFlowField: 'module_execution_order' must be an array of strings. Error: " + std::string(e.what()));
    }
    
    std::cout << "EngineFlowField configuring with module order: ";
    for(const auto& key : module_execution_order_) { std::cout << key << " "; }
    std::cout << std::endl;

    for (const std::string& module_instance_name : module_execution_order_) {
        if (!global_config.contains(module_instance_name)) {
            throw std::runtime_error("EngineFlowField: Parameters for module instance '" + module_instance_name + "' not found in global_config.");
        }
        const auto& current_module_params = global_config.at(module_instance_name);
        std::cout << "EngineFlowField: Loading module " << module_instance_name << " with params: " << current_module_params.dump(2) << std::endl;
        
        std::string base_module_name = GetBaseName(module_instance_name);

        if (base_module_name == "ModulePostCGNS") {
            active_modules_[module_instance_name] = std::make_unique<ModulePostCGNS>(current_module_params);
        } else if (base_module_name == "ModulePostPlot3D") {
            active_modules_[module_instance_name] = std::make_unique<ModulePostPlot3D>(current_module_params);
        } else if (base_module_name == "ModulePostTecplot") {
            active_modules_[module_instance_name] = std::make_unique<ModulePostTecplot>(current_module_params);
        } else {
            throw std::runtime_error("EngineFlowField: Unknown base module key: " + base_module_name + " (from instance " + module_instance_name + ")");
        }
    }
}
void EngineFlowField::Initialize() { 
    std::cout << "EngineFlowField initializing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Initialize(); }, it->second);
        }
    }
}
void EngineFlowField::Execute() { 
    std::cout << "EngineFlowField executing modules in order..." << std::endl;
    for (const auto& module_key : module_execution_order_) {
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Execute(); }, it->second);
        }
    }
}
void EngineFlowField::Release() { 
    std::cout << "EngineFlowField releasing modules in (reverse) order..." << std::endl;
    for (auto key_it = module_execution_order_.rbegin(); key_it != module_execution_order_.rend(); ++key_it) {
        const auto& module_key = *key_it;
        auto it = active_modules_.find(module_key);
        if (it != active_modules_.end()) {
            std::visit([](auto&& mod_ptr) { if (mod_ptr) mod_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EngineFlowField::GetParamSchema() {
    return {
        {"description", "FlowField Engine: executes a list of postprocessing modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "sequential"}}, {"default", "sequential_modules"}}}, // Note: "sequential" was also in enum, "sequential_modules" seems more specific.
        {"module_execution_order", {
            {"type", "array"},
            {"description", "Ordered list of module instance keys to execute for FlowField."},
            {"items", {
                {"type", "string"},
                {"enum", {"ModulePostCGNS", "ModulePostPlot3D", "ModulePostTecplot", "ModulePostPlot3D_1", "ModulePostPlot3D_2"}}
            }},
            {"default", {"ModulePostCGNS"}}
        }},
        {"module_parameters_schemas", { 
            {"ModulePostCGNS", ModulePostCGNS::GetParamSchema()},
            {"ModulePostPlot3D", ModulePostPlot3D::GetParamSchema()},
            {"ModulePostTecplot", ModulePostTecplot::GetParamSchema()},
            {"ModulePostPlot3D_1", ModulePostPlot3D::GetParamSchema()}, 
            {"ModulePostPlot3D_2", ModulePostPlot3D::GetParamSchema()}
        }}
    };
}

// --- EnginePre ---
EnginePre::EnginePre(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    std::cout << "EnginePre configuring..." << std::endl;
    const auto& config = instance_params;

    execution_order_ = config.value("execution_order", GetParamSchema().at("execution_order").at("default").get<std::vector<std::string>>());

    for (const std::string& sub_engine_instance_name : execution_order_) {
        if (!global_config.contains(sub_engine_instance_name)) {
            throw std::runtime_error("EnginePre: Config for sub-engine instance '" + sub_engine_instance_name + "' not found in global_config.");
        }
        std::string base_sub_engine_name = GetBaseName(sub_engine_instance_name);
        if (base_sub_engine_name == "EnginePreGrid") {
            active_sub_engines_[sub_engine_instance_name] = std::make_unique<EnginePreGrid>(global_config.at(sub_engine_instance_name), global_config);
        } else {
            throw std::runtime_error("EnginePre: Unknown sub-engine base name: " + base_sub_engine_name + " (from instance " + sub_engine_instance_name + ")");
        }
    }
}
void EnginePre::Initialize() { 
    std::cout << "EnginePre initializing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Initialize(); }, it->second);
        }
    }
}
void EnginePre::Execute() { 
    std::cout << "EnginePre executing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Execute(); }, it->second);
        }
    }
}
void EnginePre::Release() { 
    std::cout << "EnginePre releasing..." << std::endl; 
    for (auto key_it = execution_order_.rbegin(); key_it != execution_order_.rend(); ++key_it) {
        const auto& sub_engine_name = *key_it;
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EnginePre::GetParamSchema() {
    return {
        {"description", "Pre Engine: manages pre-processing stage"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential"}}, {"default", "sequential"}}},
        {"execution_order", {
            {"type", "array"}, 
            {"items", {
                {"type", "string"}, 
                {"enum", {"EnginePreGrid", "EnginePreGrid_1"}}
            }}, 
            {"default", {"EnginePreGrid"}} 
        }},
        {"sub_engine_parameters_schemas", {
            {"EnginePreGrid", EnginePreGrid::GetParamSchema()},
            {"EnginePreGrid_1", EnginePreGrid::GetParamSchema()} // Points to base schema
        }}
    };
}

// --- EngineSolve ---
EngineSolve::EngineSolve(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    std::cout << "EngineSolve configuring..." << std::endl;
    const auto& config = instance_params;
    execution_order_ = config.value("execution_order", GetParamSchema().at("execution_order").at("default").get<std::vector<std::string>>());

    for (const std::string& sub_engine_instance_name : execution_order_) {
        if (!global_config.contains(sub_engine_instance_name)) {
            throw std::runtime_error("EngineSolve: Config for sub-engine instance '" + sub_engine_instance_name + "' not found in global_config.");
        }
        std::string base_sub_engine_name = GetBaseName(sub_engine_instance_name);
        if (base_sub_engine_name == "EngineTurbulence") {
            active_sub_engines_[sub_engine_instance_name] = std::make_unique<EngineTurbulence>(global_config.at(sub_engine_instance_name), global_config);
        } else {
            throw std::runtime_error("EngineSolve: Unknown sub-engine base name: " + base_sub_engine_name + " (from instance " + sub_engine_instance_name + ")");
        }
    }
}
void EngineSolve::Initialize() { 
    std::cout << "EngineSolve initializing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Initialize(); }, it->second);
        }
    }
}
void EngineSolve::Execute() { 
    std::cout << "EngineSolve executing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Execute(); }, it->second);
        }
    }
}
void EngineSolve::Release() { 
    std::cout << "EngineSolve releasing..." << std::endl; 
    for (auto key_it = execution_order_.rbegin(); key_it != execution_order_.rend(); ++key_it) {
        const auto& sub_engine_name = *key_it;
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EngineSolve::GetParamSchema() {
     return {
        {"description", "Solve Engine: manages solver stage"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential"}}, {"default", "sequential"}}},
        {"execution_order", {
            {"type", "array"}, 
            {"items", {
                {"type", "string"}, 
                {"enum", {"EngineTurbulence", "EngineTurbulence_1"}}
            }}, 
            {"default", {"EngineTurbulence"}} 
        }},
        {"sub_engine_parameters_schemas", {
            {"EngineTurbulence", EngineTurbulence::GetParamSchema()},
            {"EngineTurbulence_1", EngineTurbulence::GetParamSchema()} // Points to base schema
        }}
    };
}

// --- EnginePost ---
EnginePost::EnginePost(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    std::cout << "EnginePost configuring..." << std::endl;
    const auto& config = instance_params;
    execution_order_ = config.value("execution_order", GetParamSchema().at("execution_order").at("default").get<std::vector<std::string>>());

    for (const std::string& sub_engine_instance_name : execution_order_) {
        if (!global_config.contains(sub_engine_instance_name)) {
            throw std::runtime_error("EnginePost: Config for sub-engine instance '" + sub_engine_instance_name + "' not found in global_config.");
        }
        std::string base_sub_engine_name = GetBaseName(sub_engine_instance_name);
        if (base_sub_engine_name == "EngineFlowField") {
            active_sub_engines_[sub_engine_instance_name] = std::make_unique<EngineFlowField>(global_config.at(sub_engine_instance_name), global_config);
        } else {
            throw std::runtime_error("EnginePost: Unknown sub-engine base name: " + base_sub_engine_name + " (from instance " + sub_engine_instance_name + ")");
        }
    }
}
void EnginePost::Initialize() { 
    std::cout << "EnginePost initializing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Initialize(); }, it->second);
        }
    }
}
void EnginePost::Execute() { 
    std::cout << "EnginePost executing..." << std::endl; 
    for (const auto& sub_engine_name : execution_order_) {
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Execute(); }, it->second);
        }
    }
}
void EnginePost::Release() { 
    std::cout << "EnginePost releasing..." << std::endl; 
    for (auto key_it = execution_order_.rbegin(); key_it != execution_order_.rend(); ++key_it) {
        const auto& sub_engine_name = *key_it;
        auto it = active_sub_engines_.find(sub_engine_name);
        if (it != active_sub_engines_.end()) {
            std::visit([](auto&& engine_ptr) { if (engine_ptr) engine_ptr->Release(); }, it->second);
        }
    }
}
nlohmann::json EnginePost::GetParamSchema() {
    return {
        {"description", "Post Engine: manages post-processing stage"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential"}}, {"default", "sequential"}}},
        {"execution_order", {
            {"type", "array"}, 
            {"items", {
                {"type", "string"}, 
                {"enum", {"EngineFlowField", "EngineFlowField_1"}}
            }}, 
            {"default", {"EngineFlowField"}} 
        }},
        {"sub_engine_parameters_schemas", {
            {"EngineFlowField", EngineFlowField::GetParamSchema()},
            {"EngineFlowField_1", EngineFlowField::GetParamSchema()} 
        }}
    };
}

// --- EngineMainProcess ---
// Define and initialize static const members of EngineMainProcess
EngineMainProcess::EngineMainProcess(const nlohmann::json& instance_params, const nlohmann::json& global_config) {
    std::cout << "EngineMainProcess configuring..." << std::endl;
    const auto& config = instance_params; // Config for this specific EngineMainProcess instance

    if (!config.contains("execution_order")) {
        throw std::runtime_error("EngineMainProcess: 'execution_order' key is missing in instance parameters.");
    }
    try {
        execution_order_ = config.at("execution_order").get<std::vector<std::string>>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("EngineMainProcess: 'execution_order' must be an array of strings. Error: " + std::string(e.what()));
    }

    for (const std::string& engine_instance_name : execution_order_) {
        if (!global_config.contains(engine_instance_name)) {
             throw std::runtime_error("EngineMainProcess: Configuration for engine instance '" + engine_instance_name + "' not found in global_config.");
        }
        const auto& current_engine_params = global_config.at(engine_instance_name);
        std::string base_engine_name = GetBaseName(engine_instance_name);
        
        auto it_map = engine_key_map_.find(base_engine_name);
        if (it_map == engine_key_map_.end()) {
             throw std::runtime_error("EngineMainProcess: Unknown base engine key: " + base_engine_name + " (from instance " + engine_instance_name + ")");
        }

        switch (it_map->second) {
            case 0: // EnginePre
                active_engines_[engine_instance_name] = std::make_unique<EnginePre>(current_engine_params, global_config);
                break;
            case 1: // EngineSolve
                active_engines_[engine_instance_name] = std::make_unique<EngineSolve>(current_engine_params, global_config);
                break;
            case 2: // EnginePost
                active_engines_[engine_instance_name] = std::make_unique<EnginePost>(current_engine_params, global_config);
                break;
            default: 
                throw std::runtime_error("EngineMainProcess: Unknown engine key in execution order: " + engine_instance_name);
        }
    }
}

void EngineMainProcess::Initialize() {
    std::cout << "EngineMainProcess initializing..." << std::endl;
    for (const auto& engine_name : execution_order_) {
        auto it = active_engines_.find(engine_name);
        if (it != active_engines_.end()) {
            std::visit([](auto&& engine_ptr) {
                if (engine_ptr) engine_ptr->Initialize();
            }, it->second);
        }
    }
}
void EngineMainProcess::Execute() {
    std::cout << "EngineMainProcess executing..." << std::endl;
    for (const auto& engine_name : execution_order_) {
        auto it = active_engines_.find(engine_name);
        if (it != active_engines_.end()) {
            std::visit([](auto&& engine_ptr) {
                if (engine_ptr) engine_ptr->Execute();
            }, it->second);
        }
    }
}
void EngineMainProcess::Release() {
    std::cout << "EngineMainProcess releasing..." << std::endl;
    for (auto exec_it = execution_order_.rbegin(); exec_it != execution_order_.rend(); ++exec_it) {
        const auto& engine_name = *exec_it;
        auto it = active_engines_.find(engine_name);
        if (it != active_engines_.end()) {
            std::visit([](auto&& engine_ptr) {
                if (engine_ptr) engine_ptr->Release();
            }, it->second);
        }
    }
}

nlohmann::json EngineMainProcess::GetParamSchema() {
    return {
        {"description", "Main Process Engine: orchestrates Pre, Solve, and Post stages"},
        {"execution_type", { 
            {"type", "string"},
            {"enum", {"sequential"}}, 
            {"default", "sequential"}
        }},
        {"execution_order", { 
            {"type", "array"},    
            {"items", {           
                {"type", "string"}, 
                {"enum", {"EnginePre", "EngineSolve", "EnginePost", "EnginePost_1", "EnginePost_2"}} 
            }},
            {"default", {"EnginePre", "EngineSolve", "EnginePost"}} 
        }},
        {"sub_engine_parameters_schemas", {
            {"EnginePre", EnginePre::GetParamSchema()},
            {"EngineSolve", EngineSolve::GetParamSchema()},
            {"EnginePost", EnginePost::GetParamSchema()},
            {"EnginePost_1", EnginePost::GetParamSchema()}, // Points to base schema
            {"EnginePost_2", EnginePost::GetParamSchema()}  // Points to base schema
        }}
    };
}
