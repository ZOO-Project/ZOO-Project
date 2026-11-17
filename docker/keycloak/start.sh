#!/usr/bin/env bash
# Local Keycloak (dev mode) for the ZOO-Project WebUI login.
#
#   docker/keycloak/start.sh              # start (or restart) the container
#   docker/keycloak/start.sh --recreate   # rebuild it, e.g. after the LAN IP changed
#
# Realm `zooproject`, client `webapp` (secret zoo-webapp-secret, audience
# zoo-project-webapp), user jules/zoo — see zooproject-realm.json.
# The public URL must be reachable from the browser AND from the cluster pods
# (webui + zookernel), hence the Mac's LAN IP rather than localhost.
# Admin console: <url>/admin (admin/admin).
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
NAME=zoo-keycloak
IMAGE="${KEYCLOAK_IMAGE:-quay.io/keycloak/keycloak:26.4}"
IP="$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null || hostname -I 2>/dev/null | awk '{print $1}')"
URL="${KEYCLOAK_URL:-http://${IP}:8085}"

if [ "${1:-}" = "--recreate" ]; then docker rm -f "$NAME" >/dev/null 2>&1 || true; fi
if docker container inspect "$NAME" >/dev/null 2>&1; then
  docker start "$NAME" >/dev/null
  URL="$(docker inspect "$NAME" --format '{{range .Config.Env}}{{println .}}{{end}}' | sed -n 's/^KC_HOSTNAME=//p')"
  echo "Keycloak container restarted (use --recreate to change its URL)."
else
  docker run -d --name "$NAME" -p 8085:8080 \
    -e KC_BOOTSTRAP_ADMIN_USERNAME=admin -e KC_BOOTSTRAP_ADMIN_PASSWORD=admin \
    -e KC_HOSTNAME="$URL" -e KC_HTTP_ENABLED=true \
    -v "$DIR/zooproject-realm.json:/opt/keycloak/data/import/zooproject-realm.json:ro" \
    "$IMAGE" start-dev --import-realm >/dev/null
  echo "Keycloak container created."
fi
echo "URL: $URL   (admin console: $URL/admin, admin/admin; user jules/zoo)"
echo "export KEYCLOAK_URL=$URL"
