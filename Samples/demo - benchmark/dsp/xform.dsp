; Vertex transformation by the 3x3 part of a matrix (the CPU adds the translation)
; Data RAM:
;   bank 0: M0[0..8] = m00 m01 m02 m10 m11 m12 m20 m21 m22 (16.16 fixed point)
;           M0[60] = source address / 4, M0[61] = destination address / 4, M0[62] = vertex count - 1, M0[63] = 3 * vertex count
;   bank 1: vertices x y z (21 at most, loaded by DMA)
;   bank 2: M2[2 * i] = M2[2 * i + 1] = 3 * i (first word of the vertex i in the bank 1)
;   bank 3: transformed vertices (sent by DMA)
    d1=#60,ct0
    d1=mc0,ra0
    d1=mc0,wa0
    d1=mc0,lop
    d1=#0,ct1
    dma d0,mc1,m0
wait_in:
    jmp t0,wait_in
    nop
    d1=#0,ct0
    d1=#0,ct1
    d1=#0,ct2
    d1=#0,ct3
    d1=#loop,top
    x=mc0 y=mc1
    x=mc0 y=mc1 p=mul a=0
loop:
    ad2 x=mc0 y=mc1 p=mul a=alu d1=mc2,ct1
    ad2 x=mc0 y=mc1 p=mul a=alu
    ad2 x=mc0 y=mc1 p=mul a=0 d1=alh,mc3
    ad2 x=mc0 y=mc1 p=mul a=alu d1=mc2,ct1
    ad2 x=mc0 y=mc1 p=mul a=alu
    ad2 x=mc0 y=mc1 p=mul a=0 d1=alh,mc3
    ad2 x=mc0 y=mc1 p=mul a=alu d1=#0,ct0
    ad2 x=mc0 y=mc1 p=mul a=alu
    btm
    ad2 x=mc0 y=mc1 p=mul a=0 d1=alh,mc3
    d1=#0,ct3
    d1=#63,ct0
    dma mc3,d0,m0
wait_out:
    jmp t0,wait_out
    nop
    end
