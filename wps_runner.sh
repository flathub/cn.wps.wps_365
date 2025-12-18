#!/bin/bash

function msg() {
    echo "Wrapper: $*" >&2
}

DEFAULT_DATA_HOME="${HOME}/.local/share"
DISABLE_FORCE_LOGIN_DELAY="${DISABLE_FORCE_LOGIN_DELAY:-3}"
CONF_FILE="${XDG_CONFIG_HOME}/Kingsoft/Office.conf"
BACKUPS_SUBDIR="Kingsoft/office6/data/backup"
OLD_BACKUP_PATH="${DEFAULT_DATA_HOME}/${BACKUPS_SUBDIR}"
NEW_BACKUP_PATH="${XDG_DATA_HOME}/${BACKUPS_SUBDIR}"

# Set backup path to correct dir under XDG_DATA_HOME in the config
if [ ! -f "${CONF_FILE}" ]; then
    msg "No config exists, creating one"
    mkdir -p "${NEW_BACKUP_PATH}"
    mkdir -p "$(dirname "${CONF_FILE}")"
    cat <<EOF > "${CONF_FILE}"
[6.0]
common\Backup\AutoRecoverFilePath=${NEW_BACKUP_PATH}
wpsoffice\Application%20Settings\AppComponentMode=prome_independ
EOF
elif grep -q "${OLD_BACKUP_PATH}" "${CONF_FILE}"; then
    msg "Config file contains old paths, updating"
    mkdir -p "${NEW_BACKUP_PATH}"
    sed "s|${OLD_BACKUP_PATH}|${NEW_BACKUP_PATH}|g" -i "${CONF_FILE}"
fi

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