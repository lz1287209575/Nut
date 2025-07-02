using System.Text.Json;
using NBT.Core.Models;

namespace NBT.Core.Services;

/// <summary>
/// 元数据加载器
/// </summary>
public class FMetaLoader : IMetaLoader
{
    /// <summary>
    /// JSON 序列化选项
    /// </summary>
    private readonly JsonSerializerOptions JsonOptions;

    /// <summary>
    /// 构造函数
    /// </summary>
    public FMetaLoader()
    {
        JsonOptions = new JsonSerializerOptions
        {
            PropertyNameCaseInsensitive = true,
            WriteIndented = true,
            AllowTrailingCommas = true,
            ReadCommentHandling = JsonCommentHandling.Skip
        };
    }

    /// <summary>
    /// 加载元数据
    /// </summary>
    /// <typeparam name="T">元数据类型</typeparam>
    /// <param name="FilePath">文件路径</param>
    /// <returns>元数据对象</returns>
    public async Task<T?> LoadMetaAsync<T>(string FilePath) where T : FMeta
    {
        try
        {
            if (!File.Exists(FilePath))
            {
                throw new FileNotFoundException($"元数据文件不存在: {FilePath}");
            }

            var JsonContent = await File.ReadAllTextAsync(FilePath);
            var Meta = JsonSerializer.Deserialize<T>(JsonContent, JsonOptions);

            if (Meta != null)
            {
                Meta.MetaFilePath = FilePath;
                Meta.ProjectRoot = Path.GetDirectoryName(FilePath) ?? string.Empty;
            }

            return Meta;
        }
        catch (Exception Ex)
        {
            throw new InvalidOperationException($"加载元数据失败: {Ex.Message}", Ex);
        }
    }

    /// <summary>
    /// 保存元数据
    /// </summary>
    /// <typeparam name="T">元数据类型</typeparam>
    /// <param name="Meta">元数据对象</param>
    /// <param name="FilePath">文件路径</param>
    public async Task SaveMetaAsync<T>(T Meta, string FilePath) where T : FMeta
    {
        try
        {
            Meta.Update();
            var JsonContent = JsonSerializer.Serialize(Meta, JsonOptions);
            
            var DirectoryName = Path.GetDirectoryName(FilePath);
            if (!string.IsNullOrEmpty(DirectoryName) && !Directory.Exists(DirectoryName))
            {
                Directory.CreateDirectory(DirectoryName);
            }

            await File.WriteAllTextAsync(FilePath, JsonContent);
        }
        catch (Exception Ex)
        {
            throw new InvalidOperationException($"保存元数据失败: {Ex.Message}", Ex);
        }
    }

    /// <summary>
    /// 验证元数据文件
    /// </summary>
    /// <param name="FilePath">文件路径</param>
    /// <returns>是否有效</returns>
    public bool ValidateMetaFile(string FilePath)
    {
        try
        {
            if (!File.Exists(FilePath))
            {
                return false;
            }

            var JsonContent = File.ReadAllText(FilePath);
            var Document = JsonDocument.Parse(JsonContent);
            
            // 检查必需字段
            return Document.RootElement.TryGetProperty("name", out _) &&
                   Document.RootElement.TryGetProperty("type", out _);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// 获取元数据文件列表
    /// </summary>
    /// <param name="Directory">目录路径</param>
    /// <param name="Pattern">文件模式</param>
    /// <returns>文件路径列表</returns>
    public IEnumerable<string> GetMetaFiles(string Directory, string Pattern = "*.meta.json")
    {
        try
        {
            return !System.IO.Directory.Exists(Directory) ? [] : System.IO.Directory.GetFiles(Directory, Pattern, SearchOption.AllDirectories);
        }
        catch
        {
            return [];
        }
    }

    public Task<List<FBuildMeta>> LoadAllMetasAsync()
    {
        throw new NotImplementedException();
    }

    public Task<FBuildMeta?> LoadMetaAsync(string InModuleName)
    {
        throw new NotImplementedException();
    }

    public Task<List<string>> ScanModulesAsync()
    {
        throw new NotImplementedException();
    }
} 