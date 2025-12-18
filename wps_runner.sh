#!/bin/bash
DEFAULT_DATA_HOME="${HOME}/.local/share"
DISABLE_FORCE_LOGIN_DELAY="${DISABLE_FORCE_LOGIN_DELAY:-3}"

# Symlink hardcoded datadir to XDG_DATA_HOME
if [ ! -d "${DEFAULT_DATA_HOME}/Kingsoft" ]; then
    test -d "${DEFAULT_DATA_HOME}" || mkdir -p "${DEFAULT_DATA_HOME}"
    ln -s "${XDG_DATA_HOME}/Kingsoft" "${DEFAULT_DATA_HOME}/Kingsoft"
else
    msg "Data dir exists, not touching it"
fi

# Fix input method for Chinese users
[[ "$XMODIFIERS" == "@im=fcitx" ]] && export QT_IM_MODULE=fcitx

# Disable force login after a delay
sleep ${DISABLE_FORCE_LOGIN_DELAY} && sed -i "s/enableForceLogin=true/enableForceLogin=false/" "$XDG_CONFIG_HOME/Kingsoft/Office.conf" &

/app/extra/usr/bin/$(basename "$0") "$@"