#!/usr/bin/env bash
# Local Keycloak (dev mode) for the ZOO-Project WebUI login.
#
#   docker/keycloak/start.sh              # start (or restart) the container
#   docker/keycloak/start.sh --recreate   # rebuild it, e.g. after the LAN IP changed
#
# Realm `zooproject`, confidential client `webapp` (audience zoo-project-webapp)
# and a test user `demo`, see zooproject-realm.json. No credential is stored in
# the repository: on first use this script generates the client secret and the
# passwords into docker/keycloak/.env (git-ignored, mode 600) and reuses them
# afterwards. To change them, delete that file and run with --recreate.
# Load the file before deploying with Skaffold:
#   set -a; . docker/keycloak/.env; set +a
# The public URL must be reachable from the browser AND from the cluster pods
# (webui + zookernel), hence the machine's LAN IP rather than localhost.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
NAME=zoo-keycloak
IMAGE="${KEYCLOAK_IMAGE:-quay.io/keycloak/keycloak:26.4}"
ENV_FILE="$DIR/.env"

if [ "${1:-}" = "--recreate" ]; then docker rm -f "$NAME" >/dev/null 2>&1 || true; fi

if docker container inspect "$NAME" >/dev/null 2>&1; then
  docker start "$NAME" >/dev/null
  echo "Keycloak container restarted (use --recreate to change its URL)."
  echo "Settings and credentials: $ENV_FILE"
  exit 0
fi

# Saved credentials, unless already set in the environment.
saved() { if [ -f "$ENV_FILE" ]; then sed -n "s/^$1=//p" "$ENV_FILE" | tail -1; fi; }
new_secret() { openssl rand -hex 16; }
CLIENT_SECRET="${KEYCLOAK_CLIENT_SECRET:-$(saved KEYCLOAK_CLIENT_SECRET)}"
DEMO_PASSWORD="${KEYCLOAK_DEMO_PASSWORD:-$(saved KEYCLOAK_DEMO_PASSWORD)}"
ADMIN_PASSWORD="${KEYCLOAK_ADMIN_PASSWORD:-$(saved KEYCLOAK_ADMIN_PASSWORD)}"
CLIENT_SECRET="${CLIENT_SECRET:-$(new_secret)}"
DEMO_PASSWORD="${DEMO_PASSWORD:-$(new_secret)}"
ADMIN_PASSWORD="${ADMIN_PASSWORD:-$(new_secret)}"

IP="$(ipconfig getifaddr en0 2>/dev/null || ipconfig getifaddr en1 2>/dev/null || hostname -I 2>/dev/null | awk '{print $1}')"
URL="${KEYCLOAK_URL:-http://${IP}:8085}"

(
  umask 077
  cat > "$ENV_FILE" <<ENV
KEYCLOAK_URL=$URL
KEYCLOAK_CLIENT_SECRET=$CLIENT_SECRET
KEYCLOAK_DEMO_USER=demo
KEYCLOAK_DEMO_PASSWORD=$DEMO_PASSWORD
KEYCLOAK_ADMIN_USER=admin
KEYCLOAK_ADMIN_PASSWORD=$ADMIN_PASSWORD
ENV
)

# The realm file reads ZOO_WEBAPP_CLIENT_SECRET and ZOO_DEMO_PASSWORD from the
# container environment (Keycloak resolves ${...} placeholders at import).
docker run -d --name "$NAME" -p 8085:8080 \
  -e KC_BOOTSTRAP_ADMIN_USERNAME=admin -e KC_BOOTSTRAP_ADMIN_PASSWORD="$ADMIN_PASSWORD" \
  -e ZOO_WEBAPP_CLIENT_SECRET="$CLIENT_SECRET" -e ZOO_DEMO_PASSWORD="$DEMO_PASSWORD" \
  -e KC_HOSTNAME="$URL" -e KC_HTTP_ENABLED=true \
  -v "$DIR/zooproject-realm.json:/opt/keycloak/data/import/zooproject-realm.json:ro" \
  "$IMAGE" start-dev --import-realm >/dev/null
echo "Keycloak container created: $URL (admin console: $URL/admin)"
echo "Settings and credentials: $ENV_FILE"
echo "Load them before deploying:  set -a; . $ENV_FILE; set +a"
