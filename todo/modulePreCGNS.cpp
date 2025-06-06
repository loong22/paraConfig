#include "modulePreCGNS.h"
#include <iostream>
#include <stdexcept>

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
