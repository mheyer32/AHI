* $Id$
* $Log$
* Revision 1.1  1998/12/20 12:09:26  lcs
* Initial revision
*
* Revision 4.1  1997/05/04 05:31:20  lcs
* *** empty log message ***
*
*

	include	"lvo/exec_lib.i"

	SECTION "TEXT",CODE
;
; Simple version of the C "sprintf" function.  Assumes C-style
; stack-based function conventions.
;
;   long eyecount;
;   eyecount=2;
;   sprintf(string,"%s have %ld eyes.","Fish",eyecount);
;
; would produce "Fish have 2 eyes." in the string buffer.
;

        XDEF _SPrintfA
_SPrintfA:       ; ( ostring, format, {values} )
	movem.l	a2/a3/a6,-(sp)

	lea.l	stuffChar(pc),a2
	move.l	4.w,a6
	jsr	_LVORawDoFmt(a6)

	movem.l	(sp)+,a2/a3/a6
	rts

;------ PutChProc function used by RawDoFmt -----------
stuffChar:
	move.b	d0,(a3)+	;Put data to output string
	rts

	END
;*/
