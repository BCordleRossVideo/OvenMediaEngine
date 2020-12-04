#!/bin/bash

KEY="$1"
BASE_URL="$2"
SIGNATURE_QUERY_KEY_NAME="$3"
POLICY_QUERY_KEY_NAME="$4"
POLICY="$5"

if [[ -z "${KEY}" || -z "${BASE_URL}" || -z "${SIGNATURE_QUERY_KEY_NAME}" || -z "${POLICY_QUERY_KEY_NAME}" || -z "${POLICY}" ]]
then
	echo "Usage: $0 [HMAC_KEY] [BASE_URL] [SIGNATURE_QUERY_KEY_NAME] [POLICY_QUERY_KEY_NAME] [POLICY]"
	echo ""
	echo "Example:"
	echo "    $0 ome_is_the_best ws://host:3333/app/stream signature policy '{\"url_expire\":1604377520178}'"
	echo "    => ws://host:3333/app/stream?policy=e3BvbGljeV9leHBpcmU6MTYwNDM3NzUyMDE3OH0&signature=WTgJM2CZZ5WGGsnwyAIrlzZNA_Q"

	exit
fi

# 1. Perform base64url() for POLICY (RFC 4648 5.)
# POLICY_BASE64 = base64url(POLICY)
POLICY_BASE64=$(echo -n "${POLICY}" | base64)
POLICY_BASE64=${POLICY_BASE64%==}
POLICY_BASE64=${POLICY_BASE64%=}
POLICY_BASE64=${POLICY_BASE64//+/-}
POLICY_BASE64=${POLICY_BASE64//\//_}

# 2. Generates an URL such as "ws://ome_host:3333/app/stream?policy=${POLICY_BASE64}"
# Check if BASE_URL has a question mark
[ "${BASE_URL#*\?}" = "${BASE_URL}" ] && QS_SEPARATOR="?" || QS_SEPARATOR="&"
POLICY_URL="${BASE_URL}${QS_SEPARATOR}policy=${POLICY_BASE64}"

# 3. Perform sha1(base64url()) for SIGNATURE (RFC 4648 5.)
# SHA1 = sha1(POLICY_URL)
SHA1=$(echo -n "${POLICY_URL}" | openssl dgst -sha1 -hmac "${KEY}")
# Remove the "(stdin) =" prefix which is generated by openssl
SHA1=${SHA1#*= }
# SIGNATURE = base64url(SHA1)
SIGNATURE=$(echo -n "${SHA1}" | xxd -r -p | base64)
SIGNATURE=${SIGNATURE%==}
SIGNATURE=${SIGNATURE%=}
SIGNATURE=${SIGNATURE//+/-}
SIGNATURE=${SIGNATURE//\//_}

# 4. Create a whole URL
echo "${POLICY_URL}&${SIGNATURE_QUERY_KEY_NAME}=${SIGNATURE}"