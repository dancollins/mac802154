/* This linker script was made using the RIOT OS, Contiki OS, TI and libnarm
 * linker scripts as reference. */

ENTRY(ResetISR)

/* Point this to the end of RAM */
_estack = 0x20000000 + 0x00007FFF;

/* This will create a linker error if the heap and stack wont fit in RAM */
_Min_Heap_Size = 8192;
_Min_Stack_Size = 512;

/* Pointer to where we expect the end of the stack to be */
stack_end = _estack - _Min_Stack_Size;

MEMORY
{
	FLASH (RX)  : ORIGIN = 0x00200000, LENGTH = 512K - 44
	CCA   (RX)  : ORIGIN = 0x0027FFD4, LENGTH = 44
	SRAM  (RWX) : ORIGIN = 0x20000000, LENGTH = 32K
}

SECTIONS
{
	/* Ensure the vector tables are at the start of FLASH and RAM */
	.vectors :
	{
		KEEP(*(.vectors .vectors.*))
	} > FLASH= 0

	.vtable :
	{
		KEEP(*(.vtable))
	} > SRAM= 0

	.text :
	{
		/* Start of text section */
		. = ALIGN(4);
		_text = .;
		*(.text .text.* .gnu.linkonce.t.*)
		*(.glue_7t) *(.glue_7)
		*(.rodata .rodata* .gnu.linkonce.r.*)
		*(.ARM.extab* .gnu.linkonce.armextab.*)

		/* Support c constructors and destructors in user code and
		 * in the C library. Also provides support for C++ */
		. = ALIGN(4);
		KEEP(*(.init))
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;

		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;

		. = ALIGN(0x4);
		KEEP (*crtbegin.o(.ctors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .ctors))
		KEEP (*(SORT(.ctors.*)))
		KEEP (*crtend.o(.ctors))

		. = ALIGN(4);
		KEEP(*(.fini))

		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;

		KEEP (*crtbegin.o(.dtors))
		KEEP (*(EXCLUDE_FILE (*crtend.o) .dtors))
		KEEP (*(SORT(.dtors.*)))
		KEEP (*crtend.o(.dtors))

		/* End of text section */
		. = ALIGN(4);
		_efixed = .;
	} > FLASH

	/* .ARM.exidx is sorted, so has to go in its own output section.  */
	PROVIDE_HIDDEN (__exidx_start = .);
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} > FLASH
	PROVIDE_HIDDEN (__exidx_end = .);

	/* exception handling */
	. = ALIGN(4);
	.eh_frame :
	{
		KEEP (*(.eh_frame))
	} > FLASH

	. = ALIGN(4);
	_etext = .;

	.flashcca :
	{
		KEEP(*(.flashcca))
	} > CCA

	.data :
	{
		. = ALIGN(4);
		_data = .;
		*(.data .data.*)
		. = ALIGN(4);
		_edata = .;
	} > SRAM AT > FLASH

	.bss :
	{
		. = ALIGN(4);
		_bss = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	}

	.heap :
	{
		. = ALIGN(4);
		PROVIDE (end = .);
		. = . + _Min_Heap_Size;
		. = . + _Min_Stack_Size;
	} > SRAM
}

