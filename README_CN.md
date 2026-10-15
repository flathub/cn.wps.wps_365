## WPS 365 flatpak

[English](README.md)|[中文](README_CN.md)

### 使用必读
- 要切换软件语言，创建或编辑 `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` 并将以下配置添加到开头：
    ```
    [General]
    languages=LANGUAGE
    ```
    其中 LANGUAGE 的可选项包括：`en_US`, `mn_CN`, `ru_RU`, `ug_CN` 和 `zh_CN`。
- 当“窗口管理模式”为“整合模式”时，只有“WPS文字”和“WPS Office”图标可以启动应用。如果你希望使用其它组件的 desktop 文件启动应用，你必须将“窗口管理模式”切换为“多组件模式”。或者，创建或编辑`~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf`：
    ```
    [6.0]
    wpsoffice\Application%20Settings\AppComponentMode=prome_independ
    ```

### 技巧
- 如果担心隐私问题，可以通过`flatpak --user override cn.wps.wps_365 --unshare=network`关闭网络权限.
- WPS协作 目前不能正常工作。参考[https://bbs.deepin.org/zh/post/295131](https://bbs.deepin.org/zh/post/295131)。我们暂时移除了它。如果你知道如何修复它，欢迎 issue 或者 pr。

### 实验性补丁
以下是一些实验性补丁，用于改善 WPS 365 在 Flatpak 下的体验，通过环境变量控制。可在 flatpak overrides 中设置，例如 `flatpak --user override cn.wps.wps_365 --env=KSO_ENABLE_FORWARD_SHIM=1`，或通过 Flatseal 设置。

| 变量 | 默认值 | 有效值 | 说明 |
|---|---|---|---|
| `KSO_KILL_CLOUDSVR_AFTERWARDS` | 未设置（禁用） | `1` 表示启用 | WPS 365 退出后，如果没有其他 WPS 组件仍在运行，则结束后台进程 wpscloudsvr。 |
| `KSO_KILL_CLOUDSVR_AFTERWARDS_DELAY` | `3` | 非负秒数 | WPS 365 退出后等待多久再检查并结束 wpscloudsvr。仅在 `KSO_KILL_CLOUDSVR_AFTERWARDS=1` 时生效。 |
| `KSO_ENABLE_FORWARD_SHIM` | 未设置（禁用） | `1` 表示启用 | 实验性。整合模式下，WPS 365 运行时从文件管理器打开文件，会在已有窗口中打开的同时再额外打开一个窗口，原因是每次启动都运行在独立的沙箱中。启用后会预加载一个小型库（[wps_forward_shim.c](wps_forward_shim.c)），在 WPS 已将文件交给已有窗口后结束新进程。 |
| `KSO_DISABLE_FORCE_LOGIN` | `1` | `1` 表示启用，未设置为禁用 | |
| `DISABLE_FORCE_LOGIN_DELAY` | `3` | 非负秒数 | |