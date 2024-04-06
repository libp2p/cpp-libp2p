#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

GITHUB_ENV="${GITHUB_ENV:?}"
export VIRTUAL_ENV="${1:?venv path not set}"
export PATH="$VIRTUAL_ENV/bin:$PATH"
echo "VIRTUAL_ENV=$VIRTUAL_ENV" >> "$GITHUB_ENV"
echo "PATH=$PATH" >> "$GITHUB_ENV"
