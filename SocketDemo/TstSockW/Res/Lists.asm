
;--- assemble: JWasm -coff lists.asm

	.386
	.MODEL FLAT, stdcall
	option casemap:none
	option proc:private

?USESEMAPHOR equ 0 ;1=make it MT compatible

malloc proto c :dword
free   proto c :ptr
strlen proto c :ptr

if ?USESEMAPHOR
CloseHandle      proto :ptr
CreateSemaphore  proto :ptr, :dword, :dword, :ptr
ReleaseSemaphore proto :ptr, :dword, ptr DWORD
WaitForSingleObject proto :ptr, :dword
endif

;	include lists.inc

SLIST	struct
pFirst	dd ?
if ?USESEMAPHOR
semaphor dd ?
endif
SLIST	ends

SLITEM	struct
pNext	dd ?
SLITEM	ends

;*** procedures
;
;HLIST  CreateList(void)
;BOOL   DestroyList(HLIST)
;PSTR   AddListItem(HLIST,PITEM,LENGTH)
;PSTR   AddListString(HLIST,PSTR)
;BOOL   DeleteListItem(HLIST,PITEM)
;int    GetNextListItem(HLIST,INDEX,PITEM,LENGTH)
;PSTR   GetNextListString(HLIST, INDEX)
;int    GetPrevListItem(HLIST,INDEX,PITEM,LENGTH)
;int    GetListItemCount(HLIST);

	.CODE

CreateList proc public

	invoke malloc, sizeof SLIST
	and eax,eax
	jz @F
	xor ecx,ecx
	mov [eax.SLIST.pFirst],ecx
if ?USESEMAPHOR
	push eax
	invoke CreateSemaphore, 0,1,1,0			;semaphore is signaled (nonblocking)
	pop ecx
	mov [ecx.SLIST.semaphor],eax
	mov eax,ecx
endif
@@:
	ret
	align 4
CreateList endp


if ?USESEMAPHOR
DestroyList proc public uses ebx handle:ptr

	mov ecx,handle
	jecxz dl_exx
	mov ebx,[ecx.SLIST.semaphor]
	invoke WaitForSingleObject, ebx, INFINITE
else
DestroyList proc public handle:ptr
endif
	mov ecx,handle
dl_1:
	jecxz dl_ex
	push [ecx.SLITEM.pNext]
	invoke free, ecx
	pop ecx
	jmp dl_1
dl_ex:
if ?USESEMAPHOR
	invoke CloseHandle, ebx
dl_exx:
endif
	ret
	align 4
DestroyList endp

if ?USESEMAPHOR
InsertItem proc uses ebx handle:ptr, pItem:ptr

	mov ecx,handle
	jecxz ii_exx
	mov ebx,[ecx.SLIST.semaphor]
	invoke WaitForSingleObject, ebx, INFINITE
else
InsertItem proc handle:ptr, pItem:ptr
endif
	xor eax,eax
	mov ecx,handle
ii_1:
	jecxz ii_2
	mov eax,ecx
	mov ecx,[ecx.SLITEM.pNext]
	jmp ii_1
ii_2:
	and eax,eax
	jz ii_ex
	mov ecx,pItem
	mov [eax.SLITEM.pNext],ecx
ii_ex:
if ?USESEMAPHOR
	invoke ReleaseSemaphore, ebx, 1, 0
ii_exx:
endif
	ret
	align 4
InsertItem endp

AddListItem proc public uses esi edi handle:ptr, pString:ptr, len:dword

	mov eax,handle
	and eax,eax
	jz ali_ex
	mov eax,len
	add eax,sizeof SLITEM
	invoke malloc, eax
	and eax,eax
	jz ali_ex
	mov edi,eax
	push edi
	xor eax,eax
	stosd
	mov esi,pString
	mov ecx,len
	rep movsb
	pop edi
	invoke InsertItem,handle,edi
	lea eax,[edi + sizeof SLITEM]
ali_ex:
	ret
AddListItem endp

AddListString proc public handle:ptr, pString:ptr byte

	invoke strlen, pString
	inc eax
	invoke AddListItem,handle,pString,eax
	ret
	align 4

AddListString endp

if ?USESEMAPHOR
DeleteListItem proc public uses ebx handle:ptr, pItem:ptr

	mov ecx,handle
	jecxz dli_exx
	mov ebx,[ecx.SLIST.semaphor]
	invoke WaitForSingleObject, ebx, INFINITE
else
DeleteListItem proc public handle:ptr, pItem:ptr
endif
	mov eax,pItem
	lea eax,[eax - sizeof SLITEM]
	xor edx,edx
	mov ecx,handle
dli_1:
	jecxz dli_ex
	cmp eax,ecx
	jz dli_2
	mov edx,ecx
	mov ecx,[ecx.SLITEM.pNext]
	jmp dli_1
dli_2:
	mov eax,[ecx.SLITEM.pNext]
	mov [edx.SLITEM.pNext],eax
	invoke free, ecx
	mov ecx,1
dli_ex:
if ?USESEMAPHOR
	push ecx
	invoke ReleaseSemaphore, ebx,1,0
	pop ecx
dli_exx:
endif
	mov eax,ecx
	ret
	align 4
DeleteListItem endp

DeleteListItemByIndex proc public handle:ptr, dwIndex:dword

	mov ecx,handle
	jecxz dli_exx
	mov eax,dwIndex
	mov edx,ecx
	mov ecx,[ecx.SLIST.pFirst]
	.while (eax && ecx)
		mov edx,ecx
		mov ecx,[ecx.SLITEM.pNext]
		dec eax
	.endw
	.if (ecx)
		mov eax,[ecx.SLITEM.pNext]
		mov [edx.SLITEM.pNext],eax
		invoke free, ecx
	.endif
dli_exx:
	ret
	align 4
DeleteListItemByIndex endp

searchactitem proc
	mov eax,[ebx.SLIST.pFirst]
	inc ecx
	jmp sm2
sm1:
	mov eax,[eax.SLITEM.pNext]
sm2:
	and eax,eax
	loopnz sm1
sai_1:
	and eax,eax
	jz sai_ex
	lea eax,[eax + sizeof SLITEM]
	mov ecx,esi
	mov esi,eax
	and edi,edi
	jz sai_ex
	rep movsb
sai_ex:
	ret
	align 4
searchactitem endp

GetNextListItem proc public uses ebx esi edi handle:ptr, index:dword, pItem:ptr, laenge:dword

	mov ebx,handle
	mov ecx,index
	inc ecx
	mov edi,pItem
	mov esi,laenge
	call searchactitem
	ret
	align 4
GetNextListItem endp


GetNextListString proc public handle:ptr, index:dword
	mov edx,handle
	mov ecx,index
	mov eax,[edx.SLIST.pFirst]
	inc ecx
	jecxz sm1
@@:
	mov eax,[eax.SLITEM.pNext]
	and eax,eax
	loopnz @B
sm1:
	and eax,eax
	jz exit_
	lea eax,[eax + sizeof SLITEM]
exit_:
	ret
	align 4
GetNextListString endp

GetPrevListItem proc public uses ebx esi edi handle:ptr, index:dword, pItem:ptr, laenge:dword

	mov ebx,handle
	mov ecx,index
	dec ecx
	xor eax,eax
	cmp ecx,0
	jl @F
	mov edi,pItem
	mov esi,laenge
	call searchactitem
@@:
	ret
	align 4
GetPrevListItem endp


GetListItemCount proc public handle:ptr
	mov ecx,handle
	xor eax,eax
	jecxz exit_
	mov ecx,[ecx.SLIST.pFirst]
sm1:
	jecxz exit_
	inc eax
	mov ecx,[ecx.SLITEM.pNext]
	jmp sm1
exit_:
	ret
	align 4
GetListItemCount endp


end

