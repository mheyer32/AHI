#ifndef _CA_SUPPORT_H_
#define _CA_SUPPORT_H_

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

struct List *ChooserLabelsA(STRPTR *);
void FreeChooserLabels(struct List *);
struct List *ClickTabsA(STRPTR *);
void FreeClickTabs(struct List *);
struct List *BrowserNodesA(STRPTR *);
void FreeBrowserNodes(struct List *);

#ifdef __AMIGAOS4__
#define ChooserLabels(...) ({ \
  STRPTR __args[] = { __VA_ARGS__ }; \
  ChooserLabelsA(__args); \
  })
#define ClickTabs(...) ({ \
  STRPTR __args[] = { __VA_ARGS__ }; \
  ClickTabsA(__args); \
  })
#define BrowserNodes(...) ({ \
  STRPTR __args[] = { __VA_ARGS__ }; \
  BrowserNodesA(__args); \
  })
#else
struct List *ChooserLabels(STRPTR arg1, ...);
struct List *ClickTabs(STRPTR arg1, ...);
struct List *BrowserNodes(STRPTR arg1, ...);
#endif

#endif /* _CA_SUPPORT_H_ */

