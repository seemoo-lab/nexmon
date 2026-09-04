MEMORY
{
	INCLUDE gen/memory.ld
}

SECTIONS
{
	INCLUDE gen/flashpatches.ld
	INCLUDE gen/nexmon.ld

	/* Objects that carry a TARGETREGION pragma get their .bss placed into a
	 * real region by nexmon.ld (via nexmon.ld.awk); everything else's .bss
	 * would otherwise be an orphan section with no memory region, which the
	 * linker places at an unchecked address that can silently overlap a real
	 * region. These firmware patches are freestanding and have no C runtime to
	 * zero-initialise static storage, so any non-empty .bss here is a bug.
	 * Collect the stragglers and assert that they are empty. */
	.bss (NOLOAD) : { *(.bss .bss.*) *(COMMON) }
	ASSERT(SIZEOF(.bss) == 0,
		"error: firmware patch has a non-empty .bss; patches cannot rely on zero-initialised static storage -- initialise the variable explicitly or give the object a TARGETREGION")
}
