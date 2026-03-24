#!/usr/bin/env bash
set -Eeuo pipefail

trap 'echo "[ERREUR] La ligne $LINENO a échoué."' ERR

PROJECT_FILE="${1:-machineDecoupeIHM.pro}"

APT_PACKAGES=(
  build-essential
  g++
  make
  pkg-config
  git
  cmake
  ninja-build
  qmake6
  qt6-base-dev
  qt6-base-dev-tools
  qt6-tools-dev
  qt6-tools-dev-tools
  qt6-svg-dev
  qt6-connectivity-dev
  qt6-httpserver-dev
  libqt6opengl6-dev
  linguist-qt6
  qt6-l10n-tools
  libopencv-dev
  libgpiod-dev
  libgl1-mesa-dev
  libegl1-mesa-dev
)

log() {
  echo
  echo "========== $1 =========="
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "[ERREUR] Commande manquante : $1"
    exit 1
  }
}

log "Mise à jour APT"
sudo apt update

log "Installation des paquets"
sudo apt install -y "${APT_PACKAGES[@]}"

log "Vérifications outils"
require_cmd qmake6
require_cmd pkg-config
require_cmd g++

echo "[OK] qmake6: $(command -v qmake6)"
echo "[OK] g++:    $(command -v g++)"
echo "[OK] pkg-config: $(command -v pkg-config)"

log "Vérification bibliothèques externes"
pkg-config --exists opencv4 || { echo "[ERREUR] opencv4 introuvable via pkg-config"; exit 1; }
pkg-config --exists libgpiod || { echo "[ERREUR] libgpiod introuvable via pkg-config"; exit 1; }

echo "[OK] OpenCV version : $(pkg-config --modversion opencv4)"
echo "[OK] libgpiod version : $(pkg-config --modversion libgpiod)"

log "Infos Qt"
qmake6 -query QT_VERSION
qmake6 -query QT_INSTALL_PREFIX

log "Génération Makefile"
if [ ! -f "$PROJECT_FILE" ]; then
  echo "[ERREUR] Fichier projet introuvable : $PROJECT_FILE"
  exit 1
fi

qmake6 "$PROJECT_FILE"

log "Message final"
echo "Configuration terminée."
echo "Pour compiler : make -j$(nproc)"