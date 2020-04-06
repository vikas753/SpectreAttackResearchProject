
monitor:

  NOP     
  NOP     
  B       SMC_Handler
  NOP     
  NOP     
  NOP     
  NOP     
  NOP     


SMC_Handler:
  PUSH   {r0-r3}                       
                 
  CPS     0x16    				 

  MRC     p15, 0, r0, c1, c1, 0        
  TST     r0, #0x1                  
  EOR     r0, r0, #0x1              
  MCR     p15, 0, r0, c1, c1, 0        


  CLREX                               
                                      
  POP     {r0-r3}

  MOVS    pc, lr

