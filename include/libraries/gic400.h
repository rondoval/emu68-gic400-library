// SPDX-License-Identifier: MPL-2.0 OR GPL-2.0+
#ifndef LIBRARIES_GIC400_H
#define LIBRARIES_GIC400_H

#include <exec/types.h>

/* Public GIC-400 API status codes. Functions return 0 on success or one of
 * these negative values on failure.
 */
#define GIC400_ERR_NOT_READY ((LONG)-1)
#define GIC400_ERR_INVALID_IRQ ((LONG)-2)
#define GIC400_ERR_INVALID_ARGUMENT ((LONG)-3)
#define GIC400_ERR_NOT_ROUTABLE ((LONG)-4)
#define GIC400_ERR_ALREADY_REGISTERED ((LONG)-5)
#define GIC400_ERR_NOT_FOUND ((LONG)-6)
#define GIC400_ERR_NO_MEMORY ((LONG)-7)
#define GIC400_ERR_DEVTREE ((LONG)-8)

struct GICInfo
{
    ULONG distributorIIDR;
    ULONG distributorTyper;
    ULONG cpuInterfaceIIDR;
    ULONG maxIrqs;
    UBYTE cpuCount;
    UBYTE securityExtensions;
    UBYTE lspiCount;
};

#endif /* LIBRARIES_GIC400_H */
