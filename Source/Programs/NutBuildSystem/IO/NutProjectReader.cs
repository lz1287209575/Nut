using System.Text.Json;
using NutBuildSystem.Logging;
using NutBuildSystem.Models;

namespace NutBuildSystem.IO;

public class NutProjectReader
{
    private readonly ILogger logger;
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        PropertyNameCaseInsensitive = true,
        ReadCommentHandling = JsonCommentHandling.Skip,
        AllowTrailingCommas = true
    };

    public NutProjectReader(ILogger logger)
    {
        this.logger = logger;
    }

    public async Task<NutProject?> ReadProjectAsync(string projectFilePath)
    {
        try
        {
            logger.Info($"Reading Nut project file: {projectFilePath}");
            
            if (!File.Exists(projectFilePath))
            {
                logger.Error($"Project file not found: {projectFilePath}");
                return null;
            }

            string jsonContent = await File.ReadAllTextAsync(projectFilePath);
            
            if (string.IsNullOrWhiteSpace(jsonContent))
            {
                logger.Error($"Project file is empty: {projectFilePath}");
                return null;
            }

            var project = JsonSerializer.Deserialize<NutProject>(jsonContent, JsonOptions);
            
            if (project == null)
            {
                logger.Error($"Failed to deserialize project file: {projectFilePath}");
                return null;
            }

            // Validate the project structure
            if (!ValidateProject(project))
            {
                logger.Error($"Project validation failed: {projectFilePath}");
                return null;
            }

            logger.Info($"Successfully loaded project: {project.EngineAssociation} (FileVersion: {project.FileVersion})");
            return project;
        }
        catch (JsonException ex)
        {
            logger.Error($"JSON parsing error in project file {projectFilePath}: {ex.Message}");
            return null;
        }
        catch (Exception ex)
        {
            logger.Error($"Unexpected error reading project file {projectFilePath}: {ex.Message}");
            return null;
        }
    }

    public static async Task<NutProject?> ReadProjectAsync(string projectFilePath, ILogger logger)
    {
        var reader = new NutProjectReader(logger);
        return await reader.ReadProjectAsync(projectFilePath);
    }

    public static NutProject? FindAndReadProject(string startDirectory, ILogger logger)
    {
        try
        {
            string? projectFile = FindProjectFile(startDirectory);
            if (projectFile == null)
            {
                logger.Warning($"No .nprx project file found starting from: {startDirectory}");
                return null;
            }

            return ReadProjectAsync(projectFile, logger).GetAwaiter().GetResult();
        }
        catch (Exception ex)
        {
            logger.Error($"Error finding and reading project file: {ex.Message}");
            return null;
        }
    }

    public static string? FindProjectFile(string startDirectory)
    {
        string currentDir = Path.GetFullPath(startDirectory);
        
        while (!string.IsNullOrEmpty(currentDir))
        {
            var nprxFiles = Directory.GetFiles(currentDir, "*.nprx");
            if (nprxFiles.Length > 0)
            {
                return nprxFiles[0]; // Return the first .nprx file found
            }

            string? parentDir = Directory.GetParent(currentDir)?.FullName;
            if (parentDir == currentDir)
                break;
            
            currentDir = parentDir ?? string.Empty;
        }

        return null;
    }

    private bool ValidateProject(NutProject project)
    {
        try
        {
            // Validate basic project info
            if (string.IsNullOrWhiteSpace(project.EngineAssociation))
            {
                logger.Error("EngineAssociation is required");
                return false;
            }

            if (project.FileVersion <= 0)
            {
                logger.Error("FileVersion must be greater than 0");
                return false;
            }

            // Validate required paths exist
            string projectRoot = Environment.CurrentDirectory;
            
            var requiredDirs = new[]
            {
                project.Paths.Build,
                project.Paths.Intermediate,
                project.Paths.ProjectFiles
            };

            foreach (string dir in requiredDirs.Where(d => !string.IsNullOrWhiteSpace(d)))
            {
                string fullPath = Path.Combine(projectRoot, dir);
                if (!Directory.Exists(fullPath))
                {
                    logger.Warning($"Directory does not exist: {fullPath}");
                }
            }

            // Validate plugin paths
            foreach (var plugin in project.Plugins.Where(p => p.Enabled))
            {
                if (!string.IsNullOrWhiteSpace(plugin.Path))
                {
                    string fullPath = Path.Combine(projectRoot, plugin.Path);
                    if (!Directory.Exists(fullPath))
                    {
                        logger.Warning($"Plugin path does not exist: {fullPath}");
                    }
                }
            }

            logger.Debug("Project validation completed successfully");
            return true;
        }
        catch (Exception ex)
        {
            logger.Error($"Project validation error: {ex.Message}");
            return false;
        }
    }

    public static List<NutModule> GetModules(NutProject project)
    {
        return project.Modules ?? new List<NutModule>();
    }

    public static List<NutModule> GetCoreLibModules(NutProject project)
    {
        return project.Modules?.Where(m => m.Type == "CoreLib").ToList() ?? new List<NutModule>();
    }

    public static List<NutModule> GetExecutableModules(NutProject project)
    {
        return project.Modules?.Where(m => m.Type == "Executable").ToList() ?? new List<NutModule>();
    }

    public static List<NutPlugin> GetEnabledPlugins(NutProject project)
    {
        return project.Plugins?.Where(p => p.Enabled).ToList() ?? new List<NutPlugin>();
    }
}