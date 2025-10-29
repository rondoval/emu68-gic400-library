//SPDX-License-Identifier: MPL-2.0
#ifndef LIBRARIES_GIC400_H
#define LIBRARIES_GIC400_H

#include <exec/types.h>

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
