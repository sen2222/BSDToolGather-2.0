# BSDToolGatger

这是一个 Qt/MinGW 工具项目，目前包含 OTA 固件打包、OTA 压测、字模转换、CDC 串口指令等功能。

## 编译脚本

所有编译入口脚本都放在 `build` 目录下：

- `build_config.bat`：配置本机编译路径和打包路径。
- `build_all.bat`：编译项目，并打包成可运行程序。
- `clean_all.bat`：清理项目编译产生的临时文件。

项目编译产生的文件会输出到 `out` 目录，不会放到 `build` 目录。`build` 目录只保留脚本。

## 第一次使用

先修改 `build\build_config.bat` 文件顶部的路径配置：

```bat
set "PACKAGE_ROOT=E:\app"
set "QT_BIN=D:\qt-create\6.5.3\mingw_64\bin"
set "MINGW_BIN=D:\qt-create\Tools\mingw1120_64\bin"
set "DEFAULT_BUILD_MODE=release"
```

然后运行：

```bat
cd build
build_config.bat
```

`build_config.bat` 会把配置写入当前用户环境配置。必须先运行这个脚本，再运行 `build_all.bat`。

如果没有先运行 `build_config.bat`，直接运行 `build_all.bat` 会报错并退出。

## 使用环境变量里的编译器

如果 Qt 和 MinGW 已经配置到了系统环境变量或 `PATH` 中，可以把编译器路径写成 `USE_ENV`：

```bat
set "QT_BIN=USE_ENV"
set "MINGW_BIN=USE_ENV"
```

使用 `USE_ENV` 时：

- Qt 工具会从 `%QT_BIN%` 或 `PATH` 中查找。
- MinGW 工具会从 `%MINGW_BIN%` 或 `PATH` 中查找。

需要能找到下面这些工具：

- `qmake.exe`
- `windeployqt.exe`
- `mingw32-make.exe`

## 编译并打包

完成配置后运行：

```bat
cd build
build_all.bat
```

默认编译模式由 `build_config.bat` 里的 `DEFAULT_BUILD_MODE` 决定。

也可以手动指定编译模式：

```bat
build_all.bat release
build_all.bat debug
```

编译输出目录：

```text
out\Package_release
out\Package_debug
```

打包输出目录：

```text
%PACKAGE_ROOT%\BSDToolGatger
```

打包时会复制 Qt 运行库、`config` 目录，以及下面这些第三方库：

- `libfreetype.dll`
- `libusb-1.0.dll`
- `libuvc.dll`

## 清理

运行：

```bat
cd build
clean_all.bat
```

该脚本会清理项目编译产生的文件，例如 `out`、`debug`、`release`、根目录 Makefile，以及 `source` 下部分编译临时文件。

下面这些第三方库输出目录会保留，因为项目链接和打包时需要使用它们：

```text
source\freetype\out
source\libusb\out
source\libuvc\out
```
