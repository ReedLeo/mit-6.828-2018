#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#include <kern/pci.h>

int pci_e1000_attach(struct pci_func* pcif);

#endif  // SOL >= 6
