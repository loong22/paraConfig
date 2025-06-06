#include "paraConfig.h"
#include <regex> // Required for GetBaseName function
#include <filesystem> // Required for WriteJsonFile, LoadGlobalConfig functions

// Static factory variable definitions - only for engines
PreFactory EnginePreGrid::factory_;
SolveFactory EngineTurbulence::factory_;
PostFactory EngineFlowField::factory_;
MainProcessFactory EnginePre::factory_;
MainProcessFactory EngineSolve::factory_;
MainProcessFactory EnginePost::factory_;



// --- Auxiliary function implementations ---

/**
 * @brief Extract base name from instance name.
 * For example, "ModuleClass_1" becomes "ModuleClass", "ModulePreCGNS_custom" becomes "ModulePreCGNS".
 * This is used to map instance names (e.g., "ModulePreCGNS_custom") to their base type ("ModulePreCGNS")
 * for schema lookup or factory instantiation.
 * @param instance_name Full instance name, possibly with suffix like "_1", "_custom", "_v2", etc.
 * @return Base name of the component.
 */
static std::string GetBaseName(const std::string& instance_name)
{
    std::regex pattern("(_[^_]*)$"); // Match _ followed by any characters until end of string
    return std::regex_replace(instance_name, pattern, "");
}

void WriteJsonFile(const std::string& dir_path, const std::string& filename, const nlohmann::json& j_obj)
{
    std::filesystem::path full_path = std::filesystem::path(dir_path) / filename;
    std::filesystem::create_directories(full_path.parent_path()); // Ensure directory exists
    std::ofstream ofs(full_path);
    if (!ofs.is_open()) {
        // Consider throwing exception for better error handling by caller
        std::cerr << "Error: Unable to open file for writing: " << full_path << std::endl;
        return;
    }
    ofs << j_obj.dump(4); // Use 4 spaces for pretty printing
    // std::cout << "Generated configuration file: " << full_path << std::endl; // Logging optional or handled by caller
}

nlohmann::json GenerateDefaultConfigContentFromSchema(const nlohmann::json& schema)
{
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

void GenerateRegistrationFile(const std::string& dir_path)
{
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

void WriteDefaultConfigs(const std::string& dir_path)
{
    std::filesystem::create_directories(dir_path); // Ensure base directory exists
    GenerateRegistrationFile(dir_path); 

    // Generate separate configuration files for each module and engine
    
    // Module configuration files
    WriteJsonFile(dir_path, "ModulePreCGNS.json", GenerateDefaultConfigContentFromSchema(ModulePreCGNS::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePrePlot3D.json", GenerateDefaultConfigContentFromSchema(ModulePrePlot3D::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePreTecplot.json", GenerateDefaultConfigContentFromSchema(ModulePreTecplot::GetParamSchema()));
    WriteJsonFile(dir_path, "ModuleSA.json", GenerateDefaultConfigContentFromSchema(ModuleSA::GetParamSchema()));
    WriteJsonFile(dir_path, "ModuleSST.json", GenerateDefaultConfigContentFromSchema(ModuleSST::GetParamSchema()));
    WriteJsonFile(dir_path, "ModuleSSTWDF.json", GenerateDefaultConfigContentFromSchema(ModuleSSTWDF::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePostCGNS.json", GenerateDefaultConfigContentFromSchema(ModulePostCGNS::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePostPlot3D.json", GenerateDefaultConfigContentFromSchema(ModulePostPlot3D::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePostTecplot.json", GenerateDefaultConfigContentFromSchema(ModulePostTecplot::GetParamSchema()));
    
    // Reused module instance configuration files
    WriteJsonFile(dir_path, "ModulePostPlot3D_1.json", GenerateDefaultConfigContentFromSchema(ModulePostPlot3D::GetParamSchema()));
    WriteJsonFile(dir_path, "ModulePostPlot3D_2.json", GenerateDefaultConfigContentFromSchema(ModulePostPlot3D::GetParamSchema()));
    
    // Engine configuration files
    WriteJsonFile(dir_path, "EnginePreGrid.json", GenerateDefaultConfigContentFromSchema(EnginePreGrid::GetParamSchema()));
    WriteJsonFile(dir_path, "EngineTurbulence.json", GenerateDefaultConfigContentFromSchema(EngineTurbulence::GetParamSchema()));
    WriteJsonFile(dir_path, "EngineFlowField.json", GenerateDefaultConfigContentFromSchema(EngineFlowField::GetParamSchema()));
    WriteJsonFile(dir_path, "EnginePre.json", GenerateDefaultConfigContentFromSchema(EnginePre::GetParamSchema()));
    WriteJsonFile(dir_path, "EngineSolve.json", GenerateDefaultConfigContentFromSchema(EngineSolve::GetParamSchema()));
    WriteJsonFile(dir_path, "EnginePost.json", GenerateDefaultConfigContentFromSchema(EnginePost::GetParamSchema()));
    WriteJsonFile(dir_path, "EngineMainProcess.json", GenerateDefaultConfigContentFromSchema(EngineMainProcess::GetParamSchema()));
    
    // Reused engine instance configuration files
    WriteJsonFile(dir_path, "EnginePost_1.json", GenerateDefaultConfigContentFromSchema(EnginePost::GetParamSchema()));
    WriteJsonFile(dir_path, "EnginePost_2.json", GenerateDefaultConfigContentFromSchema(EnginePost::GetParamSchema()));

    std::cout << "Generated separate default configuration files for all components: " << dir_path << std::endl;
}

void LoadConfig(const std::string& configFile, nlohmann::json& config)
{
    std::filesystem::path config_path;
    
    // Check if configFile contains path separators (is a full/relative path)
    if (configFile.find('/') != std::string::npos || configFile.find('\\') != std::string::npos) {
        // configFile contains path, use it directly
        config_path = configFile;
    } else {
        // configFile is just a filename, prepend default config directory
        std::filesystem::path config_dir = "./config";
        
        // Ensure config directory exists
        if (!std::filesystem::exists(config_dir)) {
            throw std::runtime_error("Configuration directory not found: " + config_dir.string());
        }
        
        config_path = config_dir / configFile;
    }
    
    // If filename has no extension, default to .json
    if (config_path.extension().empty()) {
        config_path += ".json";
    }
    
    if (!std::filesystem::exists(config_path)) {
        throw std::runtime_error("Configuration file not found: " + config_path.string());
    }
    
    if (std::filesystem::is_directory(config_path)) {
        throw std::runtime_error("Configuration path is a directory, but expected a file: " + config_path.string());
    }

    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Unable to open configuration file: " + config_path.string());
    }
    
    try {
        ifs >> config;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + config_path.string() + ": " + e.what());
    }

    //std::cout << "Successfully loaded configuration file: " << config_path.string() << std::endl;
}

// --- ModulePreCGNS ---
ModulePreCGNS::ModulePreCGNS(const nlohmann::json& params)
{
    ParamValidation(params);
    cgns_type_ = params.at("cgns_type").get<std::string>();
    cgns_value_ = params.at("cgns_value").get<double>();
    std::cout << "ModulePreCGNS configured successfully, type: " << cgns_type_ << ", value: " << cgns_value_ << std::endl;
}

void ModulePreCGNS::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("cgns_type")) {
        throw std::runtime_error("ModulePreCGNS: parameter 'cgns_type' is missing.");
    }
    if (!params.at("cgns_type").is_string()) {
        throw std::runtime_error("ModulePreCGNS: parameter 'cgns_type' must be a string.");
    }
    if (!params.contains("cgns_value")) {
        throw std::runtime_error("ModulePreCGNS: parameter 'cgns_value' is missing.");
    }
    if (!params.at("cgns_value").is_number()) {
        throw std::runtime_error("ModulePreCGNS: parameter 'cgns_value' must be a number.");
    }
}
void ModulePreCGNS::Initialize()
{
    std::cout << "ModulePreCGNS Initialize..." << std::endl;
}
void ModulePreCGNS::Execute()
{
    std::cout << "ModulePreCGNS Execute..." << std::endl;
}
void ModulePreCGNS::Release()
{
    std::cout << "ModulePreCGNS Release..." << std::endl;
}
nlohmann::json ModulePreCGNS::GetParamSchema()
{
    return {
        {"cgns_type", {
            {"type", "string"},
            {"description", "CGNS file type"},
            {"enum", {"HDF5", "ADF", "XML"}},
            {"default", "HDF5"}
        }},
        {"cgns_value", {
            {"type", "number"},
            {"description", "CGNS parameter value"},
            {"minimum", 1},
            {"maximum", 100},
            {"default", 15.0}
        }}
    };
}

// --- ModulePrePlot3D ---
ModulePrePlot3D::ModulePrePlot3D(const nlohmann::json& params)
{
    ParamValidation(params);
    plot3d_option_ = params.at("plot3d_option").get<std::string>();
    std::cout << "ModulePrePlot3D configured successfully, option: " << plot3d_option_ << std::endl;
}

void ModulePrePlot3D::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("plot3d_option")) {
        throw std::runtime_error("ModulePrePlot3D: parameter 'plot3d_option' is missing.");
    }
    if (!params.at("plot3d_option").is_string()) {
        throw std::runtime_error("ModulePrePlot3D: parameter 'plot3d_option' must be a string.");
    }
}
void ModulePrePlot3D::Initialize()
{
    std::cout << "ModulePrePlot3D Initialize..." << std::endl;
}
void ModulePrePlot3D::Execute()
{
    std::cout << "ModulePrePlot3D Execute..." << std::endl;
}
void ModulePrePlot3D::Release()
{
    std::cout << "ModulePrePlot3D Release..." << std::endl;
}
nlohmann::json ModulePrePlot3D::GetParamSchema()
{
    return {
        {"plot3d_option", {
            {"type", "string"},
            {"description", "Plot3D specific option"},
            {"default", "default_option"}
        }}
    };
}

// --- ModulePreTecplot ---
ModulePreTecplot::ModulePreTecplot(const nlohmann::json& params)
{
    ParamValidation(params);
    tecplot_binary_format_ = params.at("tecplot_binary_format").get<bool>();
    std::cout << "ModulePreTecplot configured successfully, binary format: " << tecplot_binary_format_ << std::endl;
}

void ModulePreTecplot::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("tecplot_binary_format")) {
        throw std::runtime_error("ModulePreTecplot: parameter 'tecplot_binary_format' is missing.");
    }
    if (!params.at("tecplot_binary_format").is_boolean()) {
        throw std::runtime_error("ModulePreTecplot: parameter 'tecplot_binary_format' must be a boolean.");
    }
}
void ModulePreTecplot::Initialize()
{
    std::cout << "ModulePreTecplot Initialize..." << std::endl;
}
void ModulePreTecplot::Execute()
{
    std::cout << "ModulePreTecplot Execute..." << std::endl;
}
void ModulePreTecplot::Release()
{
    std::cout << "ModulePreTecplot Release..." << std::endl;
}
nlohmann::json ModulePreTecplot::GetParamSchema()
{
    return {
        {"tecplot_binary_format", {
            {"type", "boolean"},
            {"description", "Use Tecplot binary format"},
            {"default", true}
        }}
    };
}

// --- ModuleSA ---
ModuleSA::ModuleSA(const nlohmann::json& params)
{
    ParamValidation(params);
    sa_constant_ = params.value("sa_constant", kSaMaxValue_);
    std::cout << "ModuleSA configured successfully, SA constant: " << sa_constant_ << std::endl;
}

void ModuleSA::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("sa_constant") && !params.at("sa_constant").is_number()) {
        throw std::runtime_error("ModuleSA: parameter 'sa_constant' must be a number.");
    }
}
void ModuleSA::Initialize()
{
    std::cout << "ModuleSA Initialize..." << std::endl;
}
void ModuleSA::Execute()
{
    std::cout << "ModuleSA Execute..." << std::endl;
}
void ModuleSA::Release()
{
    std::cout << "ModuleSA Release..." << std::endl;
}
nlohmann::json ModuleSA::GetParamSchema()
{
    return {
        {"sa_constant", {
            {"type", "number"}, {"description", "SA model constant"}, {"default", 0.41} // kSaMaxValue_
        }}
    };
}

// --- ModuleSST ---
ModuleSST::ModuleSST(const nlohmann::json& params)
{
    ParamValidation(params);
    sst_iterations_ = params.value("sst_iterations", 100);
    std::cout << "ModuleSST configured successfully, SST iterations: " << sst_iterations_ << std::endl;
}

void ModuleSST::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("sst_iterations") && !params.at("sst_iterations").is_number_integer()) {
        throw std::runtime_error("ModuleSST: parameter 'sst_iterations' must be an integer.");
    }
}
void ModuleSST::Initialize()
{
    std::cout << "ModuleSST Initialize..." << std::endl;
}
void ModuleSST::Execute()
{
    std::cout << "ModuleSST Execute..." << std::endl;
}
void ModuleSST::Release()
{
    std::cout << "ModuleSST Release..." << std::endl;
}
nlohmann::json ModuleSST::GetParamSchema()
{
    return {
        {"sst_iterations", {
            {"type", "integer"}, {"description", "SST model iterations"}, {"default", 100}
        }}
    };
}

// --- ModuleSSTWDF ---
ModuleSSTWDF::ModuleSSTWDF(const nlohmann::json& params)
{
    ParamValidation(params);
    wdf_model_name_ = params.value("wdf_model_name", "StandardWDF");
    std::cout << "ModuleSSTWDF configured successfully, WDF model name: " << wdf_model_name_ << std::endl;
}

void ModuleSSTWDF::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("wdf_model_name") && !params.at("wdf_model_name").is_string()) {
        throw std::runtime_error("ModuleSSTWDF: parameter 'wdf_model_name' must be a string.");
    }
}
void ModuleSSTWDF::Initialize()
{
    std::cout << "ModuleSSTWDF Initialize..." << std::endl;
}
void ModuleSSTWDF::Execute()
{
    std::cout << "ModuleSSTWDF Execute..." << std::endl;
}
void ModuleSSTWDF::Release()
{
    std::cout << "ModuleSSTWDF Release..." << std::endl;
}
nlohmann::json ModuleSSTWDF::GetParamSchema()
{
    return {
        {"wdf_model_name", {
            {"type", "string"}, {"description", "Wall Damping Function model name"}, {"default", "StandardWDF"}
        }}
    };
}

// --- ModulePostCGNS ---
ModulePostCGNS::ModulePostCGNS(const nlohmann::json& params)
{
    ParamValidation(params);
    output_cgns_name_ = params.value("output_cgns_name", "results.cgns");
    std::cout << "ModulePostCGNS configured successfully, output CGNS name: " << output_cgns_name_ << std::endl;
}

void ModulePostCGNS::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("output_cgns_name") && !params.at("output_cgns_name").is_string()) {
        throw std::runtime_error("ModulePostCGNS: parameter 'output_cgns_name' must be a string.");
    }
}
void ModulePostCGNS::Initialize()
{
    std::cout << "ModulePostCGNS Initialize..." << std::endl;
}
void ModulePostCGNS::Execute()
{
    std::cout << "ModulePostCGNS Execute..." << std::endl;
}
void ModulePostCGNS::Release()
{
    std::cout << "ModulePostCGNS Release..." << std::endl;
}
nlohmann::json ModulePostCGNS::GetParamSchema()
{
    return {
        {"output_cgns_name", {
            {"type", "string"}, {"description", "Output CGNS filename"}, {"default", "results.cgns"}
        }}
    };
}

// --- ModulePostPlot3D ---
ModulePostPlot3D::ModulePostPlot3D(const nlohmann::json& params)
{
    ParamValidation(params);
    write_q_file_ = params.value("write_q_file", true);
    std::cout << "ModulePostPlot3D configured successfully, write Q file: " << write_q_file_ << std::endl;
}

void ModulePostPlot3D::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("write_q_file") && !params.at("write_q_file").is_boolean()) {
        throw std::runtime_error("ModulePostPlot3D: parameter 'write_q_file' must be a boolean.");
    }
}
void ModulePostPlot3D::Initialize()
{
    std::cout << "ModulePostPlot3D Initialize..." << std::endl;
    if (write_q_file_) {
        std::cout << "ModulePostPlot3D will write Q file." << std::endl;
    } else {
        std::cout << "ModulePostPlot3D will not write Q file." << std::endl;
    }
}
void ModulePostPlot3D::Execute()
{
    std::cout << "ModulePostPlot3D Execute..." << std::endl;
}
void ModulePostPlot3D::Release()
{
    std::cout << "ModulePostPlot3D Release..." << std::endl;
}
nlohmann::json ModulePostPlot3D::GetParamSchema()
{
    return {
        {"write_q_file", {
            {"type", "boolean"}, {"description", "Write Q-file for Plot3D"}, {"default", true}
        }}
    };
}

// --- ModulePostTecplot ---
ModulePostTecplot::ModulePostTecplot(const nlohmann::json& params)
{
    ParamValidation(params);
    tecplot_zone_title_ = params.value("tecplot_zone_title", "DefaultZone");
    std::cout << "ModulePostTecplot configured successfully, Tecplot zone title: " << tecplot_zone_title_ << std::endl;
}

void ModulePostTecplot::ParamValidation(const nlohmann::json& params)
{
    if (params.contains("tecplot_zone_title") && !params.at("tecplot_zone_title").is_string()) {
        throw std::runtime_error("ModulePostTecplot: parameter 'tecplot_zone_title' must be a string.");
    }
}
void ModulePostTecplot::Initialize()
{
    std::cout << "ModulePostTecplot Initialize..." << std::endl;
}
void ModulePostTecplot::Execute()
{
    std::cout << "ModulePostTecplot Execute..." << std::endl;
}
void ModulePostTecplot::Release()
{
    std::cout << "ModulePostTecplot Release..." << std::endl;
}
nlohmann::json ModulePostTecplot::GetParamSchema()
{
    return {
        {"tecplot_zone_title", {
            {"type", "string"}, {"description", "Tecplot zone title"}, {"default", "DefaultZone"}
        }}
    };
}

// --- EnginePre ---
EnginePre::EnginePre(const nlohmann::json& instance_params) 
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    ConstructSubEngines<EnginePreVariant>(execution_order_, subEnginesPool);
}

void EnginePre::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EnginePre: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EnginePre: parameter 'execution_order' must be an array.");
    }
}
void EnginePre::Initialize() 
{
    InitializeSubEngines<EnginePreVariant>(execution_order_, subEnginesPool);
}
void EnginePre::Execute() 
{
    ExecuteSubEngines<EnginePreVariant>(execution_order_, subEnginesPool);
}
void EnginePre::Release() 
{
    ReleaseSubEngines<EnginePreVariant>(execution_order_, subEnginesPool);
}
nlohmann::json EnginePre::GetParamSchema() 
{
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
EngineSolve::EngineSolve(const nlohmann::json& instance_params)
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    ConstructSubEngines<EngineSolveVariant>(execution_order_, subEnginesPool);
}

void EngineSolve::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EngineSolve: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EngineSolve: parameter 'execution_order' must be an array.");
    }
}
void EngineSolve::Initialize()
{ 
    InitializeSubEngines<EngineSolveVariant>(execution_order_, subEnginesPool);
}
void EngineSolve::Execute()
{ 
    ExecuteSubEngines<EngineSolveVariant>(execution_order_, subEnginesPool);
}
void EngineSolve::Release()
{
    ReleaseSubEngines<EngineSolveVariant>(execution_order_, subEnginesPool);
}
nlohmann::json EngineSolve::GetParamSchema()
{
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
EnginePost::EnginePost(const nlohmann::json& instance_params)
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    ConstructSubEngines<EnginePostVariant>(execution_order_, subEnginesPool);
}

void EnginePost::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EnginePost: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EnginePost: parameter 'execution_order' must be an array.");
    }
}
void EnginePost::Initialize()
{
    InitializeSubEngines<EnginePostVariant>(execution_order_, subEnginesPool);
}
void EnginePost::Execute()
{
    ExecuteSubEngines<EnginePostVariant>(execution_order_, subEnginesPool);
}
void EnginePost::Release()
{
    ReleaseSubEngines<EnginePostVariant>(execution_order_, subEnginesPool);
}
nlohmann::json EnginePost::GetParamSchema()
{
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

// Add static registration function
void EngineMainProcess::RegisterAllEngines()
{
    static bool registered = false;
    if (!registered) {
        // Register main process engines
        MainProcessFactory::Instance().RegisterCreator("EnginePre", 
            [](const nlohmann::json& config) -> EngineMainProcessVariant {
                return std::make_unique<EnginePre>(config);
            });
        MainProcessFactory::Instance().RegisterCreator("EngineSolve", 
            [](const nlohmann::json& config) -> EngineMainProcessVariant {
                return std::make_unique<EngineSolve>(config);
            });
        MainProcessFactory::Instance().RegisterCreator("EnginePost", 
            [](const nlohmann::json& config) -> EngineMainProcessVariant {
                return std::make_unique<EnginePost>(config);
            });
        
        // Register sub-engines
        PreFactory::Instance().RegisterCreator("EnginePreGrid", 
            [](const nlohmann::json& config) -> EnginePreVariant {
                return std::make_unique<EnginePreGrid>(config);
            });
        SolveFactory::Instance().RegisterCreator("EngineTurbulence", 
            [](const nlohmann::json& config) -> EngineSolveVariant {
                return std::make_unique<EngineTurbulence>(config);
            });
        PostFactory::Instance().RegisterCreator("EngineFlowField", 
            [](const nlohmann::json& config) -> EnginePostVariant {
                return std::make_unique<EngineFlowField>(config);
            });
        
        registered = true;
    }
}

// EngineMainProcess static constant member definition and initialization
EngineMainProcess::EngineMainProcess(const nlohmann::json& config)
{
    // Ensure all engines are registered before using factories
    RegisterAllEngines();
    
    ParamValidation(config);
    ConstructSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::ParamValidation(const nlohmann::json& params)
{
    // EngineMainProcess uses default execution_order, no validation needed for now
    // Can add validation for optional parameters if needed in the future
}

void EngineMainProcess::Initialize()
{
    InitializeSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::Execute()
{
    ExecuteSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

void EngineMainProcess::Release()
{
    ReleaseSubEngines<EngineMainProcessVariant>(execution_order_, subEnginesPool);
}

nlohmann::json EngineMainProcess::GetParamSchema()
{
    return {
        {"description", "Main Process Engine: coordinates the overall workflow"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential"}}, {"default", "sequential"}}},
        {"execution_order", {
            {"type", "array"},
            {"description", "Ordered list of top-level engines to execute"},
            {"items", {
                {"type", "string"},
                {"enum", {"EnginePre", "EngineSolve", "EnginePost"}}
            }},
            {"default", {"EnginePre", "EngineSolve", "EnginePost"}}
        }},
        {"sub_engine_parameters_schemas", {
            {"EnginePre", EnginePre::GetParamSchema()},
            {"EngineSolve", EngineSolve::GetParamSchema()},
            {"EnginePost", EnginePost::GetParamSchema()}
        }}
    };
}

// --- EnginePreGrid ---
EnginePreGrid::EnginePreGrid(const nlohmann::json& instance_params)
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    
    // Construct modules directly without factory
    for (const auto& moduleName : execution_order_)
    {
        nlohmann::json config;
        std::string configFileName = moduleName + ".json";
        std::string baseModuleName = GetBaseName(moduleName);
        LoadConfig(configFileName, config);
        
        subModulesPool[moduleName] = CreateModule(baseModuleName, config);
    }
}

void EnginePreGrid::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EnginePreGrid: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EnginePreGrid: parameter 'execution_order' must be an array.");
    }
}

void EnginePreGrid::Initialize()
{
    InitializeSubEngines<EnginePreGridVariant>(execution_order_, subModulesPool);
}

void EnginePreGrid::Execute()
{
    ExecuteSubEngines<EnginePreGridVariant>(execution_order_, subModulesPool);
}

void EnginePreGrid::Release()
{
    ReleaseSubEngines<EnginePreGridVariant>(execution_order_, subModulesPool);
}

nlohmann::json EnginePreGrid::GetParamSchema()
{
    return {
        {"description", "PreGrid Engine: executes a list of preprocessing modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "chooseOne"}}, {"default", "chooseOne"}}},
        {"execution_order", { 
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

EnginePreGridVariant EnginePreGrid::CreateModule(const std::string& module_name, const nlohmann::json& config)
{
    if (module_name == "ModulePreCGNS") {
        return std::make_unique<ModulePreCGNS>(config);
    } else if (module_name == "ModulePrePlot3D") {
        return std::make_unique<ModulePrePlot3D>(config);
    } else if (module_name == "ModulePreTecplot") {
        return std::make_unique<ModulePreTecplot>(config);
    } else {
        throw std::runtime_error("Unknown module name: " + module_name);
    }
}

// --- EngineTurbulence ---
EngineTurbulence::EngineTurbulence(const nlohmann::json& instance_params)
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    
    // Construct modules directly without factory
    for (const auto& moduleName : execution_order_)
    {
        nlohmann::json config;
        std::string configFileName = moduleName + ".json";
        std::string baseModuleName = GetBaseName(moduleName);
        LoadConfig(configFileName, config);
        
        subModulesPool[moduleName] = CreateModule(baseModuleName, config);
    }
}

void EngineTurbulence::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EngineTurbulence: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EngineTurbulence: parameter 'execution_order' must be an array.");
    }
}

void EngineTurbulence::Initialize()
{
    InitializeSubEngines<EngineTurbulenceVariant>(execution_order_, subModulesPool);
}

void EngineTurbulence::Execute()
{
    ExecuteSubEngines<EngineTurbulenceVariant>(execution_order_, subModulesPool);
}

void EngineTurbulence::Release()
{ 
    ReleaseSubEngines<EngineTurbulenceVariant>(execution_order_, subModulesPool);
}

nlohmann::json EngineTurbulence::GetParamSchema()
{
     return {
        {"description", "Turbulence Engine: executes a list of turbulence modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "chooseOne"}}, {"default", "chooseOne"}}},
        {"execution_order", {
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

EngineTurbulenceVariant EngineTurbulence::CreateModule(const std::string& module_name, const nlohmann::json& config)
{
    if (module_name == "ModuleSA") {
        return std::make_unique<ModuleSA>(config);
    } else if (module_name == "ModuleSST") {
        return std::make_unique<ModuleSST>(config);
    } else if (module_name == "ModuleSSTWDF") {
        return std::make_unique<ModuleSSTWDF>(config);
    } else {
        throw std::runtime_error("Unknown module name: " + module_name);
    }
}

// --- EngineFlowField ---
EngineFlowField::EngineFlowField(const nlohmann::json& instance_params) 
{
    ParamValidation(instance_params);
    execution_order_ = instance_params["execution_order"].get<std::vector<std::string>>();
    
    // Construct modules directly without factory
    for (const auto& moduleName : execution_order_)
    {
        nlohmann::json config;
        std::string configFileName = moduleName + ".json";
        std::string baseModuleName = GetBaseName(moduleName);
        LoadConfig(configFileName, config);
        
        subModulesPool[moduleName] = CreateModule(baseModuleName, config);
    }
}

void EngineFlowField::ParamValidation(const nlohmann::json& params)
{
    if (!params.contains("execution_order")) {
        throw std::runtime_error("EngineFlowField: parameter 'execution_order' is missing.");
    }
    if (!params.at("execution_order").is_array()) {
        throw std::runtime_error("EngineFlowField: parameter 'execution_order' must be an array.");
    }
}

void EngineFlowField::Initialize() 
{ 
    InitializeSubEngines<EngineFlowFieldVariant>(execution_order_, subModulesPool);
}

void EngineFlowField::Execute() 
{ 
    ExecuteSubEngines<EngineFlowFieldVariant>(execution_order_, subModulesPool);
}

void EngineFlowField::Release() 
{ 
    ReleaseSubEngines<EngineFlowFieldVariant>(execution_order_, subModulesPool);
}

nlohmann::json EngineFlowField::GetParamSchema() 
{
    return {
        {"description", "FlowField Engine: executes a list of postprocessing modules in specified order"},
        {"execution_type", {{"type", "string"}, {"enum", {"sequential_modules", "sequential"}}, {"default", "sequential_modules"}}}, // Note: "sequential" was also in enum, "sequential_modules" seems more specific.
        {"execution_order", {
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

EngineFlowFieldVariant EngineFlowField::CreateModule(const std::string& module_name, const nlohmann::json& config)
{
    if (module_name == "ModulePostCGNS") {
        return std::make_unique<ModulePostCGNS>(config);
    } else if (module_name == "ModulePostPlot3D") {
        return std::make_unique<ModulePostPlot3D>(config);
    } else if (module_name == "ModulePostTecplot") {
        return std::make_unique<ModulePostTecplot>(config);
    } else {
        throw std::runtime_error("Unknown module name: " + module_name);
    }
}