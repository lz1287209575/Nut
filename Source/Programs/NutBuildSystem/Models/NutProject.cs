using System.Text.Json.Serialization;

namespace NutBuildSystem.Models;

public class NutProject
{
    [JsonPropertyName("FileVersion")]
    public int FileVersion { get; set; } = 3;

    [JsonPropertyName("EngineAssociation")]
    public string EngineAssociation { get; set; } = string.Empty;

    [JsonPropertyName("Category")]
    public string Category { get; set; } = string.Empty;

    [JsonPropertyName("Description")]
    public string Description { get; set; } = string.Empty;

    [JsonPropertyName("Company")]
    public string Company { get; set; } = string.Empty;

    [JsonPropertyName("Copyright")]
    public string Copyright { get; set; } = string.Empty;

    [JsonPropertyName("Modules")]
    public List<NutModule> Modules { get; set; } = new();

    [JsonPropertyName("Plugins")]
    public List<NutPlugin> Plugins { get; set; } = new();

    [JsonPropertyName("TargetPlatforms")]
    public List<string> TargetPlatforms { get; set; } = new();

    [JsonPropertyName("Paths")]
    public NutPaths Paths { get; set; } = new();

    [JsonPropertyName("AdditionalPluginDirectories")]
    public List<string> AdditionalPluginDirectories { get; set; } = new();

    [JsonPropertyName("AdditionalRootDirectories")]
    public List<string> AdditionalRootDirectories { get; set; } = new();
}

public class NutModule
{
    [JsonPropertyName("Name")]
    public string Name { get; set; } = string.Empty;

    [JsonPropertyName("Type")]
    public string Type { get; set; } = string.Empty;

    [JsonPropertyName("LoadingPhase")]
    public string LoadingPhase { get; set; } = string.Empty;
}

public class NutPlugin
{
    [JsonPropertyName("Name")]
    public string Name { get; set; } = string.Empty;

    [JsonPropertyName("Enabled")]
    public bool Enabled { get; set; } = true;

    [JsonPropertyName("Path")]
    public string Path { get; set; } = string.Empty;

    [JsonPropertyName("SupportedTargetPlatforms")]
    public List<string> SupportedTargetPlatforms { get; set; } = new();
}

public class NutPaths
{
    [JsonPropertyName("Build")]
    public string Build { get; set; } = string.Empty;

    [JsonPropertyName("Intermediate")]
    public string Intermediate { get; set; } = string.Empty;

    [JsonPropertyName("ProjectFiles")]
    public string ProjectFiles { get; set; } = string.Empty;
}