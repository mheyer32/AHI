
	.MACRO	LONG num
.ifdef	LITTLE_ENDIAN
	.byte	((\num)>>24)&255
	.byte	((\num)>>16)&255
	.byte	((\num)>> 8)&255
	.byte	((\num)>> 0)&255
.else
	.long	\num
.endif
	.ENDM

	.MACRO	LONG2 num1,num2
.ifdef	LITTLE_ENDIAN
	.byte	((\num1)>>24)&255
	.byte	((\num1)>>16)&255
	.byte	((\num1)>> 8)&255
	.byte	((\num1)>> 0)&255
	
	.byte	((\num2)>>24)&255
	.byte	((\num2)>>16)&255
	.byte	((\num2)>> 8)&255
	.byte	((\num2)>> 0)&255
.else
	.long	\num1, \num2
.endif
	.ENDM

	.MACRO	FORM_START name
CHUNKCNT	.ASSIGNA 0
	.ascii	"FORM"
	LONG	FORMEND-FORMSTART
FORMSTART:
	.ascii	"\name"
	.ENDM

	.MACRO	FORM_END name
FORMEND:
	.balign	2,0
	.ENDM

	.MACRO	CHUNK_START name
	.ascii	"\name"
	LONG	CHUNKEND\&CHUNKCNT-CHUNKSTART\&CHUNKCNT
CHUNKSTART\&CHUNKCNT:
	.ENDM

	.MACRO	CHUNK_END
CHUNKEND\&CHUNKCNT:
CHUNKCNT	.ASSIGNA \&CHUNKCNT+1
	.balign	2,0
	.ENDM

AHIDB_AudioID	.EQU	2147483648+100
AHIDB_Volume	.EQU	2147483648+103
AHIDB_Panning	.EQU	2147483648+104
AHIDB_Stereo	.EQU	2147483648+105
AHIDB_HiFi	.EQU	2147483648+106
AHIDB_MultTable	.EQU	2147483648+108
AHIDB_Name	.EQU	2147483648+32768+109

TAG_DONE	.EQU	0
	
TRUE		.EQU	1
FALSE		.EQU	0
