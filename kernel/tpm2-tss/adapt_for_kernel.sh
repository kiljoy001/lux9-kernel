#!/bin/bash
# Adapt TPM2-TSS sources for Lux9 kernel compilation

set -e

echo "Adapting TPM2-TSS for kernel compilation..."

# Find all C files in mu/ and sapi/
find mu/ sapi/ -name "*.c" -type f | while read file; do
    echo "Processing: $file"

    # Create backup
    cp "$file" "$file.orig"

    # Replace standard C includes with kernel compatibility header
    # This adds our kernel header at the top and comments out conflicting includes
    sed -i '1i\/* Kernel compatibility */\n#include "../include/tss2_kernel.h"\n' "$file"

    # Comment out config.h include (we define HAVE_CONFIG_H in tss2_kernel.h)
    sed -i 's/^#include "config.h"/#include "config.h" \/\/ Provided by tss2_kernel.h/' "$file"

    # Comment out standard C library includes
    sed -i 's/^#include <inttypes.h>/#include <inttypes.h> \/\/ Provided by tss2_kernel.h/' "$file"
    sed -i 's/^#include <string.h>/#include <string.h> \/\/ Provided by tss2_kernel.h/' "$file"
    sed -i 's/^#include <stdint.h>/#include <stdint.h> \/\/ Provided by tss2_kernel.h/' "$file"
    sed -i 's/^#include <stdbool.h>/#include <stdbool.h> \/\/ Provided by tss2_kernel.h/' "$file"

    # Fix include paths for tss2 headers (they're in ../include/)
    sed -i 's|#include "tss2_|#include "../include/tss2_|g' "$file"

    # Fix util includes
    sed -i 's|#include "util/|#include "../util/|g' "$file"

    # Fix sysapi_util include for sapi files
    if [[ "$file" == sapi/* ]]; then
        sed -i 's|#include "sysapi_util.h"|#include "../sapi/sysapi_util.h"|g' "$file"
    fi
done

# Also fix header files
echo "Processing header files..."
find sapi/ -name "*.h" -type f | while read file; do
    echo "Processing header: $file"
    cp "$file" "$file.orig"

    # Comment out standard includes
    sed -i 's/^#include <stddef.h>/#include <stddef.h> \/\/ Provided by tss2_kernel.h/' "$file"
    sed -i 's/^#include <stdint.h>/#include <stdint.h> \/\/ Provided by tss2_kernel.h/' "$file"

    # Fix tss2 header paths
    sed -i 's|#include "tss2_|#include "../include/tss2_|g' "$file"
done

echo "Adaptation complete!"
echo "Original files backed up with .orig extension"
