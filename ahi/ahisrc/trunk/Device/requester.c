/* $Id$
* $Log$
* Revision 4.4  1997/06/21 18:13:43  lcs
* ahiam_AudioID and ahiam_MixFreq are now left unchanged if the user
* cancels the audio mode requester.
*
* Changed BOOL return values to ULONG in ordet to set all 32 bits.
*
* Revision 4.3  1997/05/28 20:35:18  lcs
* AudioID and MixFreq is not changed if the user cancels the requester
* anymore.
*
* Revision 4.2  1997/04/14 01:50:39  lcs
* Spellchecked
*
* Revision 4.1  1997/04/02 22:28:11  lcs
* Bumped to version 4
*
* Revision 1.16  1997/03/25 22:27:49  lcs
* Tried to get AHIST_INPUT to work, but I cannot get it synced! :(
*
* Revision 1.15  1997/03/08 18:12:56  lcs
* The freqgadget wasn't ghosted if the requester was opened
* with audioid set to AHI_DEFAULT_ID. Now it is
*
* Revision 1.14  1997/02/15 14:02:02  lcs
* All functions that take an audio mode id as input can now use
* AHI_DEFAULT_ID as well.
*
* Revision 1.13  1997/02/14 18:43:31  lcs
* AHIR_DoDefaultMode added
*
* Revision 1.12  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.11  1997/02/10 10:36:48  lcs
* Really, I fixed it! Trust me.
*
* Revision 1.10  1997/02/10 10:32:28  lcs
* Fixed a bug in the mode list building code (unterminated taglist)
*
* Revision 1.9  1997/02/10 02:23:06  lcs
* Infowindow in the requester added.
*
* Revision 1.8  1997/02/09 18:12:23  lcs
* Default audio mode added? Don't remember...
*
* Revision 1.7  1997/02/04 22:14:27  lcs
* The users preffered audio mode can now be selected in the requester
*
* Revision 1.6  1997/02/03 16:21:23  lcs
* AHIR_Locale should work now
*
* Revision 1.5  1997/02/02 22:35:50  lcs
* Localized it
*
* Revision 1.3  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.2  1996/12/21 23:08:47  lcs
* Replaced all EQ with ==
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

#include "ahi_def.h"

#include <math.h>
#include <exec/memory.h>
#include <graphics/rpattr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>

#include "localize.h"

#include <string.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#ifndef  noprotos
#ifndef _GENPROTO
#include "requester_protos.h"
#endif

#include "cfuncs_protos.h"
#endif

static void OpenInfoWindow( struct AHIAudioModeRequesterExt * );
static void CloseInfoWindow( struct AHIAudioModeRequesterExt * );
static void UpdateInfoWindow( struct AHIAudioModeRequesterExt * );


/******************************************************************************
** Audio mode requester  ******************************************************
******************************************************************************/

#define MY_IDCMPS (LISTVIEWIDCMP|SLIDERIDCMP|BUTTONIDCMP|IDCMP_SIZEVERIFY|IDCMP_NEWSIZE|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_MENUPICK|IDCMP_VANILLAKEY|IDCMP_RAWKEY)
#define MY_INFOIDCMPS (LISTVIEWIDCMP|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW)

#define haveIDCMP   0x0001
#define lockwin     0x0002
#define freqgad     0x0004
#define ownIDCMP    0x0008
#define defaultmode 0x0010

static struct TagItem reqboolmap[] =
{
  {AHIR_PrivateIDCMP,   haveIDCMP},
  {AHIR_SleepWindow,    lockwin},
  {AHIR_DoMixFreq,      freqgad},
  {AHIR_DoDefaultMode,  defaultmode},
  {TAG_DONE, }
};

#define LASTMODEITEM  1
#define NEXTMODEITEM  2
#define PROPERTYITEM  3
#define RESTOREITEM   4
#define OKITEM        5
#define CANCELITEM    6

/* Node for audio mode requester */

struct IDnode
{
  struct Node node;
  ULONG       ID;
  char        name[80];
};

/* The attribues list */

#define ATTRNODES 6

struct Attrnode
{
  struct Node node;
  UBYTE       text[80];
};

 /* AHIAudioModeRequester extension */
struct AHIAudioModeRequesterExt
{
  struct AHIAudioModeRequester  Req;
  ULONG                         tempAudioID;
  ULONG                         tempFrequency;
  struct Window                *SrcWindow;
  STRPTR                        PubScreenName;
  struct Screen                *Screen;
  ULONG                         Flags;
  struct Hook                  *IntuiMsgFunc;
  struct TextAttr              *TextAttr;
  struct Locale                *Locale;
  STRPTR                        TitleText;
  STRPTR                        PositiveText;
  STRPTR                        NegativeText;
  struct TagItem               *FilterTags;
  struct Hook                  *FilterFunc;

//  struct Screen                *PubScreen;
  struct Window                *Window;
  struct Window                *InfoWindow;
  WORD                          gx,gy,gw,gh;
  APTR                          vi;
  struct Gadget                *Gadgets;
  struct Gadget                *InfoGadgets;
  struct Gadget                *InfoListViewGadget;
  struct Gadget                *listviewgadget;
  struct Gadget                *slidergadget;
  struct MinList               *list;
  struct MinList                InfoList;
  struct Menu                  *Menu;
  struct Catalog               *Catalog;
  struct Attrnode               AttrNodes[ATTRNODES];
};


static struct TextAttr Topaz80 = { "topaz.font", 8, 0, 0, };

#define MINSLIDERWIDTH 40

#define OKBUTTON      1
#define CANCELBUTTON  2
#define FREQSLIDER    3
#define LISTVIEW      4

#define FREQTEXT2     "%lu Hz"
#define FREQLEN2      (5+3) // 5 digits + space + "Hz"

static __stdargs __saveds LONG IndexToFrequency( struct Gadget *gad, WORD level)
{
  LONG  freq = 0;
  ULONG id   = ((struct AHIAudioModeRequesterExt *)gad->UserData)->tempAudioID;
  if(id != AHI_DEFAULT_ID)
  {
    AHI_GetAudioAttrs(id, NULL,
      AHIDB_FrequencyArg,level,
      AHIDB_Frequency,&freq,
      TAG_DONE);
  }
  else
  {
    freq = AHIBase->ahib_Frequency;
  }
  return freq;
}

static void FillReqStruct(struct AHIAudioModeRequesterExt *req, struct TagItem *tags)
{
// Check all known tags
  req->SrcWindow=(struct Window *)GetTagData(AHIR_Window,(ULONG)req->SrcWindow,tags);
  req->PubScreenName=(STRPTR)GetTagData(AHIR_PubScreenName,(ULONG)req->PubScreenName,tags);
  req->Screen=(struct Screen *)GetTagData(AHIR_Screen,(ULONG)req->Screen,tags);
  req->IntuiMsgFunc=(struct Hook *)GetTagData(AHIR_IntuiMsgFunc,(ULONG)req->IntuiMsgFunc,tags);
  req->Req.ahiam_UserData=(void *)GetTagData(AHIR_UserData,(ULONG)req->Req.ahiam_UserData,tags);
  req->TextAttr=(struct TextAttr *)GetTagData(AHIR_TextAttr,(ULONG)req->TextAttr,tags);
  req->Locale=(struct Locale *)GetTagData(AHIR_Locale,(ULONG)req->Locale,tags);
  req->TitleText=(STRPTR)GetTagData(AHIR_TitleText,(ULONG)req->TitleText,tags);
  req->PositiveText=(STRPTR)GetTagData(AHIR_PositiveText,(ULONG)req->PositiveText,tags);
  req->NegativeText=(STRPTR)GetTagData(AHIR_NegativeText,(ULONG)req->NegativeText,tags);
  req->Req.ahiam_LeftEdge=GetTagData(AHIR_InitialLeftEdge,req->Req.ahiam_LeftEdge,tags);
  req->Req.ahiam_TopEdge=GetTagData(AHIR_InitialTopEdge,req->Req.ahiam_TopEdge,tags);
  req->Req.ahiam_Width=GetTagData(AHIR_InitialWidth,req->Req.ahiam_Width,tags);
  req->Req.ahiam_Height=GetTagData(AHIR_InitialHeight,req->Req.ahiam_Height,tags);
  req->Req.ahiam_AudioID=GetTagData(AHIR_InitialAudioID,req->Req.ahiam_AudioID,tags);
  req->Req.ahiam_MixFreq=GetTagData(AHIR_InitialMixFreq,req->Req.ahiam_MixFreq,tags);
  req->Req.ahiam_InfoOpened=GetTagData(AHIR_InitialInfoOpened,req->Req.ahiam_InfoOpened,tags);
  req->Req.ahiam_InfoLeftEdge=GetTagData(AHIR_InitialInfoLeftEdge,req->Req.ahiam_InfoLeftEdge,tags);
  req->Req.ahiam_InfoTopEdge=GetTagData(AHIR_InitialInfoTopEdge,req->Req.ahiam_InfoTopEdge,tags);
//  req->Req.ahiam_InfoWidth=GetTagData(AHIR_InitialInfoWidth,req->Req.ahiam_InfoWidth,tags);
//  req->Req.ahiam_InfoHeight=GetTagData(AHIR_InitialInfoHeight,req->Req.ahiam_InfoHeight,tags);
  req->FilterTags=(struct TagItem *)GetTagData(AHIR_FilterTags,(ULONG)req->FilterTags,tags);
  req->FilterFunc=(struct Hook *)GetTagData(AHIR_FilterFunc,(ULONG)req->FilterFunc,tags);
  req->Flags=PackBoolTags(req->Flags,tags,reqboolmap);
}

/*
** Returns the ordinal number of the current audio id.
*/

static LONG GetSelected(struct AHIAudioModeRequesterExt *req)
{
  struct IDnode *idnode;
  LONG valt=0;

  for(idnode=(struct IDnode *)req->list->mlh_Head;
      idnode->node.ln_Succ;
      idnode=(struct IDnode *) idnode->node.ln_Succ)
  {
    if(idnode->ID == req->tempAudioID)
      break;
    else
      valt++;
  }
  if(idnode->node.ln_Succ == NULL)
  {
    valt=~0;
    req->tempAudioID=AHI_INVALID_ID;    // Crashed if this is not done! FIXIT!
  }
  return valt;
}

/*
** Calculates what the current slider level shoud be and how many levels total
*/

static void GetSliderAttrs(struct AHIAudioModeRequesterExt *req, LONG *levels, LONG *level)
{
  *levels=0;
  *level=0;
  
  AHI_GetAudioAttrs(req->tempAudioID, NULL,
      AHIDB_Frequencies,  levels,
      AHIDB_IndexArg,     (req->tempAudioID == AHI_DEFAULT_ID ? 
                              AHIBase->ahib_Frequency : req->tempFrequency),
      AHIDB_Index,        level,
      TAG_DONE);

  if(*level >= *levels)
    *level = *levels-1;

  AHI_GetAudioAttrs(req->tempAudioID, NULL,
      AHIDB_FrequencyArg, *level,
      AHIDB_Frequency,    &req->tempFrequency,
      TAG_DONE);
}

/*
** Updates the requester to the current frequency and if 'all'==TRUE, audio mode.
*/

static void SetSelected(struct AHIAudioModeRequesterExt *req, BOOL all)
{
  LONG sliderlevels,sliderlevel,selected;

  if(all)
  {
    //Set listview
    selected=GetSelected(req);
      GT_SetGadgetAttrs(req->listviewgadget, req->Window, NULL,
          ((selected == ~0) || (GadToolsBase->lib_Version >= 39) ? TAG_IGNORE : GTLV_Top),selected,
          (selected == ~0 ? TAG_IGNORE : GTLV_MakeVisible),selected,
          GTLV_Selected,selected,
          TAG_DONE);
  }

  //Set slider
  GetSliderAttrs(req,&sliderlevels,&sliderlevel);
  GT_SetGadgetAttrs(req->slidergadget, req->Window, NULL,
      GTSL_Max,sliderlevels-1,
      GTSL_Level, sliderlevel,
      GA_Disabled,!sliderlevels || (req->tempAudioID == AHI_DEFAULT_ID),
      TAG_DONE);

  UpdateInfoWindow(req);
}


/*
** Positions all gadgets in the requester.
*/

static BOOL LayOutReq (struct AHIAudioModeRequesterExt *req, struct TextAttr *TextAttr)
{
  struct Gadget *gad;
  struct NewGadget ng;

  struct TextAttr *gadtextattr;
  struct TextFont *font;
  LONG   fontwidth,buttonheight,buttonwidth,pixels;
  struct IntuiText intuitext = {1,0,JAM1,0,0,NULL,NULL,NULL};
  LONG  sliderlevels,sliderlevel;
  ULONG  selected;

  selected=GetSelected(req);
  GetSliderAttrs(req,&sliderlevels,&sliderlevel);

// Calculate gadget area
  req->gx=req->Window->BorderLeft+4;
  req->gy=req->Window->BorderTop+2;
  req->gw=req->Window->Width-req->gx-(req->Window->BorderRight+4);
  req->gh=req->Window->Height-req->gy-(req->Window->BorderBottom+2);

  if(req->Gadgets)
  {
    RemoveGList(req->Window,req->Gadgets,-1);
    FreeGadgets(req->Gadgets);
    SetAPen(req->Window->RPort,0);
    SetDrMd(req->Window->RPort,JAM1);
    EraseRect(req->Window->RPort, req->Window->BorderLeft, req->Window->BorderTop,
        req->Window->Width-req->Window->BorderRight-1,req->Window->Height-req->Window->BorderBottom-1);
    RefreshWindowFrame(req->Window);
  }
  req->Gadgets=NULL;
  if(gad=CreateContext(&req->Gadgets))
  {
    if(TextAttr)
      gadtextattr=TextAttr;
    else
      gadtextattr=req->Window->WScreen->Font;

    if(font=OpenFont(gadtextattr))
    {
      fontwidth=font->tf_XSize;
      CloseFont(font);
    }
    else
      return FALSE;

    buttonheight=gadtextattr->ta_YSize+6;
    intuitext.ITextFont=gadtextattr;
    intuitext.IText=req->PositiveText;
    buttonwidth=IntuiTextLength(&intuitext);
    intuitext.IText=req->NegativeText;
    pixels=IntuiTextLength(&intuitext);
    buttonwidth=max(pixels,buttonwidth);
    buttonwidth+=4+fontwidth;

// Create gadgets and check if they fit
    // Do the two buttons fit?
    if(2*buttonwidth > req->gw)
      return FALSE;
    ng.ng_TextAttr=gadtextattr;
    ng.ng_VisualInfo=req->vi;
    ng.ng_UserData=req;
// OK button
    ng.ng_LeftEdge=req->gx;
    ng.ng_TopEdge=req->gy+req->gh-buttonheight;
    ng.ng_Width=buttonwidth;
    ng.ng_Height=buttonheight;
    ng.ng_GadgetText=req->PositiveText;
    ng.ng_GadgetID=OKBUTTON;
    ng.ng_Flags=PLACETEXT_IN;
    gad=CreateGadget(BUTTON_KIND,gad,&ng,TAG_END);
// Cancel button
    ng.ng_LeftEdge=req->gx+req->gw-ng.ng_Width;
    ng.ng_GadgetText=req->NegativeText;
    ng.ng_GadgetID=CANCELBUTTON;
    gad=CreateGadget(BUTTON_KIND,gad,&ng,TAG_END);
// Frequency
    if(req->Flags & freqgad)
    {
      intuitext.IText = GetString(msgReqFrequency, req->Catalog);
      pixels=IntuiTextLength(&intuitext)+INTERWIDTH;
      if(pixels+MINSLIDERWIDTH+INTERWIDTH+FREQLEN2*fontwidth > req->gw)
        return FALSE;
      ng.ng_Width=req->gw-pixels-INTERWIDTH-FREQLEN2*fontwidth;
      ng.ng_LeftEdge=req->gx+pixels;
      ng.ng_TopEdge-=2+buttonheight;
      ng.ng_GadgetText = GetString(msgReqFrequency, req->Catalog);
      ng.ng_GadgetID=FREQSLIDER;
      ng.ng_Flags=PLACETEXT_LEFT;
      gad=CreateGadget(SLIDER_KIND,gad,&ng,
          GTSL_Min,0,
          GTSL_Max,sliderlevels-1,
          GTSL_Level,sliderlevel,
          GTSL_LevelFormat,FREQTEXT2,
          GTSL_MaxLevelLen,FREQLEN2,
          GTSL_LevelPlace,PLACETEXT_RIGHT,
          GTSL_DispFunc, IndexToFrequency,
          GA_RelVerify,TRUE,
          GA_Disabled,!sliderlevels || (req->tempAudioID == AHI_DEFAULT_ID),
          TAG_DONE);
      req->slidergadget=gad;   // Save for HadleReq()...
    }
// ListView
    if((ng.ng_Height=ng.ng_TopEdge-2-req->gy) < buttonheight)
      return FALSE;
    ng.ng_LeftEdge=req->gx;
    ng.ng_TopEdge=req->gy;
    ng.ng_Width=req->gw;
    ng.ng_GadgetText=NULL,
    ng.ng_GadgetID=LISTVIEW;
    ng.ng_Flags=PLACETEXT_ABOVE;
    gad=CreateGadget(LISTVIEW_KIND,gad,&ng,
        GTLV_ScrollWidth,(fontwidth>8 ? fontwidth*2 : 18),
        GTLV_Labels,(struct List *) req->list,
        GTLV_ShowSelected,NULL,
        ((selected == ~0) || (GadToolsBase->lib_Version >= 39) ? TAG_IGNORE : GTLV_Top),selected,
        (selected == ~0 ? TAG_IGNORE : GTLV_MakeVisible),selected,
        GTLV_Selected,selected,
        TAG_DONE);
    req->listviewgadget=gad;   // Save for HadleReq()...

    if(!gad)
      return FALSE;
  }
  else
    return FALSE;

  AddGList(req->Window,req->Gadgets,~0,-1,NULL);
  RefreshGList(req->Gadgets,req->Window,NULL,-1);
  GT_RefreshWindow(req->Window,NULL);

  return TRUE;
}



/* these functions close an Intuition window
 * that shares a port with other Intuition
 * windows or IPC customers.
 *
 * We are careful to set the UserPort to
 * null before closing, and to free
 * any messages that it might have been
 * sent.
 */

/* remove and reply all IntuiMessages on a port that
 * have been sent to a particular window
 * (note that we don't rely on the ln_Succ pointer
 *  of a message after we have replied it)
 */
static void StripIntuiMessages( struct MsgPort *mp, struct Window *win )
{
    struct IntuiMessage *msg;
    struct Node *succ;

    msg = (struct IntuiMessage *) mp->mp_MsgList.lh_Head;

    while( succ =  msg->ExecMessage.mn_Node.ln_Succ ) {

        if( msg->IDCMPWindow ==  win ) {

            /* Intuition is about to free this message.
             * Make sure that we have politely sent it back.
             */
            Remove( (struct Node *) msg );

            ReplyMsg( (struct Message *) msg );
        }
            
        msg = (struct IntuiMessage *) succ;
    }
}

static void CloseWindowSafely( struct Window *win )
{
    /* we forbid here to keep out of race conditions with Intuition */
    Forbid();

    /* send back any messages for this window 
     * that have not yet been processed
     */
    StripIntuiMessages( win->UserPort, win );

    /* clear UserPort so Intuition will not free it */
    win->UserPort = NULL;

    /* tell Intuition to stop sending more messages */
    ModifyIDCMP( win, 0L );

    /* turn multitasking back on */
    Permit();

    /* and really close the window */
    CloseWindow( win );
}

static BOOL HandleReq( struct AHIAudioModeRequesterExt *req )

// Returns FALSE if requester was cancelled

{
  BOOL done=FALSE,rc=TRUE;
  ULONG class,sec,oldsec=0,micro,oldmicro=0,oldid=AHI_INVALID_ID;
  UWORD code;
  UWORD qual;
  struct Gadget *pgsel;
  struct IntuiMessage *imsg;
  struct IDnode *idnode;
  LONG   sliderlevels,sliderlevel,i,selected;
  struct MenuItem *item;

  while(!done)
  {
    Wait(1L << req->Window->UserPort->mp_SigBit);

    while ((imsg=GT_GetIMsg(req->Window->UserPort)) != NULL )
    {

      if(imsg->IDCMPWindow == req->InfoWindow)
      {
        class = imsg->Class;
        GT_ReplyIMsg(imsg);

        switch(class)
        {
        case IDCMP_CLOSEWINDOW:
          CloseInfoWindow(req);
          break;
        case IDCMP_REFRESHWINDOW :
          GT_BeginRefresh(req->InfoWindow);
          GT_EndRefresh(req->InfoWindow,TRUE);
          break;
        }
        continue; // Get next IntuiMessage
      }

      else if(imsg->IDCMPWindow != req->Window) // Not my window!
      {
        if(req->IntuiMsgFunc)
          CallHookPkt(req->IntuiMsgFunc,req,imsg);
        // else what to do??? Reply and forget? FIXIT!
        continue;
      }

      sec=imsg->Seconds;
      micro=imsg->Micros;
      qual=imsg->Qualifier;
      class=imsg->Class;
      code=imsg->Code;
      pgsel=(struct Gadget *)imsg->IAddress; // pgsel illegal if not gadget
      GT_ReplyIMsg(imsg);

      switch ( class )
      {
      case IDCMP_RAWKEY:
        switch (code)
        {
        case 0x4c: // Cursor Up
          selected=GetSelected(req);
          if(selected == ~0)
            selected=0;
          if(selected > 0)
            selected--;
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<selected;i++)
            idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,TRUE);
          break;
        case 0x4d: // Cursor Down
          selected=GetSelected(req);
          selected++; // ~0 => 0
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<selected;i++)
            if(idnode->node.ln_Succ->ln_Succ)
              idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,TRUE);
          break;
        case 0x4e: // Cursor Right
          GetSliderAttrs(req,&sliderlevels,&sliderlevel);
          sliderlevel += (qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? 10 :1);
          if(sliderlevel >= sliderlevels)
            sliderlevel=sliderlevels-1;
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,sliderlevel,
              AHIDB_Frequency,&req->tempFrequency,
              TAG_DONE);
          SetSelected(req,FALSE);
          break;
        case 0x4f: // Cursor Left
          GetSliderAttrs(req,&sliderlevels,&sliderlevel);
          sliderlevel -= (qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? 10 :1);
          if(sliderlevel < 0)
            sliderlevel=0;
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,sliderlevel,
              AHIDB_Frequency,&req->tempFrequency,
              TAG_DONE);
          SetSelected(req,FALSE);
          break;
        }
        break;
      case IDCMP_GADGETUP :
        switch ( pgsel->GadgetID )
        {
        case OKBUTTON:
          done=TRUE;
          break;
        case CANCELBUTTON:
          done=TRUE;
          rc=FALSE;
          break;
        case FREQSLIDER:
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,code,
              AHIDB_Frequency,&req->tempFrequency,
              TAG_DONE);
          break;
        case LISTVIEW:
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<code;i++)
            idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,FALSE);
          // Test doubleclick and save timestamp
          if( (oldid == req->tempAudioID) && DoubleClick(oldsec,oldmicro,sec,micro))
            done=TRUE;
          oldsec=sec;
          oldmicro=micro;
          oldid=req->tempAudioID;

          break;
        }
        break;

      case IDCMP_NEWSIZE:
        if(!(LayOutReq(req,req->TextAttr)))
          if(!(LayOutReq(req,&Topaz80)))
          {
            // ERROR! Quit.
            done=TRUE;
            break;
          }
        break;
      case IDCMP_CLOSEWINDOW:
        done=TRUE;
        rc=FALSE;
        break;
      case IDCMP_REFRESHWINDOW :
        GT_BeginRefresh(req->Window);
        GT_EndRefresh(req->Window,TRUE);
        break;
      case IDCMP_SIZEVERIFY:
        break;
      case IDCMP_MENUPICK:
        while((code != MENUNULL) && !done)
        {
          item=ItemAddress(req->Menu, code);
          switch((ULONG)GTMENUITEM_USERDATA(item))
          {
          case LASTMODEITEM:
            selected=GetSelected(req);
            if(selected == ~0)
              selected=0;
            if(selected > 0)
              selected--;
            idnode=(struct IDnode *)req->list->mlh_Head;
            for(i=0;i<selected;i++)
              idnode=(struct IDnode *)idnode->node.ln_Succ;
            req->tempAudioID=idnode->ID;
            SetSelected(req,TRUE);
            break;
          case NEXTMODEITEM:
            selected=GetSelected(req);
            selected++; // ~0 => 0
            idnode=(struct IDnode *)req->list->mlh_Head;
            for(i=0;i<selected;i++)
              if(idnode->node.ln_Succ->ln_Succ)
                idnode=(struct IDnode *)idnode->node.ln_Succ;
            req->tempAudioID=idnode->ID;
            SetSelected(req,TRUE);
            break;
          case PROPERTYITEM:
            OpenInfoWindow(req);
            break;
          case RESTOREITEM:
            req->tempAudioID=req->Req.ahiam_AudioID;
            req->tempFrequency=req->Req.ahiam_MixFreq;
            SetSelected(req,TRUE);
            break;
          case OKITEM:
            done=TRUE;
            break;
          case CANCELITEM:
            done=TRUE;
            rc=FALSE;
            break;
          }
          code = item->NextSelect;
        }
        break;
      }
    }
  }

  if(rc)
  {
    req->Req.ahiam_AudioID = req->tempAudioID;

    if(req->tempAudioID != AHI_DEFAULT_ID)
    {
      req->Req.ahiam_MixFreq = req->tempFrequency;
    }
    else
    {
      req->Req.ahiam_MixFreq = AHI_DEFAULT_FREQ;
    }
  }
  return rc;
}


static void OpenInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  struct Gadget *gad;
  struct NewGadget ng;


  if(req->InfoWindow == NULL)
  {
    req->InfoWindow=OpenWindowTags(NULL,
      WA_Left,              req->Req.ahiam_InfoLeftEdge,
      WA_Top,               req->Req.ahiam_InfoTopEdge,
      WA_Width,             req->Req.ahiam_InfoWidth,
      WA_Height,            req->Req.ahiam_InfoHeight,
      WA_Title,             GetString(msgReqInfoTitle, req->Catalog),
      WA_CustomScreen,      req->Window->WScreen,
      WA_PubScreenFallBack, TRUE,
      WA_DragBar,           TRUE,
      WA_DepthGadget,       TRUE,
      WA_CloseGadget,       TRUE,
      WA_Activate,          FALSE,
      WA_SimpleRefresh,     TRUE,
      WA_AutoAdjust,        TRUE,
      WA_IDCMP,             0,
      WA_NewLookMenus,      TRUE,
      TAG_DONE);

    if(req->InfoWindow)
    {
      req->InfoWindow->UserPort = req->Window->UserPort;
      ModifyIDCMP(req->InfoWindow, MY_INFOIDCMPS);

      if(gad = CreateContext(&req->InfoGadgets))
      {
        ng.ng_TextAttr    = req->TextAttr;
        ng.ng_VisualInfo  = req->vi;
        ng.ng_LeftEdge    = req->InfoWindow->BorderLeft+4;
        ng.ng_TopEdge     = req->InfoWindow->BorderTop+2;
        ng.ng_Width       = req->InfoWindow->Width
                          - (req->InfoWindow->BorderLeft+4)
                          - (req->InfoWindow->BorderRight+4);
        ng.ng_Height      = req->InfoWindow->Height
                          - (req->InfoWindow->BorderTop+2)
                          - (req->InfoWindow->BorderBottom+2);
    
        ng.ng_GadgetText  = NULL;
        ng.ng_GadgetID    = 0;
        ng.ng_Flags       = PLACETEXT_ABOVE;
        gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
            GTLV_ReadOnly,  TRUE,
            TAG_DONE);
        req->InfoListViewGadget = gad;

        if(gad)
        {
          AddGList(req->InfoWindow, req->InfoGadgets, ~0, -1, NULL);
          RefreshGList(req->InfoGadgets, req->InfoWindow, NULL, -1);
          GT_RefreshWindow(req->InfoWindow, NULL);
          UpdateInfoWindow(req);
        }
      }
    }
  }
}


static void UpdateInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  LONG id=0, bits=0, stereo=0, pan=0, hifi=0, channels=0, minmix=0, maxmix=0,
       record=0, fullduplex=0;
  int i;

  id = req->tempAudioID;
  if(id == AHI_DEFAULT_ID)
  {
    id = AHIBase->ahib_AudioMode;
  }
  if(req->InfoWindow)
  {
    AHI_GetAudioAttrs(id, NULL,
      AHIDB_Stereo,       &stereo,
      AHIDB_Panning,      &pan,
      AHIDB_HiFi,         &hifi,
      AHIDB_Record,       &record,
      AHIDB_FullDuplex,   &fullduplex,
      AHIDB_Bits,         &bits,
      AHIDB_MaxChannels,  &channels,
      AHIDB_MinMixFreq,   &minmix,
      AHIDB_MaxMixFreq,   &maxmix,
      TAG_DONE);

    GT_SetGadgetAttrs(req->InfoListViewGadget, req->InfoWindow, NULL,
        GTLV_Labels, ~0,
        TAG_DONE);

    NewList((struct List *) &req->InfoList);
    for(i=0; i<ATTRNODES; i++)
    {
      req->AttrNodes[i].node.ln_Name = req->AttrNodes[i].text;
      req->AttrNodes[i].text[0]      = '\0';
      req->AttrNodes[i].node.ln_Type = NT_USER;
      req->AttrNodes[i].node.ln_Pri  = 0;
    }

    i = 0;
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoAudioID, req->Catalog),
        id);
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoResolution, req->Catalog),
        bits, GetString((stereo ?
          (pan ? msgReqInfoStereoPan : msgReqInfoStereo) :
          msgReqInfoMono), req->Catalog));
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoChannels, req->Catalog),
        channels);
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoMixrate, req->Catalog),
        minmix, maxmix);
    if(hifi)
    {
      AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
      Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoHiFi, req->Catalog));
    }
    if(record)
    {
      AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
      Sprintf(req->AttrNodes[i++].text, GetString(
          fullduplex ? msgReqInfoRecordFull : msgReqInfoRecordHalf, req->Catalog));
    }

    GT_SetGadgetAttrs(req->InfoListViewGadget, req->InfoWindow, NULL,
        GTLV_Labels, &req->InfoList,
        TAG_DONE);
  }
}

static void CloseInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  if(req->InfoWindow)
  {
    req->Req.ahiam_InfoOpened   = TRUE;
    req->Req.ahiam_InfoLeftEdge = req->InfoWindow->LeftEdge;
    req->Req.ahiam_InfoTopEdge  = req->InfoWindow->TopEdge;
    req->Req.ahiam_InfoWidth    = req->InfoWindow->Width;
    req->Req.ahiam_InfoHeight   = req->InfoWindow->Height;
    CloseWindowSafely(req->InfoWindow);
    req->InfoWindow = NULL;

  }
  else
  {
    req->Req.ahiam_InfoOpened   = FALSE;

  }

  FreeGadgets(req->InfoGadgets);
  req->InfoGadgets = NULL;
}


/******************************************************************************
** AHI_AllocAudioRequestA *****************************************************
******************************************************************************/

/****** ahi.device/AHI_AllocAudioRequestA ***********************************
*
*   NAME
*       AHI_AllocAudioRequestA -- allocate an audio mode requester.
*       AHI_AllocAudioRequest -- varargs stub for AHI_AllocAudioRequestA()
*
*   SYNOPSIS
*       requester = AHI_AllocAudioRequestA( tags );
*       D0                                  A0
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequestA(
*           struct TagItem * );
*
*       requester = AHI_AllocAudioRequest( tag1, ... );
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequest( Tag, ... );
*
*   FUNCTION
*       Allocates an audio mode requester data structure.
*
*   INPUTS
*       tags - A pointer to an optional tag list specifying how to initialize
*           the data structure returned by this function. See the
*           documentation for AHI_AudioRequestA() for an explanation of how
*           to use the currently defined tags.
*
*   RESULT
*       requester - An initialized requester data structure, or NULL on
*           failure. 
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*   BUGS
*
*   SEE ALSO
*      AHI_AudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*/

__asm struct AHIAudioModeRequester *AllocAudioRequestA( register __a0 struct TagItem *tags )
{
  struct AHIAudioModeRequesterExt *req;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_AllocAudioRequestA(0x%08lx)",tags);
  }

  if(req=AllocVec(sizeof(struct AHIAudioModeRequesterExt),MEMF_CLEAR))
  {
// Fill in defaults
    req->Req.ahiam_LeftEdge   = 30;
    req->Req.ahiam_TopEdge    = 20;
    req->Req.ahiam_Width      = 318;
    req->Req.ahiam_Height     = 198;
    req->Req.ahiam_AudioID    = AHI_INVALID_ID;
    req->Req.ahiam_MixFreq    = AHIBase->ahib_Frequency;
    req->Req.ahiam_InfoWidth  = 280;
    req->Req.ahiam_InfoHeight = 112;

    req->PubScreenName        = (char *) -1;

    FillReqStruct(req,tags);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%08lx\n",req);
  }

  return (struct AHIAudioModeRequester *) req;
}


/******************************************************************************
** AHI_AudioRequestA **********************************************************
******************************************************************************/

/****** ahi.device/AHI_AudioRequestA ****************************************
*
*   NAME
*       AHI_AudioRequestA -- get an audio mode from user using an requester.
*       AHI_AudioRequest -- varargs stub for AHI_AudioRequestA()
*
*   SYNOPSIS
*       success = AHI_AudioRequestA( requester, tags );
*       D0                           A0         A1
*
*       BOOL AHI_AudioRequestA( struct AHIAudioModeRequester *,
*           struct TagItem * );
*
*       result = AHI_AudioRequest( requester, tag1, ... );
*
*       BOOL AHI_AudioRequest( struct AHIAudioModeRequester *, Tag, ... );
*
*   FUNCTION
*       Prompts the user for an audio mode, based on the modifying tags.
*       If the user cancels or the system aborts the request, FALSE is
*       returned, otherwise the requester's data structure reflects the
*       user input.
*
*       Note that tag values stay in effect for each use of the requester
*       until they are cleared or modified by passing the same tag with a
*       new value.
*
*   INPUTS
*       requester - Requester structure allocated with
*           AHI_AllocAudioRequestA(). If this parameter is NULL, this
*           function will always return FALSE with a dos.library/IoErr()
*           result of ERROR_NO_FREE_STORE.
*       tags - Pointer to an optional tag list which may be used to control
*           features of the requester.
*
*   TAGS
*       Tags used for the requester (they look remarkable similar to the
*       screen mode requester in ASL, don't they? ;-) )
*
*       AHIR_Window (struct Window *) - Parent window of requester. If no
*           AHIR_Screen tag is specified, the window structure is used to
*           determine on which screen to open the requesting window.
*
*       AHIR_PubScreenName (STRPTR) - Name of a public screen to open on.
*           This overrides the screen used by AHIR_Window.
*
*       AHIR_Screen (struct Screen *) - Screen on which to open the
*           requester. This overrides the screen used by AHIR_Window or by
*           AHIR_PubScreenName.
*
*       AHIR_PrivateIDCMP (BOOL) - When set to TRUE, this tells AHI to
*           allocate a new IDCMP port for the requesting window. If not
*           specified or set to FALSE, and if AHIR_Window is provided, the
*           requesting window will share AHIR_Window's IDCMP port.
*
*       AHIR_IntuiMsgFunc (struct Hook *) - A function to call whenever an
*           unknown Intuition message arrives at the message port being used
*           by the requesting window. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A1 - (struct IntuiMessage *)
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_SleepWindow (BOOL) - When set to TRUE, this tag will cause the
*           window specified by AHIR_Window to be "put to sleep". That is, a
*           busy pointer will be displayed in the parent window, and no
*           gadget or menu activity will be allowed. This is done by opening
*           an invisible Intuition Requester in the parent window.
*
*       AHIR_UserData (APTR) - A 32-bit value that is simply copied in the
*           ahiam_UserData field of the requester structure.
*
*       AHIR_TextAttr (struct TextAttr *) - Font to be used for the
*           requesting window's gadgets and menus. If this tag is not
*           provided or its value is NULL, the default font of the screen
*           on which the requesting window opens will be used. This font
*           must already be in memory as AHI calls OpenFont() and not
*           OpenDiskFont().
*
*       AHIR_Locale (struct Locale *) - Locale to use for the requesting
*           window. This determines the language used for the requester's
*           gadgets and menus. If this tag is not provided or its value is
*           NULL, the system's current default locale will be used.
*
*       AHIR_TitleText (STRPTR) - Title to use for the requesting window.
*           Default is no title.
*
*       AHIR_PositiveText (STRPTR) - Label of the positive gadget in the
*           requester. English default is "OK".
*
*       AHIR_NegativeText (STRPTR) - Label of the negative gadget in the
*           requester. English default is "Cancel".
*
*       AHIR_InitialLeftEdge (WORD) - Suggested left edge of requesting
*           window.
*
*       AHIR_InitialTopEdge (WORD) - Suggested top edge of requesting
*           window.
*
*       AHIR_InitialWidth (WORD) - Suggested width of requesting window.
*
*       AHIR_InitialHeight (WORD) - Suggested height of requesting window.
*
*       AHIR_InitialAudioID (ULONG) - Initial setting of the Mode list view
*           gadget (ahiam_AudioID). Default is ~0 (AHI_INVALID_ID), which
*           means that no mode will be selected.
*
*       AHIR_InitialMixFreq (ULONG) - Initial setting of the frequency
*           slider. Default is the lowest frequency supported.
*
*       AHIR_InitialInfoOpened (BOOL) - Whether to open the property
*           information window automatically. Default is FALSE.
*
*       AHIR_InitialInfoLeftEdge (WORD) - Initial left edge of information
*           window.
*
*       AHIR_InitialInfoTopEdge (WORD) - Initial top edge of information
*           window.
*
*       AHIR_DoMixFreq (BOOL) - Set this tag to TRUE to cause the requester
*           to display the frequency slider gadget. Default is FALSE.
*
*       AHIR_DoDefaultMode (BOOL) - Set this tag to TRUE to let the user
*           select the mode she has set in the preferences program. If she
*           selects this mode,  ahiam_AudioID will be AHI_DEFAULT_ID and
*           ahiam_MixFreq will be AHI_DEFAULT_FREQ. Note that if you filter
*           the mode list (see below), you must also check the mode (with
*           AHI_BestAudioIDA()) before you use it since the user may change 
*           the meaning of AHI_DEFAULT_MODE anytime, without your knowledge.
*           Default is FALSE. (V4)
*
*       AHIR_FilterFunc (struct Hook *) - A function to call for each mode
*           encountered. If the function returns TRUE, the mode is included
*           in the file list, otherwise it is rejected and not displayed. The
*           function receives the following parameters:
*               A0 - (struct Hook *)
*               A1 - (ULONG) mode id
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_FilterTags (struct TagItem *) - A pointer to a tag list used to
*           filter modes away, like AHIR_FilterFunc does. The tags are the
*           same as AHI_BestAudioIDA() takes as arguments. See that function
*           for an explanation of each tag.
*
*   RESULT
*       result - FALSE if the user cancelled the requester or if something
*           prevented the requester from opening. If TRUE, values in the
*           requester structure will be set.
*
*           If the return value is FALSE, you can look at the result from the
*           dos.library/IoErr() function to determine whether the requester
*           was cancelled or simply failed to open. If dos.library/IoErr()
*           returns 0, then the requester was cancelled, any other value
*           indicates a failure to open. Current possible failure codes are
*           ERROR_NO_FREE_STORE which indicates there was not enough memory,
*           and ERROR_NO_MORE_ENTRIES which indicates no modes were available
*           (usually because the application filter hook filtered them all
*           away).
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*       The mixing/recording frequencies that are presented to the user
*       may not be the only ones a driver supports, but just a selection.
*
*   BUGS
*
*   SEE ALSO
*      AHI_AllocAudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*/

__asm ULONG AudioRequestA( register __a0 struct AHIAudioModeRequester *req_in, register __a1 struct TagItem *tags )
{
  struct AHIAudioModeRequesterExt *req=(struct AHIAudioModeRequesterExt *)req_in;
  struct MinList list;
  struct IDnode *node,*node2;
  ULONG screenTag=TAG_IGNORE,screenData,id=AHI_INVALID_ID;
  BOOL  rc=TRUE;
  struct Requester lockreq;
  BOOL  locksuxs;
  WORD zipcoords[4];

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_AudioRequestA(0x%08lx, 0x%08lx)",req_in,tags);
  }

  if(!req)
    return FALSE;

// Update requester structure
  FillReqStruct(req,tags);
  if(req->Req.ahiam_InfoLeftEdge == 0)
    req->Req.ahiam_InfoLeftEdge=req->Req.ahiam_LeftEdge+16;
  if(req->Req.ahiam_InfoTopEdge == 0)
    req->Req.ahiam_InfoTopEdge=req->Req.ahiam_TopEdge+25;
  req->tempAudioID=req->Req.ahiam_AudioID;
  req->tempFrequency=req->Req.ahiam_MixFreq;

// Open the catalog

  req->Catalog = ExtOpenCatalog(req->Locale, NULL);

  if(req->PositiveText == NULL)
    req->PositiveText = GetString(msgReqOK, req->Catalog);
  if(req->NegativeText == NULL)
    req->NegativeText = GetString(msgReqCancel, req->Catalog);


// Scan audio database for modes and create list

  req->list=&list;
  NewList((struct List *)req->list);
  while(AHI_INVALID_ID != (id=AHI_NextAudioID(id)))
  {
    // Check FilterTags
    if(req->FilterTags)
      if(!TestAudioID(id,req->FilterTags))
        continue;
    if(req->FilterFunc)
      if(!CallHookPkt(req->FilterFunc,req,(APTR)id))
        continue;
    // Add mode to list
    if(node=AllocVec(sizeof(struct IDnode),MEMF_ANY))
    {
      node->node.ln_Type=NT_USER;
      node->node.ln_Pri=0;
      node->node.ln_Name=node->name;
      node->ID=id;
      Sprintf(node->node.ln_Name, GetString(msgUnknown, req->Catalog),id);
      AHI_GetAudioAttrs(id, NULL,
          AHIDB_BufferLen,80,
          AHIDB_Name,node->node.ln_Name,
          TAG_DONE);
      // Insert node alphabetically
      for(node2=(struct IDnode *)req->list->mlh_Head;node2->node.ln_Succ;node2=(struct IDnode *) node2->node.ln_Succ)
        if(Stricmp(node->node.ln_Name,node2->node.ln_Name) < 0)
          break;
      Insert((struct List *) req->list,(struct Node *)node,node2->node.ln_Pred);
    }
  }

// Add the users prefered mode

  if((req->Flags & defaultmode) && (AHIBase->ahib_AudioMode != AHI_INVALID_ID)) do
  {
    if(req->FilterTags)
      if(!TestAudioID(AHIBase->ahib_AudioMode,req->FilterTags))
        continue;
    if(req->FilterFunc)
      if(!CallHookPkt(req->FilterFunc,req,(APTR)id))
        continue;

    if(node=AllocVec(sizeof(struct IDnode),MEMF_ANY))
    {
      node->node.ln_Type=NT_USER;
      node->node.ln_Pri=0;
      node->node.ln_Name=node->name;
      node->ID = AHI_DEFAULT_ID;
      Sprintf(node->node.ln_Name, GetString(msgDefaultMode, req->Catalog));
      AddTail((struct List *) req->list, (struct Node *)node);
    }
  } while(FALSE);

  if(NULL == ((struct IDnode *)req->list->mlh_Head)->node.ln_Succ)
  {
    // List is empty, no audio modes!
    // Return immediately (no nodes to free)
    SetIoErr(ERROR_NO_MORE_ENTRIES);
    ExtCloseCatalog(req->Catalog);
    req->Catalog = FALSE;
    return FALSE;
  }

// Find our sceeen
//  req->PubScreen=LockPubScreen(req->PubScreenName);

// Clear ownIDCMP flag
  req->Flags &= ~ownIDCMP;
  if(req->Screen)
  {
    screenTag=(ULONG)WA_CustomScreen;
    screenData=(ULONG)req->Screen;
  }
  else if(req->PubScreenName != (char *) -1)
  {
    screenTag=(ULONG)WA_PubScreenName;
    screenData=(ULONG)req->PubScreenName;
  }
  else if(req->SrcWindow)
  {
    screenTag=(ULONG)WA_CustomScreen;
    screenData=(ULONG)req->SrcWindow->WScreen;
    if(req->Flags & haveIDCMP)
      req->Flags |= ownIDCMP;
  }

  zipcoords[0]=req->Req.ahiam_LeftEdge;
  zipcoords[1]=req->Req.ahiam_TopEdge;
  zipcoords[2]=1;
  zipcoords[3]=1;
  req->Window=OpenWindowTags(NULL,
    WA_Left,req->Req.ahiam_LeftEdge,
    WA_Top,req->Req.ahiam_TopEdge,
    WA_Width,req->Req.ahiam_Width,
    WA_Height,req->Req.ahiam_Height,
    WA_Zoom,zipcoords,
    WA_MaxWidth,~0,
    WA_MaxHeight,~0,
    WA_Title,req->TitleText,
    screenTag,screenData,
    WA_PubScreenFallBack,TRUE,
    WA_SizeGadget,TRUE,
    WA_SizeBBottom,TRUE,
    WA_DragBar,TRUE,
    WA_DepthGadget,TRUE,
    WA_CloseGadget,TRUE,
    WA_Activate,TRUE,
    WA_SimpleRefresh,TRUE,
    WA_AutoAdjust,TRUE,
    WA_IDCMP,(req->Flags & ownIDCMP ? NULL : MY_IDCMPS),
    WA_NewLookMenus, TRUE,
    TAG_DONE);

//  UnlockPubScreen(NULL,req->PubScreen);

  if(req->Window)
  {
    WindowLimits(req->Window,
      // Topaz80: "Frequency"+INTERWIDTH+MINSLIDERWIDTH+INTERWIDTH+"99999 Hz" gives...
      (req->Window->BorderLeft+4)+
      strlen( GetString(msgReqFrequency, req->Catalog))*8+
      INTERWIDTH+MINSLIDERWIDTH+INTERWIDTH+
      FREQLEN2*8+
      (req->Window->BorderRight+4),
      // Topaz80: 5 lines, freq & buttons gives...
      (req->Window->BorderTop+2)+(5*8+6)+2+(8+6)+2+(8+6)+(req->Window->BorderBottom+2),
      0,0);

    if(req->vi=GetVisualInfoA(req->Window->WScreen, NULL))
    {
      if(!(LayOutReq(req,req->TextAttr)))
        if(!(LayOutReq(req,&Topaz80)))
          rc=FALSE;

      if(rc) // Layout OK?
      {
        struct NewMenu reqnewmenu[] =
        {
          { NM_TITLE, NULL        , 0 ,0,0,(APTR) 0,            },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) LASTMODEITEM, },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) NEXTMODEITEM, },
          {  NM_ITEM, NM_BARLABEL , 0 ,0,0,(APTR) 0,            },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) PROPERTYITEM, },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) RESTOREITEM , },
          {  NM_ITEM, NM_BARLABEL , 0 ,0,0,(APTR) 0,            },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) OKITEM,       },
          {  NM_ITEM, NULL        , 0 ,0,0,(APTR) CANCELITEM,   },
          {   NM_END, NULL        , 0 ,0,0,(APTR) 0,            },
        };
        const static APTR strings[] =
        {
          msgMenuControl,
          msgMenuLastMode,
          msgMenuNextMode,
          msgMenuPropertyList,
          msgMenuRestore,
          msgMenuOK,
          msgMenuCancel,
        };

        struct NewMenu *menuptr;
        APTR           *stringptr;
        
        menuptr   = (struct NewMenu *) &reqnewmenu;
        stringptr = (APTR *) &strings;

        while(menuptr->nm_Type != NM_END)
        {
          if(menuptr->nm_Label == NULL)
          {
            menuptr->nm_CommKey = GetString(*stringptr, req->Catalog);
            menuptr->nm_Label = menuptr->nm_CommKey + 2;
            stringptr++;
          }
          menuptr++;
        }

        if(req->Flags & ownIDCMP)
        {
          req->Window->UserPort=req->SrcWindow->UserPort;
          ModifyIDCMP(req->Window,MY_IDCMPS);
        }

        if((req->Flags & lockwin) && req->SrcWindow)
        {
          InitRequester(&lockreq);
          locksuxs=Request(&lockreq,req->SrcWindow);
          if(IntuitionBase->LibNode.lib_Version >= 39)
            SetWindowPointer(req->SrcWindow,
                WA_BusyPointer,TRUE,
                TAG_DONE);
        }
        
        // Add menus
        if(req->Menu=CreateMenus(reqnewmenu, 
            GTMN_FullMenu, TRUE,
            GTMN_NewLookMenus, TRUE,
            TAG_DONE ))
        {
          if(LayoutMenus(req->Menu,req->vi, TAG_DONE))
          {
            if(SetMenuStrip(req->Window, req->Menu))
            {
              if(req->Req.ahiam_InfoOpened)
              {
                OpenInfoWindow(req);
              }

              rc=HandleReq(req);

              CloseInfoWindow(req);
              ClearMenuStrip(req->Window);
            }
          } // else LayoutMenus failed
          FreeMenus(req->Menu);
          req->Menu=NULL;
        } // else CreateMenus failed


        if((req->Flags & lockwin) && req->SrcWindow)
        {
          if(locksuxs)
            EndRequest(&lockreq,req->SrcWindow);
          if(IntuitionBase->LibNode.lib_Version >= 39)
            SetWindowPointer(req->SrcWindow,
                WA_BusyPointer,FALSE,
                TAG_DONE);
        }

        req->Req.ahiam_LeftEdge = req->Window->LeftEdge;
        req->Req.ahiam_TopEdge  = req->Window->TopEdge;
        req->Req.ahiam_Width    = req->Window->Width;
        req->Req.ahiam_Height   = req->Window->Height;
      } // else LayOutReq failed
    }
    else // no vi
    {
      SetIoErr(ERROR_NO_FREE_STORE);
      rc=FALSE;
    }

    if(req->Flags & ownIDCMP)
      CloseWindowSafely(req->Window);
    else
      CloseWindow(req->Window);
    req->Window=NULL;
    FreeVisualInfo(req->vi);
    req->vi=NULL;
    FreeGadgets(req->Gadgets);
    req->Gadgets=NULL;
    while(node=(struct IDnode *) RemHead((struct List *)req->list))
      FreeVec(node);
  }
  else // no window
  {
    SetIoErr(ERROR_NO_FREE_STORE);
    rc=FALSE;
  }

  ExtCloseCatalog(req->Catalog);
  req->Catalog = NULL;
  req->PositiveText = req->NegativeText = NULL;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%s\n",rc ? "TRUE" : "FALSE" );
  }
  return (ULONG) rc;
}


/******************************************************************************
** AHI_FreeAudioRequest *******************************************************
******************************************************************************/

/****** ahi.device/AHI_FreeAudioRequest *************************************
*
*   NAME
*       AHI_FreeAudioRequest -- frees requester resources 
*
*   SYNOPSIS
*       AHI_FreeAudioRequest( requester );
*                             A0
*
*       void AHI_FreeAudioRequest( struct AHIAudioModeRequester * );
*
*   FUNCTION
*       Frees any resources allocated by AHI_AllocAudioRequestA(). Once a
*       requester has been freed, it can no longer be used with other calls to
*       AHI_AudioRequestA().
*
*   INPUTS
*       requester - Requester obtained from AHI_AllocAudioRequestA(), or NULL
*       in which case this function does nothing.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_AllocAudioRequestA()
*
****************************************************************************
*
*/

__asm void FreeAudioRequest( register __a0 struct AHIAudioModeRequester *req)
{

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_FreeAudioRequest(0x%08lx)\n",req);
  }

  if(req)
    FreeVec(req);
}
