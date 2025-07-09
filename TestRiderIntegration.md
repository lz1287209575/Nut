# Rider 集成测试

## 🧪 测试步骤

### 1. 验证 .nutproj 文件
```bash
# 检查 .nutproj 文件是否存在
ls -la Nut.nutproj

# 验证 JSON 格式是否正确
cat Nut.nutproj | python3 -m json.tool
```

### 2. 验证 Xcode 项目文件
```bash
# 检查是否生成了 Xcode 项目文件
find Source/ -name "*.xcodeproj" -type d

# 验证 project.pbxproj 文件
find Source/ -name "project.pbxproj" -exec head -5 {} \;
```

### 3. 验证 Visual Studio 项目文件
```bash
# 检查是否生成了 .vcxproj 文件
find Source/ -name "*.vcxproj"

# 验证 .sln 文件
head -10 Nut.sln
```

### 4. Rider 集成测试

#### macOS 下测试 Rider
1. 打开 Rider
2. 选择 "Open" 或 "Open Project"
3. 选择 `Nut.nutproj` 文件
4. 验证：
   - 项目是否正确加载
   - C++ 文件是否有语法高亮
   - 智能代码补全是否工作
   - 调试配置是否正确

#### 备选方案测试
如果 Rider 无法识别 `.nutproj`，可以：
1. 打开 `Nut.sln` 文件
2. 或打开单个 `.xcodeproj` 文件

## 🔍 预期结果

### ✅ 成功标志
- Rider 能正确打开项目
- C++ 文件有语法高亮和智能补全
- 项目结构正确显示
- 编译和调试功能正常

### ❌ 问题标志
- Rider 无法识别项目文件
- 没有语法高亮
- 编译错误
- 项目结构显示不正确

## 🛠️ 故障排除

### 如果 Rider 无法识别 .nutproj
1. 尝试打开 `Nut.sln` 文件
2. 检查是否安装了 C++ 插件
3. 验证项目文件格式

### 如果编译失败
1. 检查是否安装了必要的工具链
2. 验证项目配置
3. 查看错误日志

## 📝 测试记录

请在下方记录测试结果：

- [ ] .nutproj 文件格式正确
- [ ] Xcode 项目文件生成成功
- [ ] Visual Studio 项目文件生成成功
- [ ] Rider 能正确打开 .nutproj
- [ ] C++ 语法高亮正常
- [ ] 智能代码补全工作
- [ ] 编译功能正常
- [ ] 调试功能正常

### 测试日期：_________
### 测试结果：_________
### 问题记录：_________ 