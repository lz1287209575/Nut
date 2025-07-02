using Microsoft.Extensions.Logging;
using Moq;
using NBT.Core.Models;
using NBT.Core.Services;
using Xunit;

namespace NBT.Tests;

public class FBuildEngineTests
{
    private readonly Mock<ILogger<FBuildEngine>> LoggerMock;
    private readonly Mock<IMetaLoader> MetaLoaderMock;
    private readonly Mock<ICompiler> CompilerMock;
    private readonly FBuildEngine BuildEngine;

    public FBuildEngineTests()
    {
        LoggerMock = new Mock<ILogger<FBuildEngine>>();
        MetaLoaderMock = new Mock<IMetaLoader>();
        CompilerMock = new Mock<ICompiler>();
        
        BuildEngine = new FBuildEngine(LoggerMock.Object, MetaLoaderMock.Object, CompilerMock.Object);
    }

    [Fact]
    public async Task BuildAllAsync_ShouldReturnTrue_WhenAllModulesBuildSuccessfully()
    {
        // Arrange
        var Modules = new List<FBuildMeta>
        {
            new FServiceBuildMeta { Name = "Module1" },
            new FServiceBuildMeta { Name = "Module2" }
        };

        MetaLoaderMock.Setup(x => x.LoadAllMetasAsync())
            .ReturnsAsync(Modules);
        CompilerMock.Setup(x => x.BuildModuleAsync(It.IsAny<FBuildMeta>()))
            .ReturnsAsync(true);

        // Act
        var Result = await BuildEngine.BuildAllAsync();

        // Assert
        Assert.True(Result);
        CompilerMock.Verify(x => x.BuildModuleAsync(It.IsAny<FBuildMeta>()), Times.Exactly(2));
    }

    [Fact]
    public async Task BuildModuleAsync_ShouldReturnTrue_WhenModuleBuildsSuccessfully()
    {
        // Arrange
        var Module = new FServiceBuildMeta { Name = "TestModule" };
        MetaLoaderMock.Setup(x => x.LoadMetaAsync("TestModule"))
            .ReturnsAsync(Module);
        CompilerMock.Setup(x => x.BuildModuleAsync(Module))
            .ReturnsAsync(true);

        // Act
        var Result = await BuildEngine.BuildModuleAsync("TestModule");

        // Assert
        Assert.True(Result);
        CompilerMock.Verify(x => x.BuildModuleAsync(Module), Times.Once);
    }

    [Fact]
    public async Task BuildModuleAsync_ShouldReturnFalse_WhenModuleNotFound()
    {
        // Arrange
        MetaLoaderMock.Setup(x => x.LoadMetaAsync("NonExistentModule"))
            .ReturnsAsync((FBuildMeta?)null);

        // Act
        var Result = await BuildEngine.BuildModuleAsync("NonExistentModule");

        // Assert
        Assert.False(Result);
        CompilerMock.Verify(x => x.BuildModuleAsync(It.IsAny<FBuildMeta>()), Times.Never);
    }

    [Fact]
    public async Task ListModulesAsync_ShouldReturnAllModules()
    {
        // Arrange
        var ExpectedModules = new List<FBuildMeta>
        {
            new FServiceBuildMeta { Name = "Module1" },
            new FServiceBuildMeta { Name = "Module2" }
        };

        MetaLoaderMock.Setup(x => x.LoadAllMetasAsync())
            .ReturnsAsync(ExpectedModules);

        // Act
        var Result = await BuildEngine.ListModulesAsync();

        // Assert
        Assert.Equal(ExpectedModules.Count, Result.Count);
        Assert.Equal(ExpectedModules[0].Name, Result[0].Name);
        Assert.Equal(ExpectedModules[1].Name, Result[1].Name);
    }
} 