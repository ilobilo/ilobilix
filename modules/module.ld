ENTRY(0)

PHDRS
{
    null        PT_NULL;
    text        PT_LOAD     FLAGS(5);
    rodata      PT_LOAD     FLAGS(4);
    data        PT_LOAD     FLAGS(6);
    dynamic     PT_DYNAMIC  FLAGS(6);
    modules     0x60000001;
}

SECTIONS
{
    .text : {
        *(.init)
        *(.text)
        *(.text.*)
        *(.fini)
        *(.plt)
        *(.plt.*)
    } :text

    .rodata : {
        *(.rodata)
        *(.rodata.*)
        *(.got)
        *(.got.*)
    } :rodata

    .modules : {
        KEEP(*(.modules))
    } :rodata :modules

    .data : {
        *(.data)
        *(.data.*)
    } :data

    .bss : {
        *(.*bss)
        *(.*bss.*)
        *(COMMON)
    } :data

    .dynsym : {
        *(.dynsym)
    } :data

    .dynstr : {
        *(.dynstr)
    } :data

    .rela.dyn : {
        *(.rela.dyn)
    } :data

    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    /DISCARD/ :
    {
        *(.comment*)
        *(.eh_frame*)
        *(.rel.eh_frame*)
        *(.gcc_except_table*)
        *(.note*)
        *(.interp)
    }
}