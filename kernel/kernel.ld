KERNEL_VMA = 0xFFFF800000100000;		/* virtual address of kernel */

PHDRS
{
	lowmem 0x60000000;			/* tells bootloader to map memory, but not load */
	text PT_LOAD;
	data PT_LOAD;
	initrd 0x60000001;			/* tells bootloader to load the initrd here */
	percpu PT_LOAD;
	klog PT_LOAD;
}

SECTIONS
{
	.lowmem 0xFFFF800000000000 : AT(0)
	{
		*(.lowmem)
	} :lowmem
	
	. = KERNEL_VMA;
	
	.text :
	{
		code = .;
		*(.text)
		*(.rodata)
		. = ALIGN(4096);
		*(.usup_text)
	} :text
	
	. = ALIGN(4096);
	
	.data :
	{
		data = .;
		*(.data)
	} :data
	
	.bss :
	{
		bss = .;
		*(.bss)
	} :data
	
	. = ALIGN(4096);
	
	.stack (NOLOAD) :
	{
		*(.stack)
	} :data
	_stackBottom = .;
	
	. = ALIGN(4096);
	
	.initrd (NOLOAD) :
	{
		*(.initrd)
	} :initrd

	. = 0xFFFF828000000000;
	_per_cpu_start = .;
	.per_cpu_bss (NOLOAD) :
	{
		*(.data_per_cpu)
	} :percpu
	_per_cpu_end = .;
	
	. = 0xFFFF848000000000;
	.klog (NOLOAD) :
	{
		*(.klog)
	} :klog
}
