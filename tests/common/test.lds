OUTPUT_ARCH( "riscv" )

ENTRY( reset_vector )

SECTIONS
{

  /*--------------------------------------------------------------------*/
  /* Code and read-only segment                                         */
  /*--------------------------------------------------------------------*/

  /* Begining of code and text segment */
  . = 0x80000000;
  _ftext = .;
  PROVIDE( eprol = . );

  _global_start = .;
  .text :
  {
    *(.text.init)
  }

  /* text: Program code section */
  .text : 
  {
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t.*)
  }

  /* rodata: Read-only data */
  . = ALIGN(8);
  _data_start = .;
  .rodata : 
  {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r.*)
  }
  
  /* End of code and read-only segment */
  PROVIDE( etext = . );
  _etext = .;

  /*--------------------------------------------------------------------*/
  /* HTIF, isolated onto separate page                                  */
  /*--------------------------------------------------------------------*/
  . = ALIGN(0x1000);
  htif :
  {
    *(htif)
  }
  . = ALIGN(0x1000);

  /*--------------------------------------------------------------------*/
  /* Initialized data segment                                           */
  /*--------------------------------------------------------------------*/

  /* Start of initialized data segment */
  . = ALIGN(16);
   _fdata = .;

  /* data: Writable data */
  .data : 
  {
    *(.data)
    *(.data.*)
    *(.srodata*)
    *(.gnu.linkonce.d.*)
    *(.comment)
  }

  /* End of initialized data segment */
  . = ALIGN(4);
  PROVIDE( edata = . );
  _edata = .;

  /*--------------------------------------------------------------------*/
  /* Uninitialized data segment                                         */
  /*--------------------------------------------------------------------*/

  /* Start of uninitialized data segment */
  . = .;
  _fbss = .;

  /* sbss: Uninitialized writeable small data section */
  . = .;

  /* bss: Uninitialized writeable data section */
  . = .;
  _bss_start = .;
  .bss : 
  {
    *(.bss)
    *(.bss.*)
    *(.sbss*)
    *(.gnu.linkonce.b.*)
    *(COMMON)
  }
  . = . + 1;
  . = ALIGN(8);
  _data_end = .;
  
  . = ALIGN(8);
  _test_start = .;
  .test.text :
  {
    *(.test)
    *(.test.*)
  }
  . = . + 1;
  . = ALIGN(8);
  _test_end = .;
  
  _global_end = .;
  
  . = ALIGN(8);
  _section_start_a = .;
  .section.a :
  {
    *(.section.a)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_a = .;
  
  . = ALIGN(8);
  _section_start_b = .;
  .section.b :
  {
    *(.section.b)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_b = .;
  
  . = ALIGN(8);
  _section_start_c = .;
  .section.c :
  {
    *(.section.c)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_c = .;
  
  . = ALIGN(8);
  _section_start_d = .;
  .section.d :
  {
    *(.section.d)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_d = .;
  
    . = ALIGN(8);
  _section_user_start = .;
  .section.user :
  {
    *(.section.user)
    *(.section.user.*)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_user_end = .;
  
  . = ALIGN(8);
  _section_start_trusted = .;
  .section.trusted :
  {
    *(.section.trusted)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_trusted = .;

  . = ALIGN(8);
  _section_start_normal = .;
  .section.normal :
  {
    *(.section.normal)
  }
  . = . + 1;
  . = ALIGN(8);
  _section_end_normal = .;
  
  _end = .;
  _mstack_start = .;
  . = ALIGN(0x1000);
  . = . + 1;
  . = ALIGN(0x1000);
  _mstack_end = .;
}
