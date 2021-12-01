#include <kern/e1000.h>
#include <kern/pmap.h>

#define TX_MAX_NDESC 64         // number of tx descriptors
#define TX_MAX_BUF_SIZE 1520    // 1518 round up to 16-algined, in bytes

// TCTL bit values
#define TCTL_EN         (1 << 1)
#define TCTL_PSP        (1 << 3)
#define TCTL_CT(x)      (((uint8_t)x) << 4)
#define TCTL_COLD(x)    ((x & 0x3ff) << 12)
#define TCTL_COLD_HALF  TCTL_COLD(512)
#define TCTL_COLD_FULL  TCTL_COLD(64)

static volatile
uint32_t *e1000_base_addr;

static
struct e1000_tx_desc tx_descs[TX_MAX_NDESC];

static
uint8_t tx_bufs[TX_MAX_NDESC][TX_MAX_BUF_SIZE];

// LAB 6: Your driver code here
static
void pci_e1000_init() 
{
    for (uint32_t i = 0; i < TX_MAX_NDESC; ++i) {
        tx_descs[i].buffer_addr = PADDR(tx_bufs[i]);
    }

    // set TDBA that consists of TDBAH:TDBAL these 2 32-bit registers.
    e1000_base_addr[E1000_TDBAL >> 2] = PADDR(tx_descs);
    e1000_base_addr[E1000_TDBAH >> 2] = 0;

    // set TDLEN to the size(in bytes) of the descriptor ring.
    e1000_base_addr[E1000_TDLEN >> 2] = sizeof(tx_descs);

    // set TDH/TDT to 0
    e1000_base_addr[E1000_TDH >> 2] = e1000_base_addr[E1000_TDT >> 2] = 0;

    // set TCTL (control register)
    e1000_base_addr[E1000_TXDCTL >> 2] = TCTL_EN | TCTL_PSP | TCTL_CT(0x10) | TCTL_COLD_FULL;

    // set TIPG
    e1000_base_addr[E1000_TIPG >> 2] = E1000_DEFAULT_TIPG_IPGT
                                     | (E1000_DEFAULT_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT)
                                     | (E1000_DEFAULT_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT);
}

int pci_e1000_attach(struct pci_func* pcif) 
{
    pci_func_enable(pcif);
    e1000_base_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    pci_e1000_init();
    return 0;
}