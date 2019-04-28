# SPDX-License-Identifier: BSD-3-Clause

#!/bin/sh

. ./setup.sh

make O=$RTE_TARGET $*

echo ===

ls -al  $RTE_TARGET/simple

echo ===

ldd $RTE_TARGET/simple
