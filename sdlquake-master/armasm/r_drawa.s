                    IMPORT R_EmitEdge


                    AREA    |.text|, CODE, READONLY
                    ALIGN

_aRclipEdge         PROC
                    PUSH          {r4-r6,lr}
                    VPUSH.64      {d8-d9}
                    SUB           sp,sp,#0x10
                    MOV           r5,r0
                    MOV           r6,r1
                    MOV           r4,r2
;if (clip) 
;{ 
                    CMP           r4,#0x00
                    BEQ           emit
;do 
;{ 
                    NOP           
;d0 = DotProduct (pv0->position, clip->normal) - clip->dist; 
                    VLDR          s0,[r5,#0x00]
                    VLDR          s1,[r4,#0x00]
                    VMUL.F32      s0,s0,s1
                    VLDR          s1,[r5,#0x04]
                    VLDR          s2,[r4,#0x04]
                    VMLA.F32      s0,s1,s2
                    VLDR          s1,[r5,#0x08]
                    VLDR          s2,[r4,#0x08]
                    VMLA.F32      s0,s1,s2
                    VLDR          s1,[r4,#0x0C]
                    VSUB.F32      s17,s0,s1
;d1 = DotProduct (pv1->position, clip->normal) - clip->dist; 
                    VLDR          s0,[r6,#0x00]
                    VLDR          s1,[r4,#0x00]
                    VMUL.F32      s0,s0,s1
                    VLDR          s1,[r6,#0x04]
                    VLDR          s2,[r4,#0x04]
                    VMLA.F32      s0,s1,s2
                    VLDR          s1,[r6,#0x08]
                    VLDR          s2,[r4,#0x08]
                    VMLA.F32      s0,s1,s2
                    VLDR          s1,[r4,#0x0C]
                    VSUB.F32      s18,s0,s1
;if (d0 >= 0) 
;{ 
;// point 0 is unclipped 
                    VCMPE.F32     s17,#0.0
                    VMRS          APSR_nzcv,FPSCR
                    BLT           0x2004332A
;if (d1 >= 0) 
;{ 
;// both points are unclipped 
                    VCMPE.F32     s18,#0.0
                    VMRS          APSR_nzcv,FPSCR
                    BLT           0x20043296
;continue; 
;} 
;// only point 1 is clipped 
;// we don't cache clipped edges 
                    B             0x200433D8
;cacheoffset = 0x7FFFFFFF; 
                    MVN           r0,#0x80000000
                    LDR           r1,[pc,#336]  ; @0x200433EC
                    STR           r0,[r1,#0x00]
;f = d0 / (d0 - d1); 
                    VSUB.F32      s0,s17,s18
                    VDIV.F32      s16,s17,s0
;clipvert.position[0] = pv0->position[0] + 
;f * (pv1->position[0] - pv0->position[0]); 
                    VLDR          s0,[r6,#0x00]
                    VLDR          s1,[r5,#0x00]
                    VSUB.F32      s1,s0,s1
                    VLDR          s0,[r5,#0x00]
                    VMLA.F32      s0,s16,s1
                    VSTR          s0,[sp,#0x04]
;clipvert.position[1] = pv0->position[1] + 
;f * (pv1->position[1] - pv0->position[1]); 
                    VLDR          s0,[r6,#0x04]
                    VLDR          s1,[r5,#0x04]
                    VSUB.F32      s1,s0,s1
                    VLDR          s0,[r5,#0x04]
                    VMLA.F32      s0,s16,s1
                    VSTR          s0,[sp,#0x08]
;clipvert.position[2] = pv0->position[2] + 
;f * (pv1->position[2] - pv0->position[2]); 
                    VLDR          s0,[r6,#0x08]
                    VLDR          s1,[r5,#0x08]
                    VSUB.F32      s1,s0,s1
                    VLDR          s0,[r5,#0x08]
                    VMLA.F32      s0,s16,s1
                    VSTR          s0,[sp,#0x0C]
;if (clip->leftedge) 
;{ 
                    LDRB          r0,[r4,#0x14]
                    CBZ           r0,0x20043302
;r_leftclipped = true; 
                    MOVS          r0,#0x01
                    LDR           r1,[pc,#248]  ; @0x200433F0
                    STR           r0,[r1,#0x00]
;r_leftexit = clipvert; 
;} 
                    LDR           r0,[pc,#248]  ; @0x200433F4
                    ADD           r1,sp,#0x04
                    LDM           r1,{r1-r3}
                    STM           r0!,{r1-r3}
                    B             0x20043314
;else if (clip->rightedge) 
;{ 
                    LDRB          r0,[r4,#0x15]
                    CBZ           r0,0x20043314
;r_rightclipped = true; 
                    MOVS          r0,#0x01
                    LDR           r1,[pc,#236]  ; @0x200433F8
                    STR           r0,[r1,#0x00]
;r_rightexit = clipvert; 
;} 
                    LDR           r0,[pc,#236]  ; @0x200433FC
                    ADD           r1,sp,#0x04
                    LDM           r1,{r1-r3}
                    STM           r0!,{r1-r3}
;R_ClipEdge (pv0, &clipvert, clip->next); 
                    ADD           r1,sp,#0x04
                    MOV           r0,r5
                    LDR           r2,[r4,#0x10]
                    BL.W          R_ClipEdge (0x20043214)
emit                MOV           r1,r6
                    MOV           r0,r5
                    BL.W          R_EmitEdge
                    VPOP.64       {d8-d9}
                    POP           {r4-r6,pc}
                    BX
                    ENDP
;} 
