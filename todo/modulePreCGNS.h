#pragma once

#include "nlohmann/json.hpp"
#include <string>

/**
 * @brief CGNS file preprocessing module.
 */
class ModulePreCGNS {
public:
    /**
     * @brief Construct ModulePreCGNS object.
     * @param params JSON object containing initialization parameters.
     * @throw std::runtime_error If configuration parameters are missing or invalid.
     */
    ModulePreCGNS(const nlohmann::json& params);
    /** @brief Initialize the module. */
    void Initialize();
    /** @brief Execute the module's main logic. */
    void Execute();
    /** @brief Release any resources held by the module. */
    void Release();
    /**
     * @brief Validate parameters for this module.
     * @param params JSON parameters to validate.
     * @throw std::runtime_error If validation fails.
     */
    void ParamValidation(const nlohmann::json& params);
    /**
     * @brief Get JSON schema for this module's parameters.
     * @return nlohmann::json object representing the schema.
     */
    static nlohmann::json GetParamSchema();
    
protected:
    std::string cgns_type_;
    double cgns_value_;
};
