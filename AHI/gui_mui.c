/* $Id$
 * $Log$
 * Revision 4.5  1997/07/11 14:25:49  lcs
 * Small bug fix: Wrong order of set() calls in guinewmode()
 *
 * Revision 4.4  1997/06/24 21:49:31  lcs
 * Fixed an enforcer hit, and a few more potential ones (problem caused by
 * having a 0-level slider (min 0, max -1...).
 *
 * Revision 4.3  1997/05/06 15:15:46  lcs
 * Can now which pages with the keyboard.
 * Fixed a bug in the mode properties code.
 *
 * Revision 4.2  1997/05/04 22:13:29  lcs
 * Keyboard shortcuts and more.
 *
 * Revision 4.1  1997/05/04 05:30:28  lcs
 * First MUI version.
 *
 *
 */

#include <exec/memory.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <math.h>
#include <string.h>

extern void kprintf(char *, ...);

#include "ahi.h"
#include "ahiprefs_Cat.h"

#include "ahi_protos.h"
#include "support_protos.h"

#ifndef _GENPROTO
#include "gui_protos.h"
#endif

extern APTR __asm SPrintfA(register __a3 STRPTR, register __a0 STRPTR, register __a1 LONG*);

static void GUINewSettings(void);
static void GUINewUnit(void);
static void GUINewMode(void);


enum windowIDs {
  WINID_MAIN=1,
  WINID_COUNT
};

enum actionIDs {
  ACTID_OPEN=1, ACTID_SAVEAS, ACTID_ABOUT, ACTID_QUIT,
  ACTID_DEFAULT, ACTID_LASTSAVED, ACTID_RESTORE,
  ACTID_ICONS,
  ACTID_HELP, ACTID_GUIDE, ACTID_HELPINDEX,
  ACTID_SAVE, ACTID_USE,

  ACTID_TABS, ACTID_PAGE,

  ACTID_UNIT, ACTID_MODE, 
  SHOWID_MODE,

  ACTID_FREQ, ACTID_CHANNELS, ACTID_OUTVOL, ACTID_MONVOL, ACTID_GAIN,
  ACTID_INPUT, ACTID_OUTPUT,
  SHOWID_FREQ, SHOWID_CHANNELS, SHOWID_OUTVOL, SHOWID_MONVOL, SHOWID_GAIN,
  SHOWID_INPUT, SHOWID_OUTPUT,

  ACTID_DEBUG, ACTID_SURROUND, ACTID_ECHO, ACTID_CLIPMV,
  ACTID_CPULIMIT, SHOWID_CPULIMIT,
  

  ACTID_COUNT
};

#define Title(t)        { NM_TITLE, t, NULL, 0, 0, NULL }
#define Item(t,s,i)     { NM_ITEM, t, s, 0, 0, (APTR)i }
#define ItemBar         { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL }
#define SubItem(t,s,i)  { NM_SUB, t, s, 0, 0, (APTR)i }
#define SubBar          { NM_SUB, NM_BARLABEL, NULL, 0, 0, NULL }
#define EndMenu         { NM_END, NULL, NULL, 0, 0, NULL }
#define ItCk(t,s,i,f)   { NM_ITEM, t, s, f, 0, (APTR)i }

struct Library       *MUIMasterBase  = NULL;

static struct NewMenu Menus[] = {
  Title( NULL /* Project */ ),
    Item( NULL /* Open... */,             NULL, ACTID_OPEN      ),
    Item( NULL /* Save As... */,          NULL, ACTID_SAVEAS    ),
    ItemBar,
    Item( NULL /* About... */,            NULL, ACTID_ABOUT     ),
    ItemBar,
    Item( NULL /* Quit */,                NULL, ACTID_QUIT      ),
  Title( NULL /* Edit */ ),
    Item( NULL /* Reset To Defaults */,   NULL, ACTID_DEFAULT   ),
    Item( NULL /* Last Saved */,          NULL, ACTID_LASTSAVED ),
    Item( NULL /* Restore */,             NULL, ACTID_RESTORE   ),
  Title( NULL /* Settings */ ),
    ItCk( NULL /* Create Icons? */,       NULL, ACTID_ICONS, CHECKIT|MENUTOGGLE ),
  Title( NULL /* Help */ ),
    ItCk( NULL /* Help... */,             NULL, ACTID_HELP, COMMSEQ),
    ItemBar,
    Item( NULL /* AHI User's guide... */, NULL, ACTID_GUIDE),
    Item( NULL /* Concept Index... */,    NULL, ACTID_HELPINDEX ),
  EndMenu
};

static char *PageNames[] =
{
  NULL,  /* Mode settings */
  NULL,  /* Advanced settings */
  NULL
};

static char * DebugLabels[] = {
  NULL,  /* None */
  NULL,  /* Low */
  NULL,  /* High */
  NULL,  /* Full */
  NULL
};

static char * EchoLabels[] = {
  NULL,  /* Enabled */
  NULL,  /* Fast */
  NULL,  /* Disabled */
  NULL
};

static char * SurroundLabels[] = {
  NULL,  /* Enabled */
  NULL,  /* Disabled */
  NULL
};

static char * ClipMVLabels[] = {
  NULL,  /* Without clipping */
  NULL,  /* With clipping */
  NULL
};

/***** Local function to update the strings above ****************************/

static void UpdateStrings(void) {
  char ** strings[] =
  {
    &msgMenuProject,
    &msgItemOpen,
    &msgItemSaveAs,
    &msgItemAbout,
    &msgItemQuit,
    &msgMenuEdit,
    &msgItemDefaults,
    &msgItemLastSaved,
    &msgItemRestore,
    &msgMenuSettings,
    &msgItemCreateIcons,
    &msgMenuHelp,
    &msgItemHelp,
    &msgItemUsersGuide,
    &msgItemConceptIndex
  };

  struct NewMenu   *menuptr;
  char           ***stringptr;
  
  menuptr   = (struct NewMenu *) &Menus;
  stringptr = (char ***) &strings;

  while(menuptr->nm_Type != NM_END)
  {
    if(menuptr->nm_Label == NULL)
    {
      if(strlen(**stringptr) != 0) {
        menuptr->nm_CommKey = **stringptr;
      }
      menuptr->nm_Label = **stringptr + strlen(**stringptr) + 1;
      stringptr++;
    }
    menuptr++;
  }


  PageNames[0] = (char *) msgPageMode;
  PageNames[1] = (char *) msgPageAdvanced;
  DebugLabels[0] = (char *) msgDebugNone;
  DebugLabels[1] = (char *) msgDebugLow;
  DebugLabels[2] = (char *) msgDebugHigh;
  DebugLabels[3] = (char *) msgDebugFull;
  EchoLabels[0] = (char *) msgEchoEnabled;
  EchoLabels[1] = (char *) msgEchoFast;
  EchoLabels[2] = (char *) msgEchoDisabled;
  SurroundLabels[0] = (char *) msgSurroundEnabled;
  SurroundLabels[1] = (char *) msgSurroundDisabled;
  ClipMVLabels[0] = (char *) msgMVNoClip;
  ClipMVLabels[1] = (char *) msgMVClip;

}


static Object *MUIWindow,*MUIList,*MUIInfos,*MUIUnit;
static Object *MUIFreq,*MUIChannels,*MUIOutvol,*MUIMonvol,*MUIGain,*MUIInput,*MUIOutput;
static Object *MUILFreq,*MUILChannels,*MUILOutvol,*MUILMonvol,*MUILGain,*MUILInput,*MUILOutput;
static Object *MUIDebug,*MUIEcho,*MUISurround,*MUIClipvol,*MUICpu;

LONG xget(Object * obj, ULONG attribute)
{
  LONG x = 0;

  get(obj, attribute, &x);
  return (x);
}

static void GUINewSettings(void)
{
  set(MUIUnit,MUIA_Cycle_Active,state.UnitSelected);
  set(MUIDebug, MUIA_Cycle_Active, globalprefs.ahigp_DebugLevel);
  set(MUIEcho, MUIA_Cycle_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0)|(globalprefs.ahigp_FastEcho    ? 1 : 0));
  set(MUISurround, MUIA_Cycle_Active, globalprefs.ahigp_DisableSurround);
  set(MUIClipvol, MUIA_Cycle_Active, globalprefs.ahigp_ClipMasterVolume);
  set(MUICpu, MUIA_Cycle_Active, (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16);
  GUINewUnit();
}

static void GUINewUnit(void)
{
  DoMethod(MUIList, MUIM_List_Clear);
  set(MUIList, MUIA_List_Quiet, TRUE);
  DoMethod(MUIList, MUIM_List_Insert, Modes, -1, MUIV_List_Insert_Bottom);
  set(MUIList, MUIA_List_Quiet, FALSE);
  set(MUIList, MUIA_List_Active, state.ModeSelected);
  GUINewMode();
}

static char *infoargs[6];

static void GUINewMode(void)
{
  int Max, Sel;
  STRPTR buffer;

  infoargs[0] = (char *) getAudioMode();
  infoargs[1] = getRecord();
  infoargs[2] = getAuthor();
  infoargs[3] = getCopyright();
  infoargs[4] = getDriver();
  infoargs[5] = getVersion();

  if(buffer = AllocVec(strlen(infoargs[1]) + strlen(infoargs[2]) +
                       strlen(infoargs[3]) + strlen(infoargs[4]) +
                       strlen(infoargs[5]) + 128, MEMF_ANY))
  {
    SPrintfA(buffer,"0x%08lx\n%s\n%s\n%s\nDevs:AHI/%s.audio\n%s", (APTR)infoargs);
    set(MUIInfos, MUIA_Text_Contents, buffer);
    FreeVec(buffer);
  }

  Max = max(state.Frequencies -1, 0);
  Sel = min(Max, state.FreqSelected);
  set(MUIFreq, MUIA_Disabled, Max==0);
  set(MUIFreq, MUIA_Numeric_Max, Max);
  set(MUIFreq, MUIA_Numeric_Value, Sel);
  set(MUILFreq, MUIA_Text_Contents, getFreq());

  Max = max(state.Channels, 0);
  Sel = min(Max, state.ChannelsSelected);
  set(MUIChannels, MUIA_Disabled, (Max == 1) || state.ChannelsDisabled);
  set(MUIChannels, MUIA_Numeric_Max, Max);
  set(MUIChannels, MUIA_Numeric_Value, Sel);
  set(MUILChannels, MUIA_Text_Contents, getChannels());

  Max = max(state.OutVols -1, 0);
  Sel = min(Max, state.OutVolSelected);
  set(MUIOutvol, MUIA_Disabled, Max==0);
  set(MUIOutvol, MUIA_Numeric_Max, Max);
  set(MUIOutvol, MUIA_Numeric_Value, Sel);
  set(MUILOutvol, MUIA_Text_Contents, getOutVol());

  Max = max(state.MonVols -1, 0);
  Sel = min(Max, state.MonVolSelected);
  set(MUIMonvol, MUIA_Disabled, Max==0);
  set(MUIMonvol, MUIA_Numeric_Max, Max);
  set(MUIMonvol, MUIA_Numeric_Value, Sel);
  set(MUILMonvol, MUIA_Text_Contents, getMonVol());

  Max = max(state.Gains -1, 0);
  Sel = min(Max, state.GainSelected);
  set(MUIGain, MUIA_Disabled, Max==0);
  set(MUIGain, MUIA_Numeric_Max, Max);
  set(MUIGain, MUIA_Numeric_Value, Sel);
  set(MUILGain, MUIA_Text_Contents, getGain());

  Max = max(state.Inputs -1, 0);
  Sel = min(Max, state.InputSelected);
  set(MUIInput, MUIA_Disabled, Max==0);
  set(MUIInput, MUIA_Numeric_Max, Max);
  set(MUIInput, MUIA_Numeric_Value, Sel);
  set(MUILInput, MUIA_Text_Contents, getInput());

  Max = max(state.Outputs -1, 0);
  Sel = min(Max, state.OutputSelected);
  set(MUIOutput, MUIA_Disabled, Max==0);
  set(MUIOutput, MUIA_Numeric_Max, Max);
  set(MUIOutput, MUIA_Numeric_Value, Sel);
  set(MUILOutput, MUIA_Text_Contents, getOutput());
}

static __saveds __asm VOID SliderHookFunc(register __a2 Object *obj, register __a1 ULONG** arg )
{
  if(obj == MUIFreq)
  {
    state.FreqSelected = (LONG) (*arg);
    set(MUILFreq,MUIA_Text_Contents,getFreq());
  }
  else if(obj == MUIChannels )
  {
    state.ChannelsSelected = (LONG) (*arg);
    set(MUILChannels,MUIA_Text_Contents,getChannels());
  }
  else if(obj == MUIOutvol )
  {
    state.OutVolSelected = (LONG) (*arg);
    set(MUILOutvol,MUIA_Text_Contents,getOutVol());
  }
  else if(obj == MUIMonvol )
  {
    state.MonVolSelected = (LONG) (*arg);
    set(MUILMonvol,MUIA_Text_Contents,getMonVol());
  }
  else if(obj == MUIGain )
  {
    state.GainSelected = (LONG) (*arg);
    set(MUILGain,MUIA_Text_Contents,getGain());
  }
  else if(obj == MUIInput )
  {
    state.InputSelected = (LONG) (*arg);
    set(MUILInput,MUIA_Text_Contents,getInput());
  }
  else if(obj == MUIOutput )
  {
  state.OutputSelected = (ULONG) (*arg);
  set(MUILOutput,MUIA_Text_Contents,getOutput());
  }
}

static struct Hook hookSlider = { NULL,NULL,(HOOKFUNC) SliderHookFunc,NULL,NULL };

/******************************************************************************
**** Call to open the window **************************************************
******************************************************************************/

static Object *MUIApp,*MUIMenu;

static Object* SpecialLabel(STRPTR label)
{
  return(TextObject,
      MUIA_HorizWeight, 75,
      MUIA_Text_Contents, label,
      MUIA_Text_PreParse, "\33l",
    End);
}

static Object* SpecialButton(STRPTR label)
{
  Object *button = NULL;
  STRPTR lab;
  
  if(lab = AllocVec(strlen(label)+1,0))
  {
    char ctrlchar = 0;
    STRPTR l = lab;

    while(*label)
    {
      *l = *label;
      if(*label++ == '_')
      {
        ctrlchar = ToLower(*label);
        *l = *label++;
      }
      l++;
    }
    *l = '\0';
    button = TextObject,
      MUIA_HorizWeight, 0,
      MUIA_Text_Contents, lab,
      MUIA_Text_PreParse, "\33r",
      MUIA_Text_HiChar, ctrlchar,
      MUIA_ControlChar, ctrlchar,
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_ShowSelState, FALSE,
    End;
    FreeVec(lab);
  }
  return button;
}

static Object* SpecialSlider(LONG min, LONG max, LONG value)
{
  return(SliderObject,
      MUIA_CycleChain, 1,
      MUIA_Slider_Quiet, TRUE,
      MUIA_Numeric_Min, min,
      MUIA_Numeric_Max,max,
      MUIA_Numeric_Value,value,
      MUIA_Numeric_Format, "",
    End);
}

BOOL BuildGUI(char *screenname)
{
  Object *MUISave, *MUIUse, *MUICancel;
  Object *page1,*page2;
  Object *MUITFreq,*MUITChannels,*MUITOutvol,*MUITMonvol,*MUITGain,*MUITInput,*MUITOutput,*MUITDebug,*MUITEcho,*MUITSurround,*MUITClipvol,*MUITCpu;

  UpdateStrings();

  MUIMasterBase = (void *)OpenLibrary("muimaster.library", MUIMASTER_VLATEST);
  if(MUIMasterBase == NULL)
  {
    Printf((char *) msgTextNoOpen, "muimaster.library", MUIMASTER_VLATEST);
    Printf("\n");
    return FALSE;
  }

  page1 = HGroup,
    Child, VGroup,
      Child, MUIUnit = CycleObject,
        MUIA_CycleChain, 1,
        MUIA_Cycle_Entries, Units,
        MUIA_Cycle_Active, state.UnitSelected,
      End,
      Child, ListviewObject,
        MUIA_CycleChain, 1,
        MUIA_Listview_List, MUIList = ListObject,
          InputListFrame,
          MUIA_List_AutoVisible, TRUE,
        End,
      End,
      Child, HGroup,
        ReadListFrame,
        MUIA_Background, MUII_TextBack,
        Child, TextObject,
          MUIA_Text_Contents, msgProperties,
          MUIA_Text_SetMax, TRUE,
        End,
        Child, MUIInfos = TextObject,
          MUIA_Text_Contents, "\n\n\n\n\n",
          MUIA_Text_SetMin, FALSE,
        End,
      End,
    End,
    Child, BalanceObject,
    End,
    Child, VGroup,
      Child, HVSpace,
      Child, ColGroup(3),
        GroupFrameT(msgOptions),
        Child, MUITFreq = SpecialButton((STRPTR)msgOptFrequency),
        Child, MUIFreq = SpecialSlider(0,max(state.Frequencies-1,0),state.FreqSelected),
        Child, MUILFreq = SpecialLabel(getFreq()),
        Child, MUITChannels = SpecialButton((STRPTR)msgOptChannels),
        Child, MUIChannels = SpecialSlider(1,state.Channels,state.ChannelsSelected),
        Child, MUILChannels = SpecialLabel(getChannels()),
        Child, MUITOutvol = SpecialButton((STRPTR)msgOptVolume),
        Child, MUIOutvol = SpecialSlider(0,max(state.OutVols-1,0),state.OutVolSelected),
        Child, MUILOutvol = SpecialLabel(getOutVol()),
        Child, MUITMonvol = SpecialButton((STRPTR)msgOptMonitor),
        Child, MUIMonvol = SpecialSlider(0,max(state.MonVols-1,1),state.MonVolSelected),
        Child, MUILMonvol = SpecialLabel(getMonVol()),
        Child, MUITGain = SpecialButton((STRPTR)msgOptGain),
        Child, MUIGain = SpecialSlider(0,max(state.Gains-1,0),state.GainSelected),
        Child, MUILGain = SpecialLabel(getGain()),
        Child, MUITInput = SpecialButton((STRPTR)msgOptInput),
        Child, MUIInput = SpecialSlider(0,max(state.Inputs-1,0),state.InputSelected),
        Child, MUILInput = SpecialLabel(getInput()),
        Child, MUITOutput = SpecialButton((STRPTR)msgOptOutput),
        Child, MUIOutput = SpecialSlider(0,max(state.Outputs-1,0),state.OutputSelected),
        Child, MUILOutput = SpecialLabel(getOutput()),
      End,
      Child, HVSpace,
    End,
  End;

  page2 = VGroup,
    Child, HVSpace,
    Child, HGroup,
      Child, HVSpace,
      Child, ColGroup(2),
        GroupFrameT(msgGlobalOptions),
        Child, MUITDebug = SpecialButton((STRPTR)msgGlobOptDebugLevel),
        Child, MUIDebug = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, DebugLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_DebugLevel,
        End,
        Child, MUITEcho = SpecialButton((STRPTR)msgGlobOptEcho),
        Child, MUIEcho  = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, EchoLabels,
          MUIA_Cycle_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0) | (globalprefs.ahigp_FastEcho ? 1 : 0),
        End,
        Child, MUITSurround = SpecialButton((STRPTR)msgGlobOptSurround),
        Child, MUISurround = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, SurroundLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_DisableSurround,
        End,
        Child, MUITClipvol = SpecialButton((STRPTR)msgGlobOptMasterVol),
        Child, MUIClipvol = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, ClipMVLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_ClipMasterVolume,
        End,
        Child, MUITCpu = SpecialButton((STRPTR)msgGlobOptCPULimit),
        Child, MUICpu = SliderObject,
          MUIA_CycleChain, 1,
          MUIA_Slider_Horiz, TRUE,
          MUIA_Numeric_Min, 0,
          MUIA_Numeric_Max, 100,
          MUIA_Numeric_Value,(globalprefs.ahigp_MaxCPU * 100 + 32768) / 65536,
          MUIA_Numeric_Format,"%ld%%",
        End,
      End,
      Child, HVSpace,
    End,
    Child, HVSpace,
  End;

  if(MUIApp = ApplicationObject,
    MUIA_Application_Title, (char *) msgTextProgramName,
    MUIA_Application_Version, Version,
    MUIA_Application_Copyright, "©1997 Martin Blom",
    MUIA_Application_Author, "Stéphane Barbaray/Martin Blom",
    MUIA_Application_Base, "AHI",
    MUIA_Application_HelpFile, HELPFILE,
    MUIA_Application_Menustrip, MUIMenu = MUI_MakeObject(MUIO_MenustripNM,Menus,0),
    MUIA_Application_SingleTask, TRUE,
    SubWindow, MUIWindow = WindowObject,
      MUIA_Window_Title, (char *) msgTextProgramName,
      MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
      MUIA_HelpNode, "AHI",
      WindowContents, VGroup,
        Child, RegisterGroup(PageNames),
          MUIA_CycleChain, 1,
          Child, page1,
          Child, page2,
        End,
        Child, HGroup,
          Child, MUISave = SimpleButton(msgButtonSave),
          Child, MUIUse = SimpleButton(msgButtonUse),
          Child, MUICancel = SimpleButton(msgButtonCancel),
        End,
      End,
    End,
  End)
  {
    APTR item = (APTR)DoMethod(MUIMenu,MUIM_FindUData,ACTID_ICONS);
    if(item)
    {
      set(item, MUIA_Menuitem_Checked, SaveIcons);
    }
    DoMethod(MUIWindow, MUIM_MultiSet, MUIA_Text_PreParse,"\033l",MUILFreq,MUILChannels,MUILOutvol,MUILMonvol,MUILGain,MUILInput,MUILOutput,NULL);
    DoMethod(MUIWindow, MUIM_MultiSet, MUIA_CycleChain, 1, MUISave,MUIUse,MUICancel,NULL);

    DoMethod(MUITFreq, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIFreq);
    DoMethod(MUITChannels, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIChannels);
    DoMethod(MUITOutvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIOutvol);
    DoMethod(MUITMonvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIMonvol);
    DoMethod(MUITGain, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIGain);
    DoMethod(MUITInput, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIInput);
    DoMethod(MUITOutput, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIOutput);
    DoMethod(MUITDebug, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIDebug);
    DoMethod(MUITEcho, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIEcho);
    DoMethod(MUITSurround, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUISurround);
    DoMethod(MUITClipvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIClipvol);
    DoMethod(MUITCpu, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUICpu);

    DoMethod(MUIWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIApp, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(MUISave, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, ACTID_SAVE);
    DoMethod(MUIUse, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, ACTID_USE);
    DoMethod(MUICancel, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(MUIUnit, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID, ACTID_UNIT);
    DoMethod(MUIList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID, ACTID_MODE);
    DoMethod(MUIDebug, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_DEBUG);
    DoMethod(MUISurround, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_SURROUND);
    DoMethod(MUIEcho, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_ECHO);
    DoMethod(MUICpu, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_CPULIMIT);
    DoMethod(MUIClipvol, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_CLIPMV);
    DoMethod(MUIFreq, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIChannels, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIOutvol, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIMonvol, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIGain, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIInput, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIOutput, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    set(MUIWindow, MUIA_Window_Open, TRUE);
    GUINewUnit();
    return TRUE;
  }
  return FALSE;
}

/******************************************************************************
**** Call to close the window *************************************************
******************************************************************************/

void CloseGUI(void)
{
  if (MUIApp)
    MUI_DisposeObject(MUIApp);
  if (MUIMasterBase)
    CloseLibrary(MUIMasterBase);
}


/******************************************************************************
**** Handles the input events *************************************************
******************************************************************************/

void EventLoop(void)
{
  while (1)
  {
    ULONG sigs = 0UL;

    switch(DoMethod(MUIApp, MUIM_Application_NewInput, &sigs))
    {

      case MUIV_Application_ReturnID_Quit:
        return;

      case ACTID_OPEN:
      {
        struct FileRequester *request;

        if (request = MUI_AllocAslRequestTags(ASL_FileRequest, ASLFR_Window, xget(MUIWindow, MUIA_Window_Window), ASLFR_TitleText, msgTextProgramName, ASLFR_RejectIcons, TRUE, ASLFR_InitialDrawer, "SYS:Prefs/Presets", TAG_DONE))
        {
          DoMethod(MUIApp, MUIA_Application_Sleep, TRUE);
          if (MUI_AslRequest(request, NULL))
          {
            char *file;

            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
            if(file = AllocVec(strlen(request->fr_Drawer)+128,0))
            {
              CopyMem(request->fr_Drawer, file, strlen(request->fr_Drawer)+1);
              AddPart(file, request->fr_File, 128);
              NewSettings(file);
              GUINewSettings();
              FreeVec(file);
            }
          }
          else
          {
            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
          }
          MUI_FreeAslRequest(request);
        }
        break;
      }

      case ACTID_SAVEAS:
      {
        struct FileRequester *request;

        if (request = MUI_AllocAslRequestTags(ASL_FileRequest, ASLFR_Window, xget(MUIWindow, MUIA_Window_Window), ASLFR_TitleText, msgTextProgramName, ASLFR_RejectIcons, TRUE, ASLFR_DoSaveMode, TRUE,ASLFR_InitialDrawer, "SYS:Prefs/Presets", TAG_DONE))
        {
          DoMethod(MUIApp, MUIA_Application_Sleep, TRUE);
          FillUnit();
          if (MUI_AslRequest(request, NULL))
          {
            char *file;

            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
            if(file = AllocVec(strlen(request->fr_Drawer)+128,0))
            {
              CopyMem(request->fr_Drawer, file, strlen(request->fr_Drawer)+1);
              AddPart(file, request->fr_File, 128);
              SaveSettings(file, UnitList);
              if(SaveIcons)
              {
                WriteIcon(file);
              }
              FreeVec(file);
            }
          }
          else
          {
            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
          }
          MUI_FreeAslRequest(request);
        }
        break;
      }

      case ACTID_ABOUT:
        MUI_Request(MUIApp, MUIWindow, 0, (char *) msgTextProgramName,
            (char*)msgButtonOK, (char*)msgTextCopyright, "\033c",
            (char*)msgTextProgramName, "1997 Stéphane Barbaray/Martin Blom");
        break;

      case ACTID_SAVE:
        FillUnit();
        SaveSettings(ENVFILE, UnitList);
        SaveSettings(ENVARCFILE, UnitList);
        return;

      case ACTID_USE:
        FillUnit();
        SaveSettings(ENVFILE, UnitList);
        return;

      case ACTID_QUIT:
        return;

      case ACTID_DEFAULT:
        NewSettings(NULL);
        GUINewSettings();
        break;

      case ACTID_LASTSAVED:
        NewSettings(ENVARCFILE);
        GUINewSettings();
        break;

      case ACTID_RESTORE:
        NewSettings(args.from);
        GUINewSettings();
        break;

      case ACTID_ICONS:
      {
        APTR item = (APTR)DoMethod(MUIMenu,MUIM_FindUData,ACTID_ICONS);

        if(item)
        {
          if(xget(item, MUIA_Menuitem_Checked))
          {
            SaveIcons = TRUE;
          }
          else
          {
            SaveIcons = FALSE;
          }
        }
        break;
      }

      case ACTID_HELP:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"AHI",0);
        break;

      case ACTID_GUIDE:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"MAIN",0);
        break;

      case ACTID_HELPINDEX:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"Concept Index",0);
        break;

      case ACTID_UNIT:
        FillUnit();
        NewUnit(xget(MUIUnit,MUIA_Cycle_Active));
        GUINewUnit();
        break;

      case ACTID_MODE:
        FillUnit();
        NewMode(xget(MUIList, MUIA_List_Active));
        GUINewMode();
        break;

      case ACTID_DEBUG:
      case ACTID_SURROUND:
      case ACTID_ECHO:
      case ACTID_CPULIMIT:
      case ACTID_CLIPMV:
      {
        ULONG debug = AHI_DEBUG_NONE, surround = FALSE, echo = 0, cpu = 90;
        ULONG clip = FALSE;

        get(MUIDebug, MUIA_Cycle_Active, &debug);
        get(MUISurround, MUIA_Cycle_Active, &surround);
        get(MUIEcho, MUIA_Cycle_Active, &echo);
        get(MUIClipvol, MUIA_Cycle_Active, &clip);
        get(MUICpu, MUIA_Numeric_Value, &cpu);

        globalprefs.ahigp_DebugLevel      = debug;
        globalprefs.ahigp_DisableSurround = surround;
        globalprefs.ahigp_DisableEcho     = (echo == 2);
        globalprefs.ahigp_FastEcho        = (echo == 1);
        globalprefs.ahigp_MaxCPU = (cpu << 16) / 100;
        globalprefs.ahigp_ClipMasterVolume= clip;

        break;
      }

      default:
        if (sigs)
        {
          sigs = Wait(sigs | SIGBREAKF_CTRL_C);
          if (sigs & SIGBREAKF_CTRL_C)
          break;
        }
        break;
    }
  }
}

