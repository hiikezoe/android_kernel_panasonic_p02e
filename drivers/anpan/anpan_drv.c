#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/laputa-fixaddr.h>

#ifndef DMEM_PANIC_FLAG
#define DMEM_PANIC_FLAG 0xBFF65020
#endif
#ifndef DMEM_PANIC_FLAG_SIZE
#define DMEM_PANIC_FLAG_SIZE 32
#endif

unsigned long *anpan_base = NULL;

static int __init anpan_init(void)
{
        anpan_base = ioremap(DMEM_PANIC_FLAG, DMEM_PANIC_FLAG_SIZE);
        printk(KERN_INFO " anpan ioremap-address %p\n ", anpan_base);

        return 0;
}
late_initcall(anpan_init);

