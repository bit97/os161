#include <test.h>
#include <vm.h>
#include <lib.h>

/**
 * Integer percentage calculation
 * @param a     Partial parameter
 * @param b     Total parameter
 * @return      Percentage of a over b
 */
static
unsigned
perc(unsigned a, unsigned b)
{
  return (100*a + b/2)/b;
}

/**
 * Provides statistics about RAM usage and contiguous block of pages allocatable
 * @param nargs     Unused
 * @param args      Unused
 * @return          Success value
 */
int
memstats(int nargs, char **args)
{
  (void)nargs;
  (void)args;
  
  unsigned total_ram, kernel_ram;
  unsigned total_pages, alloc_pages;
  unsigned leaked_pages;
  
  total_ram = ram_gettotal();
  kernel_ram = ram_getkernel();
  total_pages = ram_gettotalpages();
  alloc_pages = ram_getallocatedpages();
  leaked_pages = ram_leaked();
  
  kprintf("Kernel VM statistics:\n\n");
  kprintf("Total RAM:                  %u KiB\n", total_ram);
  kprintf("Kernel RAM:                 %u KiB (%d%%)\n",
          kernel_ram,
          perc(kernel_ram, total_ram));
  kprintf("Allocatable RAM:            %u KiB (%d%%)\n",
          total_ram - kernel_ram,
          perc(total_ram - kernel_ram, total_ram));
  kprintf("\n");
  kprintf("Total allocatable pages:    %u\n", total_pages);
  kprintf("Total allocated pages:      %u (%d%%)\n",
          alloc_pages,
          perc(alloc_pages, total_pages));
  kprintf("\n");
  if (leaked_pages > 0) {
  kprintf("Leakage detected:           %u pages\n", leaked_pages);
  } else {
  kprintf("No leakage detected\n");
  }

  kprintf("\n");
  kprintf("memstats completed.. closing");
  kprintf("\n");

  return 0;
}