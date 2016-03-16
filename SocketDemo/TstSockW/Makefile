
# NMAKE makefile.
# creates TSTSOCKW.EXE
# use one of the following tools:
# - MS VC v6
# - MS VisualC++ Toolkit 2003 (+ MS Platform SDK)
# just compiler, linker and resource compiler are needed

name = tstsockw

# some root directories which most likely need to be adjusted
VCDIR=\msvc8
PSDK=\Microsoft SDK

!ifndef DEBUG
DEBUG=0
!endif

SRCMODS = \
!include modules.inc

OBJMODS = $(SRCMODS:.cpp=.obj)
!if $(DEBUG)
OBJMODS = $(OBJMODS:.\=DEBUG\)
!else
OBJMODS = $(OBJMODS:.\=RELEASE\)
!endif

!if $(DEBUG)
LOPTD=/DEBUG:FULL
AOPTD=-Zi -DDEBUG
COPTD=-Zi -Od -D "_DEBUG"
OUTDIR=DEBUG
!else
LOPTD=
AOPTD=
COPTD=-Ox -D "NDEBUG"
OUTDIR=RELEASE
!endif

LANGCC=-D "AFX_TARG_ENG"
LANGRC=/l 0x409 /d AFX_TARG_ENG 

INCS=rsrc.h tstsockw.h CExplorerView.h
CC  = $(VCDIR)\bin\cl.exe -c -nologo -W3 -MT -Fo$* $(COPTD) -D "WIN32" -D "_WINDOWS" $(LANGCC) -I $(VCDIR)\Include -I "$(PSDK)\Include"
RC  = $(VCDIR)\bin\rc.exe $(LANGRC) /d AFX_TARG_NEUD /d AFX_RESOURCE_DLL /fo $*.res -i "$(PSDK)\Include"
LIBS=kernel32.lib advapi32.lib user32.lib gdi32.lib shell32.lib comctl32.lib comdlg32.lib ole32.lib uuid.lib wsock32.lib
LOPT=/OUT:$*.exe /MAP:$*.map /SUBSYSTEM:WINDOWS $(LOPTD)
LINK=link.exe /OPT:NOWIN98 /LIBPATH:"$(VCDIR)\Lib" /LIBPATH:"$(PSDK)\Lib"

ALL: $(OUTDIR) $(OUTDIR)\$(name).exe

$(OUTDIR):
	@mkdir $(OUTDIR)

$(OUTDIR)\$(name).exe: $(OBJMODS) $(OUTDIR)\$(name).res Makefile
	$(LINK) $(OBJMODS) $(OUTDIR)\$(name).res RES\lists.obj $(LIBS) $(LOPT)

.cpp{$(OUTDIR)}.obj:
	$(CC) $<

.rc{$(OUTDIR)}.res:
	$(RC) $<

$(OBJMODS): $(INCS) Makefile

$(OUTDIR)\$(name).res: $(name).rc Makefile RES\tstsockw.ICO 

