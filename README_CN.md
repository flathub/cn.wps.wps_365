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
- WPS 365 可能会创建后台进程。在这种情况下你需要使用`flatpak kill cn.wps.wps_365`才能彻底关闭程序.