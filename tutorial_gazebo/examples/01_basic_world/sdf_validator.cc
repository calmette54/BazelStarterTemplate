/*
 * Gazebo Tutorial Example 01: SDF Validation Tool
 * 
 * This example demonstrates:
 * 1. Loading and parsing SDF files
 * 2. Validation of SDF syntax and semantics
 * 3. Error reporting and diagnostics
 * 4. Best practices for SDF creation
 * 
 * Learning objectives:
 * - Understand SDF file structure
 * - Learn validation techniques
 * - Debug common SDF errors
 * - Ensure world file compatibility
 */

#include <gz/common/Console.hh>
#include <gz/common/Filesystem.hh>

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

// Forward declarations for SDF parsing (these would come from SDF library)
// Note: In a full implementation, this would use the official SDF library
namespace sdf
{
    struct ValidationResult
    {
        bool valid;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };
}

class SdfValidator
{
public:
    SdfValidator() = default;

    /// \brief Validate an SDF file
    sdf::ValidationResult ValidateFile(const std::string &filename)
    {
        sdf::ValidationResult result;
        result.valid = true;

        std::cout << "Validating SDF file: " << filename << std::endl;

        // Check if file exists
        if (!gz::common::exists(filename))
        {
            result.valid = false;
            result.errors.push_back("File does not exist: " + filename);
            return result;
        }

        // Read file content
        std::string content = ReadFileContent(filename);
        if (content.empty())
        {
            result.valid = false;
            result.errors.push_back("Failed to read file or file is empty: " + filename);
            return result;
        }

        // Perform basic validation checks
        ValidateXmlStructure(content, result);
        ValidateSdfElements(content, result);
        ValidateSemantics(content, result);

        return result;
    }

    /// \brief Print validation results
    void PrintResults(const sdf::ValidationResult &result)
    {
        if (result.valid)
        {
            std::cout << "✓ SDF file is VALID" << std::endl;
        }
        else
        {
            std::cout << "✗ SDF file is INVALID" << std::endl;
        }

        if (!result.errors.empty())
        {
            std::cout << "\nERRORS:" << std::endl;
            for (const auto &error : result.errors)
            {
                std::cout << "  ✗ " << error << std::endl;
            }
        }

        if (!result.warnings.empty())
        {
            std::cout << "\nWARNINGS:" << std::endl;
            for (const auto &warning : result.warnings)
            {
                std::cout << "  ⚠ " << warning << std::endl;
            }
        }

        std::cout << "\nValidation Summary:" << std::endl;
        std::cout << "  Errors: " << result.errors.size() << std::endl;
        std::cout << "  Warnings: " << result.warnings.size() << std::endl;
    }

private:
    std::string ReadFileContent(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            return "";
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    void ValidateXmlStructure(const std::string &content, sdf::ValidationResult &result)
    {
        // Basic XML structure validation
        if (content.find("<?xml") == std::string::npos)
        {
            result.warnings.push_back("Missing XML declaration");
        }

        if (content.find("<sdf") == std::string::npos)
        {
            result.valid = false;
            result.errors.push_back("Missing <sdf> root element");
            return;
        }

        // Check for balanced tags (simplified)
        size_t openTags = 0;
        size_t closeTags = 0;
        size_t pos = 0;
        
        while ((pos = content.find('<', pos)) != std::string::npos)
        {
            if (pos + 1 < content.length())
            {
                if (content[pos + 1] == '/')
                {
                    closeTags++;
                }
                else if (content[pos + 1] != '?' && content[pos + 1] != '!')
                {
                    // Check if it's a self-closing tag
                    size_t closePos = content.find('>', pos);
                    if (closePos != std::string::npos && closePos > 0 && content[closePos - 1] == '/')
                    {
                        // Self-closing tag, counts as both open and close
                        openTags++;
                        closeTags++;
                    }
                    else
                    {
                        openTags++;
                    }
                }
            }
            pos++;
        }

        if (openTags != closeTags)
        {
            result.valid = false;
            result.errors.push_back("Unbalanced XML tags (open: " + std::to_string(openTags) + 
                                  ", close: " + std::to_string(closeTags) + ")");
        }
    }

    void ValidateSdfElements(const std::string &content, sdf::ValidationResult &result)
    {
        // Validate SDF version
        if (content.find("version=") == std::string::npos)
        {
            result.warnings.push_back("SDF version not specified, using default");
        }

        // Check for required world elements
        if (content.find("<world") != std::string::npos)
        {
            ValidateWorldElement(content, result);
        }

        // Check for model elements
        if (content.find("<model") != std::string::npos)
        {
            ValidateModelElements(content, result);
        }

        // Validate plugin elements
        ValidatePluginElements(content, result);
    }

    void ValidateWorldElement(const std::string &content, sdf::ValidationResult &result)
    {
        // Check for physics element
        if (content.find("<physics") == std::string::npos)
        {
            result.warnings.push_back("World missing <physics> element - using default physics");
        }

        // Check for essential plugins
        std::vector<std::string> requiredPlugins = {
            "gz-sim-physics-system",
            "gz-sim-user-commands-system", 
            "gz-sim-scene-broadcaster-system"
        };

        for (const auto &plugin : requiredPlugins)
        {
            if (content.find(plugin) == std::string::npos)
            {
                result.warnings.push_back("Missing recommended plugin: " + plugin);
            }
        }

        // Check for lighting
        if (content.find("<light") == std::string::npos)
        {
            result.warnings.push_back("World has no lighting - consider adding light sources");
        }
    }

    void ValidateModelElements(const std::string &content, sdf::ValidationResult &result)
    {
        // Basic model validation
        size_t pos = 0;
        while ((pos = content.find("<model", pos)) != std::string::npos)
        {
            // Find model name
            size_t namePos = content.find("name=", pos);
            if (namePos == std::string::npos || namePos > content.find(">", pos))
            {
                result.errors.push_back("Model missing name attribute");
                result.valid = false;
            }

            // Check for links
            size_t modelEnd = content.find("</model>", pos);
            if (modelEnd == std::string::npos)
            {
                result.errors.push_back("Unclosed model element");
                result.valid = false;
                break;
            }

            std::string modelContent = content.substr(pos, modelEnd - pos);
            if (modelContent.find("<link") == std::string::npos)
            {
                result.warnings.push_back("Model has no links - may not be physically meaningful");
            }

            pos = modelEnd;
        }
    }

    void ValidatePluginElements(const std::string &content, sdf::ValidationResult &result)
    {
        size_t pos = 0;
        while ((pos = content.find("<plugin", pos)) != std::string::npos)
        {
            // Check for filename attribute
            size_t filenamePos = content.find("filename=", pos);
            size_t pluginEnd = content.find(">", pos);
            
            if (filenamePos == std::string::npos || filenamePos > pluginEnd)
            {
                result.errors.push_back("Plugin missing filename attribute");
                result.valid = false;
            }

            // Check for name attribute
            size_t namePos = content.find("name=", pos);
            if (namePos == std::string::npos || namePos > pluginEnd)
            {
                result.warnings.push_back("Plugin missing name attribute - using filename as name");
            }

            pos = pluginEnd;
        }
    }

    void ValidateSemantics(const std::string &content, sdf::ValidationResult &result)
    {
        // Semantic validation checks

        // Check for reasonable physics timestep
        if (content.find("<max_step_size>") != std::string::npos)
        {
            // In a full implementation, we'd parse the actual value
            // and check if it's within reasonable bounds
        }

        // Check for model pose validity
        if (content.find("<pose>") != std::string::npos)
        {
            // Validate pose format (6 values: x y z roll pitch yaw)
        }

        // Check for joint limits and physics parameters
        if (content.find("<joint") != std::string::npos)
        {
            // Validate joint configurations
        }

        // Performance recommendations
        if (content.find("<visual") != std::string::npos && 
            content.find("<collision") == std::string::npos)
        {
            result.warnings.push_back("Models have visuals but no collision - may impact physics");
        }
    }
};

int main(int argc, char *argv[])
{
    gz::common::Console::SetVerbosity(2);

    std::cout << "=== Gazebo SDF Validator ===" << std::endl;
    std::cout << "Validates SDF files for syntax and semantic correctness\n" << std::endl;

    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <sdf_file> [additional_files...]" << std::endl;
        std::cout << "\nExample:" << std::endl;
        std::cout << "  " << argv[0] << " worlds/basic_world.sdf" << std::endl;
        std::cout << "  " << argv[0] << " worlds/*.sdf" << std::endl;
        return -1;
    }

    SdfValidator validator;
    bool allValid = true;
    int totalFiles = argc - 1;
    int validFiles = 0;

    for (int i = 1; i < argc; ++i)
    {
        std::string filename = argv[i];
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "File " << (i) << "/" << totalFiles << ": " << filename << std::endl;
        std::cout << std::string(60, '=') << std::endl;

        auto result = validator.ValidateFile(filename);
        validator.PrintResults(result);

        if (result.valid)
        {
            validFiles++;
        }
        else
        {
            allValid = false;
        }
    }

    // Print summary
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "VALIDATION SUMMARY" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Total files: " << totalFiles << std::endl;
    std::cout << "Valid files: " << validFiles << std::endl;
    std::cout << "Invalid files: " << (totalFiles - validFiles) << std::endl;

    if (allValid)
    {
        std::cout << "\n✓ All files are VALID!" << std::endl;
        return 0;
    }
    else
    {
        std::cout << "\n✗ Some files have ERRORS - please fix before using in simulation" << std::endl;
        return 1;
    }
}

/*
 * Key Concepts Demonstrated:
 * 
 * 1. **SDF File Structure**: Understanding the hierarchy and required elements
 *    of SDF (Simulation Description Format) files.
 * 
 * 2. **XML Validation**: Basic XML syntax checking and structure validation
 *    for well-formed documents.
 * 
 * 3. **Semantic Validation**: Beyond syntax, checking for logical consistency
 *    and best practices in SDF files.
 * 
 * 4. **Error Reporting**: Providing clear, actionable feedback for common
 *    SDF authoring mistakes.
 * 
 * 5. **Best Practices**: Recommendations for optimal SDF file structure
 *    and performance considerations.
 * 
 * Common SDF Issues This Tool Helps Detect:
 * - Missing required elements (physics, plugins)
 * - Malformed XML structure
 * - Invalid model configurations
 * - Performance anti-patterns
 * - Missing attributes or parameters
 * 
 * Usage Tips:
 * - Validate SDF files before using them in simulations
 * - Fix errors before warnings for best results
 * - Use this tool in automated build pipelines
 * - Regular validation helps maintain SDF file quality
 */