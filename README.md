# AI Blueprint Assistant

一个用于 **Unreal Engine 5.0 – 5.6+** 的编辑器插件，通过自然语言描述自动生成蓝图节点，并直接注入到当前打开的蓝图图表中。

---

## 功能概览

- **自然语言生成节点**：描述你想要的逻辑，点击按钮，AI 生成对应的蓝图节点并自动排列到图表中
- **自动编译与自我修复**：节点导入后自动编译蓝图，如发现报错则将错误信息反馈给 AI，自动撤销、修正、重新导入，直到编译通过（最多 N 次，可配置）
- **T3D 预览确认**：生成后弹出预览窗口展示原始 T3D 代码，用户确认后再导入，避免误操作
- **多轮对话支持**：可开启会话历史，基于已生成的节点继续追加需求，AI 保留上下文连续修改
- **上下文感知**：自动读取当前蓝图的变量、组件树、自定义函数、接口实现和父类信息，确保 AI 生成的节点引用正确的名称
- **目标版本可切换**：在 Project Settings 中指定目标 UE 版本（如 5.3），AI 将生成对应版本的 API 调用，即使编辑器本身是 5.6
- **持久化配置**：API Key、模型名称、Temperature 等参数保存在 Project Settings，重启编辑器无需重填
- **原生 T3D 注入**：通过 `FEdGraphUtilities::ImportNodesFromText` 导入节点，完全兼容编辑器撤销/重做（Ctrl+Z）
- **异步无阻塞**：HTTP 请求在后台线程执行，编辑器界面不卡顿
- **兼容 OpenAI 格式的 API**：支持 OpenAI、Azure OpenAI、本地 Ollama 等任意兼容接口

---

## 面板预览

```
┌─────────────────────────────────────────────────────┐
│  AI Blueprint Assistant                       × □   │
├─────────────────────────────────────────────────────┤
│  Model: gpt-4o  |  Target UE: 5.6    [New Conversation] │
│                                                     │
│  Configure API URL, key, model, and prompt in:      │
│  Edit → Project Settings → Plugins →                │
│  AI Blueprint Assistant                             │
│                                                     │
│  描述你想生成的蓝图节点：                           │
│  ┌─────────────────────────────────────────────┐    │
│  │ 当 BeginPlay 触发时，将 Health 变量设为 100  │    │
│  │ 并用 PrintString 显示到屏幕                  │    │
│  └─────────────────────────────────────────────┘    │
│                                                     │
│          [ Generate Blueprint Nodes ]               │
│                                                     │
│  状态：Ready                                        │
│  ┌─────────────────────────────────────────────┐    │
│  │ [Info] Fetching Blueprint context...        │    │
│  │ [Info] Sending request to AI...             │    │
│  │ [Info] AI response received.                │    │
│  │ [Info] Importing nodes into Blueprint...    │    │
│  │ [Warning] Compile errors (attempt 1/3):     │    │
│  │   [Set Health] Variable not found           │    │
│  │ [Info] Undoing, requesting self-repair...   │    │
│  │ [Info] Re-importing corrected nodes (2/3)   │    │
│  │ [Success] Nodes generated and compiled.     │    │
│  └─────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────┘
```

---

## 环境要求

| 项目 | 要求 |
|---|---|
| Unreal Engine | 5.0 – 5.6+（含） |
| 操作系统 | Windows / macOS / Linux |
| AI 接口 | 任意 OpenAI Chat Completions 兼容接口 |
| 编译器 | Visual Studio 2022 / Xcode 15+ / clang |

---

## 安装步骤

### 1. 复制插件到项目

将 `AIBlueprintAssistant/` 目录复制到你的 UE 项目的 `Plugins/` 文件夹中：

```
MyUEProject/
├── Content/
├── Source/
└── Plugins/
    └── AIBlueprintAssistant/        ← 复制到这里
        ├── AIBlueprintAssistant.uplugin
        └── Source/
```

### 2. 重新生成项目文件

在 `.uproject` 文件上右键 → **Generate Visual Studio project files**，或在命令行运行：

```bash
# Windows
UnrealBuildTool.exe -projectfiles -project="YourProject.uproject" -game -rocket -progress

# macOS / Linux
./GenerateProjectFiles.sh
```

### 3. 编译项目

在 Visual Studio / Rider 中选择 `Development Editor` 配置并编译，或在 UE 编辑器中点击顶部工具栏的 **Compile**。

### 4. 启用插件

**Edit → Plugins → Editor 分类** 下找到 **AI Blueprint Assistant**，勾选启用后重启编辑器。

---

## 使用方法

### 打开面板

菜单栏 → **Window → AI Blueprint Assistant**

### 配置插件设置

所有持久化配置均在：**Edit → Project Settings → Plugins → AI Blueprint Assistant**

#### API 配置

| 字段 | 说明 |
|---|---|
| **API URL** | 接口地址，默认 OpenAI 官方地址 |
| **API Key** | Bearer Token（`sk-...`），保存于用户本地配置，不提交到版本控制 |

支持任意兼容 OpenAI Chat Completions 格式的接口：

| 服务 | URL 示例 |
|---|---|
| OpenAI | `https://api.openai.com/v1/chat/completions` |
| Azure OpenAI | `https://<resource>.openai.azure.com/openai/deployments/<model>/chat/completions?api-version=2024-02-01` |
| Ollama（本地） | `http://localhost:11434/v1/chat/completions` |

#### 模型配置

| 字段 | 默认值 | 说明 |
|---|---|---|
| **Model Name** | `gpt-4o` | 模型标识符，支持任意兼容模型名 |
| **Max Tokens** | `2048` | 响应最大 Token 数（256–8192） |
| **Temperature** | `0.2` | 采样温度，越低越确定性，T3D 生成建议保持低值 |

#### 目标引擎版本

| 字段 | 默认值 | 说明 |
|---|---|---|
| **Target Major Version** | 编译时自动检测 | 固定为 5 |
| **Target Minor Version** | 编译时自动检测 | 0–9，例如 `3` 表示 UE 5.3 |

> 修改后立即生效，无需重启。面板信息栏会实时显示当前目标版本，例如 `Target UE: 5.3`。

**典型用例**：你的编辑器是 UE 5.6，但项目正在迁移到 UE 5.3 版本。将 Minor Version 设为 `3`，AI 将避免使用 5.4+ 引入的新 API，生成的节点可直接用于 5.3 项目。

#### 提示词配置

| 字段 | 说明 |
|---|---|
| **Custom System Prompt** | 留空使用内置提示词（含 Few-Shot 示例）；填入后完全替换内置提示词 |

#### 行为配置

| 选项 | 默认 | 说明 |
|---|---|---|
| **Show T3D Preview Before Import** | ✅ 开启 | 生成后弹出预览窗口，确认后再导入 |
| **Enable Conversation History** | ❌ 关闭 | 开启后保留历史对话，可连续追加需求 |
| **Max Self-Repair Attempts** | `3` | 编译失败后自动修复的最大次数；设为 `0` 可关闭自动修复 |

### 生成节点

1. 在 UE 编辑器中**打开一个蓝图**（双击任意 Blueprint 资产）
2. 在需求输入框中用自然语言描述逻辑
3. 点击 **Generate Blueprint Nodes**
4. 若开启了预览，在弹出窗口中检查 T3D 代码后点击 **Import Nodes**
5. 插件自动编译蓝图，若出现报错则触发自我修复流程
6. 状态日志显示 `[Success] Nodes generated and compiled successfully.` 后，切回蓝图编辑器查看新生成的节点

### 自我修复流程

当生成的节点导入后编译失败，插件会自动执行以下循环（最多 `Max Self-Repair Attempts` 次）：

```
导入 T3D 节点
    │
    ▼ 编译蓝图
    │
    ├─ ✅ 编译通过 → [Success] 结束
    │
    └─ ❌ 编译报错
           │
           ├─ 收集节点级错误信息
           ├─ Ctrl+Z 撤销本次导入
           ├─ 将"原始需求 + 编译错误"发送给 AI
           └─ 接收修正后的 T3D → 重新导入 → 再次编译 ↺
```

> 达到最大次数后若仍有错误，日志提示 `[Warning] Max self-repair attempts reached`，最后一次的节点保留在图表中，可手动修正或使用 Ctrl+Z 撤销。

**示例输入：**

```
当 Event BeginPlay 触发时，将 Health 变量设为 100，
并用 Print String 在屏幕上显示"游戏开始"
```

### 多轮对话

1. 在 Project Settings 中开启 **Enable Conversation History**
2. 第一次生成后，面板信息栏显示 `History: 1 turn(s)`
3. 继续输入追加需求，AI 会在上一次结果的基础上扩展
4. 点击 **New Conversation** 清空历史，开始全新一轮

### 撤销生成的节点

使用标准快捷键 **Ctrl+Z** 即可完整撤销本次生成的所有节点。

---

## 架构设计

插件采用分层解耦架构：

```
用户输入
    │
    ▼
SAIBPAssistantWidget          ← UI 层：Slate 面板，渲染控件，响应用户操作
    │
    ├──► UAIBPSettings         ← 配置层：UDeveloperSettings 子类，持久化所有参数
    │
    ├──► UAIBPContextUtils     ← 上下文层：读取蓝图的变量/组件/函数/接口/父类
    │
    ├──► UAIBPHttpService      ← 网络层：异步 POST，构造含 Few-Shot 的系统提示词
    │
    ├──► SAIBPPreviewDialog    ← 预览层：模态窗口，展示 T3D 代码供用户确认
    │
    └──► FAIBPNodeFactory      ← 注入层：T3D 解析，事务管理，节点位置排列
```

### 数据流

```
1. OnGenerateClicked()
       │
2. UAIBPSettings::GetDefault()   读取 API URL / Key / 模型 / 目标版本
       │
3. UAIBPContextUtils::GetActiveBlueprintData()
       │  返回 { baseClass, variables[], components[], functions[], interfaces[] } JSON
       │
4. UAIBPHttpService::SendRequest()
       │  构造系统提示词（含目标版本 + Few-Shot 示例 + 对话历史）
       │  POST → AI API（后台线程）
       │
5. 解析响应，提取 T3D 代码块
       │
6. AsyncTask(GameThread) →（可选）SAIBPPreviewDialog 模态预览
       │  用户确认后继续，取消则终止
       │
7. FAIBPNodeFactory::ExecuteT3DImport()
       │  FScopedTransaction → ImportNodesFromText → OffsetNodes → NotifyGraphChanged
       │  → CompileBlueprint() → 收集节点级 ErrorMsg
       │
8. 编译成功 → 记录对话历史（若启用）+ AppendLog("[Success]")
   编译失败 → UndoTransaction() → 构造修复提示词（含编译错误）→ 回到步骤 4 重试
       │  最多重试 MaxRepairAttempts 次
```

### 关键设计说明

**为什么用 T3D 格式？**

T3D 是 UE 内部的蓝图序列化格式。通过 `FEdGraphUtilities::ImportNodesFromText` 可直接解析为图表节点，天然支持编辑器撤销系统，无需经过任何编译流程。

**为什么注入蓝图上下文？**

AI 模型没有记忆。将当前蓝图的变量名、组件名、自定义函数名和接口随每次请求一并发送，使模型在生成连线时引用正确的名称，避免连接到不存在的引脚。

**目标版本如何影响生成？**

系统提示词中会注入 "You are an Unreal Engine X.Y Blueprint expert" 以及 "Prefer UE X.Y APIs"，引导模型使用对应版本的 API（如 5.3 避免使用 5.4 引入的新节点）。

**线程安全**

所有 HTTP 回调在后台线程触发，通过 `AsyncTask(ENamedThreads::GameThread, ...)` 回到主线程后再操作 Slate 控件和 UObject。UI 回调使用 `TWeakPtr<SAIBPAssistantWidget>` 防止面板关闭后的野指针访问。

**自我修复如何工作？**

节点导入后调用 `FKismetEditorUtilities::CompileBlueprint()`，检查 `UBlueprint::Status` 是否为 `BS_Error`。若有错误，遍历所有事件图表节点收集 `ErrorMsg`，组合成结构化错误报告，连同原始需求一并发送给 AI。AI 返回修正后的 T3D，通过 `GEditor->UndoTransaction()` 撤销上一次导入后重新注入。

**跨版本编译兼容**

`FEdGraphUtilities::ImportNodesFromText` 在 UE 5.4 修改了 API 签名（从返回值改为 out-param）。插件通过 `#if ENGINE_MINOR_VERSION >= 4` 预处理守卫在编译时自动选择正确的调用方式，在 UE 5.0–5.3 和 5.4–5.6+ 上均可编译。

---

## 源文件结构

```
AIBlueprintAssistant/
├── AIBlueprintAssistant.uplugin           # 插件描述符
└── Source/
    └── AIBlueprintAssistant/
        ├── AIBlueprintAssistant.Build.cs  # 模块依赖配置
        ├── Public/
        │   ├── AIBlueprintAssistant.h     # 模块类（Tab 注册 + 菜单扩展）
        │   ├── AIBPSettings.h             # Project Settings 配置类
        │   ├── SAIBPAssistantWidget.h     # Slate 主面板声明
        │   ├── SAIBPPreviewDialog.h       # T3D 预览模态对话框
        │   ├── AIBPContextUtils.h         # 蓝图上下文提取工具
        │   ├── AIBPNodeFactory.h          # T3D 节点导入工厂
        │   └── AIBPHttpService.h          # 异步 HTTP 服务
        └── Private/
            ├── AIBlueprintAssistant.cpp
            ├── AIBPSettings.cpp
            ├── SAIBPAssistantWidget.cpp
            ├── SAIBPPreviewDialog.cpp
            ├── AIBPContextUtils.cpp
            ├── AIBPNodeFactory.cpp
            └── AIBPHttpService.cpp
```

---

## 模块依赖

| 模块 | 用途 |
|---|---|
| `Slate` / `SlateCore` | UI 控件渲染 |
| `UnrealEd` | `GEditor`、`FScopedTransaction` |
| `BlueprintGraph` | `UEdGraph`、蓝图节点类型 |
| `Kismet` | `FBlueprintEditorUtils` |
| `KismetCompiler` | `FKismetEditorUtilities::CompileBlueprint`（自我修复编译检查） |
| `GraphEditor` | `FEdGraphUtilities::ImportNodesFromText` |
| `HTTP` | 异步 HTTP 请求 |
| `Json` / `JsonUtilities` | JSON 序列化/反序列化 |
| `EditorWidgets` | UE 5.1+ 样式系统 |
| `DeveloperSettings` | `UAIBPSettings`（`UDeveloperSettings` 子类） |
| `MessageLog` | 错误输出到编辑器 Message Log 面板 |
| `ToolMenus` | Window 菜单项扩展 |

---

## 错误排查

### 生成后蓝图中没有出现新节点

1. 打开 **Window → Message Log → AI Blueprint Assistant** 查看详细错误
2. 确认当前有蓝图编辑器处于打开状态
3. 若开启了预览对话框，检查是否点击了 **Import Nodes**（点击 Cancel 会取消导入）
4. AI 可能返回了格式不合规的 T3D；尝试在提示词中加入更具体的描述，或调低 Temperature

### 自我修复反复失败

- 适当增大 **Max Self-Repair Attempts**（默认 3），给 AI 更多修正机会
- 检查需求描述是否引用了蓝图中不存在的变量或组件名
- 尝试将 Temperature 调低至 `0.1`，使 AI 输出更确定性
- 达到最大重试次数后，日志中的 `[Warning]` 行会列出具体编译错误，可据此手动修改需求再次生成

### 请求失败（401 / 403）

- API Key 填写有误，或对应账户配额已用尽
- 检查 **Project Settings → AI Blueprint Assistant → API Key**

### 请求失败（400）

- 所填 URL 对应的服务不支持该模型名，或接口格式不兼容
- 尝试调整 **API URL** 或更换 **Model Name**

### 编译报错：找不到头文件

- 确认 UE 版本为 5.0 或以上
- 检查项目的 `.uproject` 是否已启用该插件（`Plugins` 数组中有对应条目）
- 重新运行 Generate Project Files 后重新编译

### AI 生成了错误版本的 API 调用

- 检查 **Project Settings → Target Engine Version** 是否设置为正确的目标版本
- 面板信息栏的 `Target UE: X.Y` 应与你的项目版本一致

---

## 许可证

Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.
