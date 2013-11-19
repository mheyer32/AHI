#ifdef __AMIGAOS4__
#define __USE_INLINE__ 1
#endif
#include "ca_support.h"
#include <gadgets/chooser.h>
#include <gadgets/listbrowser.h>
#include <gadgets/clicktab.h>
#include <proto/exec.h>
#include <proto/chooser.h>
#include <proto/listbrowser.h>
#include <proto/clicktab.h>

#ifdef __AMIGAOS4__
#define AllocList() AllocSysObject(ASOT_LIST, NULL)
#define FreeList(list) FreeSysObject(ASOT_LIST, list)
#else
static struct List *AllocList(void) {
  struct List *list;
  list = AllocMem(sizeof(*list), MEMF_PUBLIC|MEMF_CLEAR);
  if (list != NULL) {
    NEWLIST(list);
  }
  return list;
}

static void FreeList(struct List *list) {
  if (list != NULL) {
    FreeMem(list, sizeof(*list));
  }
}
#endif

struct List *ChooserLabelsA(STRPTR *array) {
	struct List *list;
	list = AllocList();
	if (list != NULL) {
		STRPTR label;
		struct Node *node;
		while ((label = *array++)) {
			node = AllocChooserNode(
				CNA_CopyText, TRUE,
				CNA_Text, label,
				TAG_END);
			if (node == NULL) {
				FreeChooserLabels(list);
				list = NULL;
				break;
			}
			AddTail(list, node);
		}
	}
	return list;
}

void FreeChooserLabels(struct List *list) {
	if (list != NULL) {
		struct Node *node;
		while ((node = RemHead(list))) {
			FreeChooserNode(node);
		}
		FreeList(list);
	}
}

struct List *BrowserNodesA(STRPTR *array) {
	struct List *list;
	list = AllocList();
	if (list != NULL) {
		STRPTR label;
		struct Node *node;
		while ((label = *array++)) {
			node = AllocListBrowserNode(1,
				LBNCA_CopyText, TRUE,
				LBNCA_Text, label,
				TAG_END);
			if (node == NULL) {
				FreeBrowserNodes(list);
				list = NULL;
				break;
			}
			AddTail(list, node);
		}
	}
	return list;
}

void FreeBrowserNodes(struct List *list) {
	if (list != NULL) {
		FreeListBrowserList(list);
		FreeList(list);
	}
}

struct List *ClickTabsA(STRPTR *array) {
	struct List *list;
	list = AllocList();
	if (list != NULL) {
		STRPTR label;
		struct Node *node;
		while ((label = *array++)) {
			node = AllocClickTabNode(
				TNA_Text, label,
				TAG_END);
			if (node == NULL) {
				FreeClickTabs(list);
				list = NULL;
				break;
			}
			AddTail(list, node);
		}
	}
	return list;
}

void FreeClickTabs(struct List *list) {
	if (list != NULL) {
		FreeClickTabList(list);
		FreeList(list);
	}
}

#ifndef __AMIGAOS4__
struct List *ChooserLabels(STRPTR arg1, ...) {
  return ChooserLabelsA(&arg1);
}

struct List *ClickTabs(STRPTR arg1, ...) {
  return ClickTabsA(&arg1);
}

struct List *BrowserNodes(STRPTR arg1, ...) {
  return BrowserNodesA(&arg1);
}
#endif

