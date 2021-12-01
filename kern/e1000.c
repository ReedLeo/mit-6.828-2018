#include <kern/e1000.h>
#include <kern/pmap.h>

volatile
uint32_t *e1000_base_addr;

// LAB 6: Your driver code here
int pci_e1000_attach(struct pci_func* pcif) 
{
    pci_func_enable(pcif);
    e1000_base_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("in pci_e1000_attach: e1000's BAR 0 = %x, and its status register contains %x\n", e1000_base_addr, e1000_base_addr[2]);
    return 0;
}