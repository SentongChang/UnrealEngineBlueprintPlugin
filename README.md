# AI Blueprint Assistant

一个用于 **Unreal Engine 5.6** 的编辑器插件，通过自然语言描述自动生成蓝图节点，并直接注入到当前打开的蓝图图表中。

---

## 功能概览

- **自然语言生成节点**：在文本框中描述你想要的逻辑，点击按钮，AI 生成对应的蓝图节点并自动排列到图表中
- **上下文感知**：自动读取当前蓝图的变量列表、组件树和父类信息，确保 AI 生成的节点使用正确的变量名和组件名进行连线
- **原生 T3D 注入**：通过 `FEdGraphUtilities::ImportNodesFromText` 将节点直接导入图表，非代码生成，完全兼容编辑器撤销/重做（Ctrl+Z）
- **异步无阻塞**：HTTP 请求在后台线程执行，编辑器界面不卡顿
- **兼容 OpenAI 格式的 API**：支持 OpenAI、Azure OpenAI、本地 Ollama 等任意兼容接口

---

## 面板预览

```
┌─────────────────────────────────────────┐
│  AI Blueprint Assistant            × □  │
├─────────────────────────────────────────┤
│  API URL                                │
│  [https://api.openai.com/v1/...]        │
│                                         │
│  API Key                                │
│  [••••••••••••••••••••]                 │
│                                         │
│  描述你想生成的蓝图节点：               │
│  ┌───────────────────────────────────┐  │
│  │ 当玩家与 TriggerBox 重叠时，      │  │
│  │ 将 Health 变量减少 10             │  │
│  └───────────────────────────────────┘  │
│                                         │
│       [ Generate Blueprint Nodes ]      │
│                                         │
│  状态：Ready                            │
│  ┌───────────────────────────────────┐  │
│  │ [Info] Fetching blueprint context │  │
│  │ [Info] Sending request to AI...   │  │
│  │ [Success] 节点生成成功            │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

---

## 环境要求

| 项目 | 要求 |
|---|---|
| Unreal Engine | 5.6 |
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

### 配置 API

| 字段 | 说明 |
|---|---|
| **API URL** | 接口地址，默认填入 OpenAI 官方地址 |
| **API Key** | 你的 Bearer Token，输入后自动掩码显示 |

支持任意兼容 OpenAI Chat Completions 格式的接口：

| 服务 | URL 示例 |
|---|---|
| OpenAI | `https://api.openai.com/v1/chat/completions` |
| Azure OpenAI | `https://<resource>.openai.azure.com/openai/deployments/<model>/chat/completions?api-version=2024-02-01` |
| Ollama（本地） | `http://localhost:11434/v1/chat/completions` |

### 生成节点

1. 在 UE 编辑器中**打开一个蓝图**（双击任意 Blueprint 资产）
2. 在需求输入框中用自然语言描述逻辑
3. 点击 **Generate Blueprint Nodes**
4. 状态日志显示 `[Success]` 后，切回蓝图编辑器查看新生成的节点

**示例输入：**

```
当 Event BeginPlay 触发时，将 Health 变量设为 100，
并用 Print String 在屏幕上显示"游戏开始"
```

### 撤销生成的节点

使用标准快捷键 **Ctrl+Z** 即可完整撤销本次生成的所有节点。

---

## 架构设计

插件采用四层解耦架构，每层职责单一：

```
用户输入
    │
    ▼
SAIBPAssistantWidget          ← UI 层：Slate 面板，渲染控件，响应用户操作
    │
    ├──► UAIBPContextUtils     ← 上下文层：读取当前蓝图的变量/组件/父类
    │
    ├──► UAIBPHttpService      ← 网络层：异步 POST 请求，构造系统提示词
    │
    └──► FAIBPNodeFactory      ← 注入层：T3D 解析，事务管理，节点位置排列
```

### 数据流

```
1. OnGenerateClicked()
       │
2. UAIBPContextUtils::GetActiveBlueprintData()
       │  返回 { baseClass, variables[], components[] } JSON
       │
3. UAIBPHttpService::SendRequest()
       │  POST → AI API（后台线程）
       │
4. 解析响应，提取 T3D 代码块
       │
5. AsyncTask(GameThread) → FAIBPNodeFactory::ExecuteT3DImport()
       │  FScopedTransaction → ImportNodesFromText → NotifyGraphChanged
       │
6. AppendLog("[Success]") + SetGenerating(false)
```

### 关键设计说明

**为什么用 T3D 格式？**

T3D 是 UE 内部的蓝图序列化格式。通过 `FEdGraphUtilities::ImportNodesFromText` 可将其直接解析为图表节点，天然支持编辑器的撤销系统，无需经过任何编译流程。

**为什么注入蓝图上下文？**

AI 模型没有记忆。将当前蓝图的变量名和组件名随每次请求一并发送，使模型在生成连线时引用正确的名称，避免连接到不存在的引脚。

**线程安全**

所有 HTTP 回调在后台线程触发，通过 `AsyncTask(ENamedThreads::GameThread, ...)` 回到主线程后再操作 Slate 控件和 UObject。UI 回调使用 `TWeakPtr<SAIBPAssistantWidget>` 防止面板关闭后的野指针访问。

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
        │   ├── SAIBPAssistantWidget.h     # Slate UI 面板声明
        │   ├── AIBPContextUtils.h         # 蓝图上下文提取工具
        │   ├── AIBPNodeFactory.h          # T3D 节点导入工厂
        │   └── AIBPHttpService.h          # 异步 HTTP 服务
        └── Private/
            ├── AIBlueprintAssistant.cpp
            ├── SAIBPAssistantWidget.cpp
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
| `GraphEditor` | `FEdGraphUtilities::ImportNodesFromText` |
| `HTTP` | 异步 HTTP 请求 |
| `Json` / `JsonUtilities` | JSON 序列化/反序列化 |
| `EditorWidgets` | UE5.1+ 样式系统（替代废弃的 `EditorStyle`） |
| `MessageLog` | 错误输出到编辑器 Message Log 面板 |
| `ToolMenus` | Window 菜单项扩展 |

---

## 错误排查

### 生成后蓝图中没有出现新节点

1. 打开 **Window → Message Log → AI Blueprint Assistant** 查看详细错误
2. 确认当前有蓝图编辑器处于打开状态
3. AI 可能返回了格式不合规的 T3D；尝试在提示词中加入更具体的描述

### 请求失败（401 / 403）

- API Key 填写有误，或对应账户配额已用尽

### 请求失败（400）

- 所填 URL 对应的服务不支持该模型名，或接口格式不兼容
- 尝试调整 API URL 或在请求体中更换模型名

### 编译报错：找不到头文件

- 确认 UE 版本为 5.6
- 检查项目的 `.uproject` 是否已启用该插件（`Plugins` 数组中有对应条目）

---

## 已知限制与后续计划

| 限制 | 计划 |
|---|---|
| T3D 格式偶有偏差 | 在系统提示词中附加真实 T3D 示例（few-shot）以提升准确率 |
| 模型名硬编码为 `gpt-4o` | 通过 `UDeveloperSettings` 子类开放 Project Settings 配置 |
| 不支持流式输出 | 计划支持 `"stream": true`，实时显示生成进度 |
| 节点直接注入无预览 | 计划增加预览/确认步骤 |

---

## 许可证

Copyright (c) 2026 AIBlueprintAssistant. All Rights Reserved.
