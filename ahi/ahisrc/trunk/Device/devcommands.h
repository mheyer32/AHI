
#ifndef _DEVCOMMANDS_H_
#define _DEVCOMMANDS_H_

#include "ahi_def.h"

void
PerformIO ( struct AHIRequest *ioreq,
            struct AHIBase *AHIBase );

struct Node *
FindNode ( struct List *list,
           struct Node *node );

void
FeedReaders ( struct AHIDevUnit *iounit,
              struct AHIBase *AHIBase );

void
RethinkPlayers ( struct AHIDevUnit *iounit,
                 struct AHIBase *AHIBase );

void 
UpdateSilentPlayers ( struct AHIDevUnit *iounit,
                      struct AHIBase *AHIBase );

#endif /* _DEVCOMMANDS_H_ */