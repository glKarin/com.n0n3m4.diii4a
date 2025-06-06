; /* Copyright (c) 2002-2012 Croteam Ltd. All rights reserved. */
;
; ------- YOU DO NOT NEED THIS FILE UNDER WIN32 AT THE MOMENT. -------
;
;  Chances are, you will need NASM (http://www.octium.net/nasm/) to assemble
;  this file.
;
; If we were smart, we'd move all the inline asm to similar files, which
;  cleans up the C, and makes the assembly (ahem) more portable. NASM can
;  generate Visual C compatible object files as well as Linux ELF and Intel
;  Mac Mach-O binaries.
;
;  --ryan.

; asm shortcuts
%define O offset
%define Q qword
%define D dword
%define W  word
%define B  byte

%define TRUE 1
%define FALSE 0

; ************************
; **  Start Data Block  **
; ************************
SECTION .data

global slMixerBufferSize
global bEndOfSound
global bNotLoop
global fLeftOfs
global fLeftStep
global fOfsDelta
global fPhase
global fRightOfs
global fRightStep
global fSoundSampleRate
global fStep
global fixLeftOfs
global fixRightOfs
global mmSurroundFactor
global mmVolumeGain
global pswSrcBuffer
global pvMixerBuffer
global slLastLeftSample
global slLastRightSample
global slLeftFilter
global slLeftVolume
global slRightFilter
global slRightVolume
global slSoundBufferSize

slMixerBufferSize dd 0
pvMixerBuffer     dd 0
pswSrcBuffer      dd 0
slLeftVolume      dd 0
slRightVolume     dd 0
slLeftFilter      dd 0
slRightFilter     dd 0
slLastLeftSample  dd 0
slLastRightSample dd 0
slSoundBufferSize dd 0
fSoundSampleRate  dd 0
fPhase            dd 0
fOfsDelta         dd 0
fStep             dd 0
fLeftStep         dd 0
fRightStep        dd 0
fLeftOfs          dd 0
fRightOfs         dd 0
fixLeftOfs        dd 0, 0
fixRightOfs       dd 0, 0
mmSurroundFactor  dd 0, 0
mmLeftStep        dd 0, 0
mmRightStep       dd 0, 0
mmVolumeGain      dd 0, 0
mmInvFactor       dd 0x00007FFF, 0x00007FFF
bNotLoop          dd 0
bEndOfSound       dd 0
f65536            dd 65536.0
f4G               dd 4294967296.0


; ************************
; **   End Data Block   **
; ************************


; ************************
; **  Start Code Block  **
; ************************
SEGMENT .text

global MixMono_asm
MixMono_asm:
    push    ebx   ; Save GCC register.
    push    esi
    push    edi
    ; convert from floats to fixints 32:16
    fld     D [fLeftOfs]
    fmul    D [f65536]
    fld     D [fRightOfs]
    fmul    D [f65536]
    fld     D [fLeftStep]
    fmul    D [f65536]
    fld     D [fRightStep]
    fmul    D [f4G]
    fistp   Q [mmRightStep]  ; fixint 32:32
    fistp   Q [mmLeftStep]   ; fixint 32:16
    fistp   Q [fixRightOfs]  ; fixint 32:16
    fistp   Q [fixLeftOfs]   ; fixint 32:16

    ; get last played sample (for filtering purposes)
    movzx   eax,W [slLastRightSample]
    movzx   edx,W [slLastLeftSample]
    shl     eax,16
    or      eax,edx
    movd    mm6,eax                       ; MM6 = 0 | 0 || lastRightSample | lastLeftSample

    ; get volume
    movd    mm5,D [slRightVolume]
    movd    mm0,D [slLeftVolume]
    psllq   mm5,32
    por     mm5,mm0                       ; MM5 = rightVolume || leftVolume

    ; get filter
    mov     eax,D [slRightFilter]
    mov     edx,D [slLeftFilter]
    shl     eax,16
    or      eax,edx
    movd    mm7,eax                       ; MM7 = 0 | 0 || rightFilter | leftFilter

    ; get offset of each channel inside sound and loop thru destination buffer
    mov     W [mmRightStep],0
    movzx   eax,W [fixLeftOfs]
    movzx   edx,W [fixRightOfs]
    shl     edx,16
    or      eax,edx                       ; EAX = right ofs frac | left ofs frac
    mov     ebx,D [fixLeftOfs+2]          ; EBX = left ofs int
    mov     edx,D [fixRightOfs+2]         ; EDX = right ofs int
    mov     esi,D [pswSrcBuffer]          ; ESI = source sound buffer start ptr
    mov     edi,D [pvMixerBuffer]         ; EDI = mixer buffer ptr
    mov     ecx,D [slMixerBufferSize]     ; ECX = samples counter

sampleLoop_MixMono:
    ; check if source offsets came to the end of source sound buffer
    cmp     ebx,D [slSoundBufferSize]
    jl      lNotEnd_MixMono
    sub     ebx,D [slSoundBufferSize]
    push    D [bNotLoop]
    pop     D [bEndOfSound]
lNotEnd_MixMono:
    ; same for right channel
    cmp     edx,D [slSoundBufferSize]
    jl      rNotEnd_MixMono
    sub     edx,D [slSoundBufferSize]
    push    D [bNotLoop]
    pop     D [bEndOfSound]
rNotEnd_MixMono:

    ; check end of sample
    cmp     ecx,0
    jle     near loopEnd_MixMono
    cmp     D [bEndOfSound],TRUE
    je      near loopEnd_MixMono

    ; get sound samples
    movd    mm1,D [esi+ ebx*2]    ; MM1 = 0 | 0 || nextLeftSample  | leftSample
    movd    mm2,D [esi+ edx*2]    ; MM2 = 0 | 0 || nextRightSample | RightSample
    psllq   mm2,32
    por     mm1,mm2   ; MM1 = nextRightSample | rightSample || nextLeftSample | leftSample

    ; calc linear interpolation factor (strength)
    movd    mm3,eax   ; MM3 = 0 | 0 || right frac | left frac
    punpcklwd mm3,mm3
    psrlw   mm3,1     ; MM3 = rightFrac | rightFrac || leftFrac | leftFrac
    pxor    mm3,Q [mmInvFactor] ; MM3 = rightFrac | 1-rightFrac || leftFrac | 1-leftFrac
    ; apply linear interpolation
    pmaddwd mm1,mm3
    psrad   mm1,15
    packssdw mm1,mm1  ; MM1 = ? | ? || linearRightSample | linearLeftSample

    ; apply filter
    psubsw  mm1,mm6
    pmulhw  mm1,mm7
    psllw   mm1,1
    paddsw  mm1,mm6
    movq    mm6,mm1

    ; apply volume adjustment
    movq    mm0,mm5
    psrad   mm0,16
    packssdw mm0,mm0
    pmulhw  mm1,mm0
    psllw   mm1,1
    pxor    mm1,Q [mmSurroundFactor]
    paddd   mm5,Q [mmVolumeGain]   ; modify volume

    ; unpack to 32bit and mix it into destination buffer
    punpcklwd mm1,mm1
    psrad   mm1,16              ; MM1 = finalRightSample || finalLeftSample
    paddd   mm1,Q [edi]
    movq    Q [edi],mm1

    ; advance to next samples in source sound
    add     eax,D [mmRightStep+0]
    adc     edx,D [mmRightStep+4]
    add      ax,W [mmLeftStep +0]
    adc     ebx,D [mmLeftStep +2]
    add     edi,8
    dec     ecx
    jmp     sampleLoop_MixMono

loopEnd_MixMono:
    ; store modified asm local vars
    mov     D [fixLeftOfs +0],eax
    shr     eax,16
    mov     D [fixRightOfs+0],eax
    mov     D [fixLeftOfs +2],ebx
    mov     D [fixRightOfs+2],edx
    movd    eax,mm6
    mov     edx,eax
    and     eax,0x0000FFFF
    shr     edx,16
    mov     D [slLastLeftSample],eax
    mov     D [slLastRightSample],edx
    pop     edi
    pop     esi
    pop     ebx   ; Restore GCC register.
    emms
    ret


global MixStereo_asm
MixStereo_asm:
    push    ebx   ; Save GCC register.
    push    esi
    push    edi
    ; convert from floats to fixints 32:16
    fld     D [fLeftOfs]
    fmul    D [f65536]
    fld     D [fRightOfs]
    fmul    D [f65536]
    fld     D [fLeftStep]
    fmul    D [f65536]
    fld     D [fRightStep]
    fmul    D [f4G]
    fistp   Q [mmRightStep] ; fixint 32:32
    fistp   Q [mmLeftStep]  ; fixint 32:16
    fistp   Q [fixRightOfs] ; fixint 32:16
    fistp   Q [fixLeftOfs]  ; fixint 32:16

    ; get last played sample (for filtering purposes)
    movzx   eax,W [slLastRightSample]
    movzx   edx,W [slLastLeftSample]
    shl     eax,16
    or      eax,edx
    movd    mm6,eax                       ; MM6 = 0 | 0 || lastRightSample | lastLeftSample

    ; get volume
    movd    mm5,D [slRightVolume]
    movd    mm0,D [slLeftVolume]
    psllq   mm5,32
    por     mm5,mm0                       ; MM5 = rightVolume || leftVolume

    ; get filter
    mov     eax,D [slRightFilter]
    mov     edx,D [slLeftFilter]
    shl     eax,16
    or      eax,edx
    movd    mm7,eax                       ; MM7 = 0 | 0 || rightFilter | leftFilter

    ; get offset of each channel inside sound and loop thru destination buffer
    mov     W [mmRightStep],0
    movzx   eax,W [fixLeftOfs]
    movzx   edx,W [fixRightOfs]
    shl     edx,16
    or      eax,edx                       ; EAX = right ofs frac | left ofs frac
    mov     ebx,D [fixLeftOfs+2]          ; EBX = left ofs int
    mov     edx,D [fixRightOfs+2]         ; EDX = right ofs int
    mov     esi,D [pswSrcBuffer]          ; ESI = source sound buffer start ptr
    mov     edi,D [pvMixerBuffer]         ; EDI = mixer buffer ptr
    mov     ecx,D [slMixerBufferSize]     ; ECX = samples counter

sampleLoop_MixStereo:
    ; check if source offsets came to the end of source sound buffer
    cmp     ebx,D [slSoundBufferSize]
    jl      lNotEnd_MixStereo
    sub     ebx,D [slSoundBufferSize]
    push    D [bNotLoop]
    pop     D [bEndOfSound]
lNotEnd_MixStereo:
    ; same for right channel
    cmp     edx,D [slSoundBufferSize]
    jl      rNotEnd_MixStereo
    sub     edx,D [slSoundBufferSize]
    push    D [bNotLoop]
    pop     D [bEndOfSound]
rNotEnd_MixStereo:

    ; check end of sample
    cmp     ecx,0
    jle     near loopEnd_MixStereo
    cmp     D [bEndOfSound],TRUE
    je      near loopEnd_MixStereo

    ; get sound samples
    movq    mm1,Q [esi+ ebx*4]  ; MM1 = ### | nextLeftSample  || ### | leftSample             // slLeftSample = pswSrcBuffer[X+0]; nextLeftSample = pswSrcBuffer[X+2];
    movq    mm2,Q [esi+ edx*4]  ; MM2 = nextRightSample | ### ||  rightSample | ###           // slLeftSample = pswSrcBuffer[Y+3]; nextLeftSample = pswSrcBuffer[Y+3];
    pslld   mm1,16                                                                            ; << 16 -logical left shift of each of two double words
                                ; MM1 = nextLeftSample  | ### ||  leftSample  | 0
                                ; MM2 = nextRightSample | ### ||  rightSample | ###
    psrad   mm1,16              ; MM1 = 0 | nextLeftSample  || 0 | leftSample                 ; >> 16 - Arithmetic right shift of each of two double words
    psrad   mm2,16              ; MM2 = 0 | nextRightSample || 0 | rightSample                ; >> 16 - Arithmetic right shift of each of two double words

    packssdw mm1,mm2            ; MM1 = nextRightSample | rightSample || nextLeftSample | leftSample  ; Converts 2 packed signed doubleword integers from mm1 and from mm2/m64 
                                                                                                      ;into 4 packed signed word integers in mm1 using signed saturation.
    ; calc linear interpolation factor (strength)
    movd    mm3,eax   ; MM3 = 0 | 0 || right frac | left frac
    punpcklwd mm3,mm3
    psrlw   mm3,1     ; MM3 = rightFrac | rightFrac || leftFrac | leftFrac
    pxor    mm3,Q [mmInvFactor] ; MM3 = rightFrac | 1-rightFrac || leftFrac | 1-leftFrac
    ; apply linear interpolation
    pmaddwd mm1,mm3
    psrad   mm1,15
    packssdw mm1,mm1  ; MM1 = ? | ? || linearRightSample | linearLeftSample

    ; apply filter
    psubsw  mm1,mm6
    pmulhw  mm1,mm7
    psllw   mm1,1
    paddsw  mm1,mm6
    movq    mm6,mm1

    ; apply volume adjustment
    movq    mm0,mm5
    psrad   mm0,16
    packssdw mm0,mm0
    pmulhw  mm1,mm0
    psllw   mm1,1
    pxor    mm1,Q [mmSurroundFactor]
    paddd   mm5,Q [mmVolumeGain]   ; modify volume

    ; unpack to 32bit and mix it into destination buffer
    punpcklwd mm1,mm1
    psrad   mm1,16              ; MM1 = finalRightSample || finalLeftSample
    paddd   mm1,Q [edi]
    movq    Q [edi],mm1

    ; advance to next samples in source sound
    add     eax,D [mmRightStep+0]
    adc     edx,D [mmRightStep+4]
    add      ax,W [mmLeftStep +0]
    adc     ebx,D [mmLeftStep +2]
    add     edi,8
    dec     ecx
    jmp     sampleLoop_MixStereo

loopEnd_MixStereo:
    ; store modified asm local vars
    mov     D [fixLeftOfs +0],eax
    shr     eax,16
    mov     D [fixRightOfs+0],eax
    mov     D [fixLeftOfs +2],ebx
    mov     D [fixRightOfs+2],edx
    movd    eax,mm6
    mov     edx,eax
    and     eax,0x0000FFFF
    shr     edx,16
    mov     D [slLastLeftSample],eax
    mov     D [slLastRightSample],edx
    emms
    pop     edi
    pop     esi
    pop     ebx   ; Restore GCC register.
    ret

; ************************
; **   End Code Block   **
; ************************

