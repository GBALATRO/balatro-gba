/* Linker Script Original v1.3 by Jeff Frohwein     */
/*  v1.0 - Original release                         */
/*  v1.1 - Added proper .data section support       */
/*  v1.2 - Added support for c++ & iwram overlays   */
/*       - Major contributions by Jason Wilkins.    */
/*  v1.3 - .ewram section now can be used when      */
/*         compiling for MULTIBOOT mode. This fixes */
/*         malloc() in DevKitAdvance which depends  */
/*         on __eheap_start instead of end to define*/
/*         the starting location of heap space.     */
/*         External global variable __gba_iwram_heap*/
/*         support added to allow labels end, _end, */
/*         & __end__ to point to end of iwram or    */
/*         the end of ewram.                        */
/*	Additions by WinterMute				*/
/* v1.4 -	.sbss section added for unitialised	*/
/*		    data in ewram 			*/
/* v1.5 -	padding section added to stop EZF 	*/
/*		    stripping important data		*/

/* This file is released into the public domain		*/
/* for commercial or non-commercial use with no		*/
/* restrictions placed upon it.				*/

/* NOTE!!!: This linker script defines the RAM &  */
/*   ROM start addresses. In order for it to work */
/*   properly, remove -Ttext and -Tbss linker     */
/*   options from your makefile if they are       */
/*   present.                                     */

/* You can use the following to view section      */
/* addresses in your .elf file:                   */
/*   objdump -h file.elf                          */
/* Please note that empty sections may incorrectly*/
/* list the lma address as the vma address for    */
/* some versions of objdump.                      */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
/* SEARCH_DIR(/bin/arm); */

/* The linker script function "var1 += var2;" sometimes    */
/* reports incorrect values in the *.map file but the      */
/* actual value it calculates is usually, if not always,   */
/* correct. If you leave out the ". = ALIGN(4);" at the    */
/* end of each section then the return value of SIZEOF()   */
/* is sometimes incorrect and "var1 += var2;" appears to   */
/* not work as well. "var1 += var2" style functions are    */
/* avoided below as a result.                              */

MEMORY {

	rom	: ORIGIN = 0x08000000, LENGTH = 32M
	iwram	: ORIGIN = 0x03000000, LENGTH = 32K
	ewram	: ORIGIN = 0x02000000, LENGTH = 256K
}

__text_start	=	ORIGIN(rom);
__eheap_end	=	ORIGIN(ewram) + LENGTH(ewram);
__iwram_start	=	ORIGIN(iwram);
__iwram_top	=	ORIGIN(iwram) + LENGTH(iwram);;
__sp_irq	=	__iwram_top - 0x060;
__sp_usr	=	__sp_irq - 0x0a0;
__irq_flags	=	0x03007ff8;

SECTIONS
{
	. = __text_start;
	.crt0 :
	{
		KEEP (*(.crt0))
		. = ALIGN(4);
	} >rom =0xff


	.init :
	{
		KEEP (*(SORT_NONE(.init)))
	} >rom

	.plt :
	{
		*(.plt)
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom

	.text  :   /* ALIGN (4): */
	{
		*(EXCLUDE_FILE (*.iwram*) .text*)
		*(.gnu.linkonce.t.*)
		KEEP (*(.text.*personality*))
		/* .gnu.warning sections are handled specially by elf32.em.  */
		*(.gnu.warning)
		*(.glue_7t) *(.glue_7) *(.vfp11_veneer)
		. = ALIGN(4);  /* REQUIRED. LD is flaky without it. */
	} >rom = 0xff

	__text_end = .;
	.fini           :
	{
		KEEP (*(.fini))
		. = ALIGN(4);  /* REQUIRED. LD is flaky without it. */
	} >rom =0

	.rodata :
	{
		*(.rodata)
		*all.rodata*(*)
		*(.roda)
		*(.rodata.*)
		*(.gnu.linkonce.r*)
		SORT(CONSTRUCTORS)
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom = 0xff
    .jokers :
    {
        __jokers_start = .;
        KEEP(*(.jokers))
        __jokers_end = .;
    } >rom 
	.ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >rom
	__exidx_start = .;
	.ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >rom
	__exidx_end = .;

	.ctors :
	{
		/*	gcc uses crtbegin.o to find the start of the constructors, so
			we make sure it is first.  Because this is a wildcard, it
			doesn't matter if the user does not actually link against
			crtbegin.o; the linker won't look for a file to match a
			wildcard.  The wildcard also means that it doesn't matter which
			directory crtbegin.o is in.  */
		KEEP (*crtbegin.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*(.ctors))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom = 0

	.dtors :
	{
		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*(.dtors))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom = 0


	.eh_frame :
	{
		KEEP (*(.eh_frame))
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom = 0

	.gcc_except_table :
	{
		*(.gcc_except_table)
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
	} >rom = 0

	__iwram_lma = .;

	.iwram __iwram_start : AT (__iwram_lma)
	{
		__iwram_start__ = ABSOLUTE(.) ;
		*(.iwram .iwram*)
		*iwram.*(.text* .data*)
		. = ALIGN(4);   /* REQUIRED. LD is flaky without it. */
		__iwram_end__ = ABSOLUTE(.) ;
	} >iwram = 0xff

	__data_lma = __iwram_lma + SIZEOF(.iwram) ;

	.bss ALIGN(4) (NOLOAD) :
	{
		__bss_start = ABSOLUTE(.);
		__bss_start__ = ABSOLUTE(.);
		*(.dynbss)
		*(.gnu.linkonce.b*)
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);    /* REQUIRED. LD is flaky without it. */
		__bss_end__ = ABSOLUTE(.);

	} AT>iwram

	.data ALIGN(4) : AT (__data_lma)
	{
		__data_start__ = ABSOLUTE(.);
		*(.data*)
		*(.gnu.linkonce.d*)
		CONSTRUCTORS
		. = ALIGN(4);
	} >iwram = 0xff

	__preinit_lma = __data_lma + SIZEOF(.data);

	.preinit_array ALIGN(4)    : AT (__preinit_lma)
	{
		__preinit_array_start = ABSOLUTE(.);
		KEEP (*(.preinit_array))
		__preinit_array_end = ABSOLUTE(.);
	} >iwram

	__init_lma = __preinit_lma + SIZEOF(.preinit_array);

	.init_array  ALIGN(4)   : AT (__init_lma)
	{
		__init_array_start = ABSOLUTE(.);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = ABSOLUTE(.);
	} >iwram

	__fini_lma = __init_lma + SIZEOF(.init_array);

	.fini_array  ALIGN(4)   : AT (__fini_lma)
	{
		__fini_array_start = ABSOLUTE(.);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array))
  		__fini_array_end = ABSOLUTE(.);
	} >iwram

	__jcr_lma = __fini_lma + SIZEOF(.fini_array);
	.jcr        ALIGN(4)    : AT (__jcr_lma) { KEEP (*(.jcr)) } >iwram

	__data_end__  =  ABSOLUTE(.);
	__iwram_overlay_lma = __jcr_lma + SIZEOF(.jcr);

	__iwram_overlay_start = . ;

	OVERLAY ALIGN(4) : NOCROSSREFS AT (__iwram_overlay_lma)
	{
		.iwram0 { *(.iwram0) . = ALIGN(4);}
		.iwram1 { *(.iwram1) . = ALIGN(4);}
		.iwram2 { *(.iwram2) . = ALIGN(4);}
		.iwram3 { *(.iwram3) . = ALIGN(4);}
		.iwram4 { *(.iwram4) . = ALIGN(4);}
		.iwram5 { *(.iwram5) . = ALIGN(4);}
		.iwram6 { *(.iwram6) . = ALIGN(4);}
		.iwram7 { *(.iwram7) . = ALIGN(4);}
		.iwram8 { *(.iwram8) . = ALIGN(4);}
		.iwram9 { *(.iwram9) . = ALIGN(4);}
	}>iwram = 0xff

	__iwram_overlay_end = . ;
	__ewram_lma = __iwram_overlay_lma + (__iwram_overlay_end - __iwram_overlay_start) ;

	__iheap_start = . ;

	__ewram_start = ORIGIN(ewram);
	.ewram __ewram_start : AT (__ewram_lma)
	{
		*(.ewram*)
		. = ALIGN(4);  /* REQUIRED. LD is flaky without it. */
		__ewram_end = ABSOLUTE(.);
	}>ewram = 0xff

	__pad_lma = __ewram_lma + SIZEOF(.ewram);

	.sbss ALIGN(4)(NOLOAD):
 	{
		__sbss_start__ = ABSOLUTE(.);
 		*(.sbss*)
 		. = ALIGN(4);
		__sbss_end__  = ABSOLUTE(.);
		__end__ = ABSOLUTE(.);
		__eheap_start = ABSOLUTE(.);
 	} AT>ewram

	/* EZF Advance strips trailing 0xff bytes, add a pad section so nothing important is removed */
	.pad ALIGN(4) : AT (__pad_lma)
	{
		LONG(0x52416b64)
		LONG(0x4d)
		. = ALIGN(4);  /* REQUIRED. LD is flaky without it. */
	} = 0xff
	__rom_end__ = __pad_lma + SIZEOF(.pad);


	/* Stabs debugging sections.  */
	.stab 0 : { *(.stab) }
	.stabstr 0 : { *(.stabstr) }
	.stab.excl 0 : { *(.stab.excl) }
	.stab.exclstr 0 : { *(.stab.exclstr) }
	.stab.index 0 : { *(.stab.index) }
	.stab.indexstr 0 : { *(.stab.indexstr) }
	.comment 0 : { *(.comment) }
	/*	DWARF debug sections.
		Symbols in the DWARF debugging sections are relative to the beginning
		of the section so we begin them at 0.  */
	/* DWARF 1 */
	.debug          0 : { *(.debug) }
	.line           0 : { *(.line) }
	/* GNU DWARF 1 extensions */
	.debug_srcinfo  0 : { *(.debug_srcinfo) }
	.debug_sfnames  0 : { *(.debug_sfnames) }
	/* DWARF 1.1 and DWARF 2 */
	.debug_aranges  0 : { *(.debug_aranges) }
	.debug_pubnames 0 : { *(.debug_pubnames) }
	/* DWARF 2 */
	.debug_info     0 : { *(.debug_info) }
	.debug_abbrev   0 : { *(.debug_abbrev) }
	.debug_line     0 : { *(.debug_line) }
	.debug_frame    0 : { *(.debug_frame) }
	.debug_str      0 : { *(.debug_str) }
	.debug_loc      0 : { *(.debug_loc) }
	.debug_macinfo  0 : { *(.debug_macinfo) }
	/* SGI/MIPS DWARF 2 extensions */
	.debug_weaknames 0 : { *(.debug_weaknames) }
	.debug_funcnames 0 : { *(.debug_funcnames) }
	.debug_typenames 0 : { *(.debug_typenames) }
	.debug_varnames  0 : { *(.debug_varnames) }
	.stack 0x80000 : { _stack = .; *(.stack) }
	/* These must appear regardless of  .  */
  .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
  .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) }
  /DISCARD/ : { *(.note.GNU-stack) }
}
