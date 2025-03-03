#!/usr/bin/env bash
set -ex

[[ -d interop/js ]]
which npm
which npx

cmake --build $BUILD --target interop

(cd interop/js &&
  npm install &&
  npx patch-package &&
  npm run build)

(cd interop/test &&
  npm install &&
  npx patch-package &&
  npm run build)

(cd interop/test &&
  npm test)
