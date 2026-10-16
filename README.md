## WPS 365 flatpak

[English](README.md)|[中文](README_CN.md)

### Important
- If the ui doesn't display in your language, edit or create `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` and add the following lines at the beginning:
    ```
    [General]
    languages=LANGUAGE
    ```
    supported value of LANGUAGE includes `en_US`, `mn_CN`, `ru_RU`, `ug_CN` and `zh_CN`.
- If only the "Writer" icon can launch the software while "Spreadsheets" and "Presentation" do not respond, edit or create `~/.var/app/cn.wps.wps_365/config/Kingsoft/Office.conf` with
    ```
    [6.0]
    wpsoffice\Application%20Settings\AppComponentMode=prome_independ
    ```

### Tips
- If your desktop environment isn't included in: `gnome`, `unity`, `kde`, `ukui`, `kylin`, `deepin`, the "Open File Location" button may not work. You can set the `XDG_CURRENT_DESKTOP` environment variable to one of the above in flatpak overrides to fix.
- Xiezuo currently doesn't work. Checkout discussion at: [https://bbs.deepin.org/zh/post/295131](https://bbs.deepin.org/zh/post/295131). So we remove its files. If anyone knows how to fix it, please feel free to contribute.

### Workaround switches
These optional workarounds are controlled by environment variables. Set them in flatpak overrides, e.g. `flatpak --user override cn.wps.wps_365 --env=KSO_ENABLE_FORWARD_SHIM=1` or with Flatseal.

| Variable | Default | Valid values | Description |
|---|---|---|---|
| `KSO_KILL_CLOUDSVR_AFTERWARDS` | unset (disabled) | `1` to enable | Kill the background process wpscloudsvr after WPS 365 quits, if no other WPS component is still running. |
| `KSO_KILL_CLOUDSVR_AFTERWARDS_DELAY` | `3` | Non-negative number of seconds | How long to wait after WPS 365 quits before checking and killing wpscloudsvr. Only takes effect when `KSO_KILL_CLOUDSVR_AFTERWARDS=1`. |
| `KSO_ENABLE_FORWARD_SHIM` | unset (disabled) | `1` to enable | Experimental. In integrated mode, opening a file from the file manager while WPS 365 is running opens it in the running window and also in an extra window, because each launch runs in its own sandbox. This preloads a small library ([wps_forward_shim.c](wps_forward_shim.c)) that ends the new process once WPS has handed the file to the running window. |
| `KSO_DISABLE_FORCE_LOGIN` | `1` | `1` to enable, unset to disable | |
| `DISABLE_FORCE_LOGIN_DELAY` | `3` | Non-negative number of seconds | |