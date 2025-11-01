#!/bin/bash
/opt/m68k-amigaos/bin/sfdc --addvectors library --target m68k-amigaos --mode clib gic400_lib.sfd >../include/clib/gic400_protos.h
/opt/m68k-amigaos/bin/sfdc --addvectors library --target m68k-amigaos --mode macros gic400_lib.sfd >../include/inline/gic400.h
/opt/m68k-amigaos/bin/sfdc --addvectors library --target m68k-amigaos --mode pragmas gic400_lib.sfd >../include/pragmas/gic400_pragmas.h
/opt/m68k-amigaos/bin/sfdc --addvectors library --target m68k-amigaos --mode proto gic400_lib.sfd >../include/proto/gic400.h


