#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>

#define TX_MAX_NDESC 64         // number of tx descriptors
#define TX_MAX_BUF_SIZE 1520    // 1518 round up to 16-algined, in bytes

// TCTL bit values
#define TCTL_EN         (1 << 1)
#define TCTL_PSP        (1 << 3)
#define TCTL_CT(x)      (((uint8_t)x) << 4)
#define TCTL_COLD(x)    ((x & 0x3ff) << 12)
#define TCTL_COLD_HALF  TCTL_COLD(512)
#define TCTL_COLD_FULL  TCTL_COLD(64)

#define TXD_CMD_EOP     1
#define TXD_CMD_RS      (1<<3)

#define TXD_STAT_DD     1

static volatile
uint32_t *e1000_base_addr;

static
struct e1000_tx_desc tx_descs[TX_MAX_NDESC];

static
uint8_t tx_bufs[TX_MAX_NDESC][TX_MAX_BUF_SIZE];

// LAB 6: Your driver code here
static
void transmit_init()
{
    for (uint32_t i = 0; i < TX_MAX_NDESC; ++i) {
        tx_descs[i].buffer_addr = PADDR(tx_bufs[i]);
        tx_descs[i].lower.flags.cmd = 0;
        tx_descs[i].upper.fields.status |= E1000_TXD_STAT_DD;
    }

    // set TDBA that consists of TDBAH:TDBAL these 2 32-bit registers.
    e1000_base_addr[E1000_TDBAL >> 2] = PADDR(tx_descs);
    e1000_base_addr[E1000_TDBAH >> 2] = 0;

    // set TDLEN to the size(in bytes) of the descriptor ring.
    e1000_base_addr[E1000_TDLEN >> 2] = sizeof(tx_descs);

    // set TDH/TDT to 0
    e1000_base_addr[E1000_TDH >> 2] = e1000_base_addr[E1000_TDT >> 2] = 0;

    // set TCTL (control register)
    e1000_base_addr[E1000_TCTL >> 2] = TCTL_EN | TCTL_PSP | TCTL_CT(0x10) | TCTL_COLD_FULL;

    // set TIPG
    e1000_base_addr[E1000_TIPG >> 2] = E1000_DEFAULT_TIPG_IPGT
                                     | (E1000_DEFAULT_TIPG_IPGR1 << E1000_TIPG_IPGR1_SHIFT)
                                     | (E1000_DEFAULT_TIPG_IPGR2 << E1000_TIPG_IPGR2_SHIFT);
}

static
void receive_init()
{

}

static
void pci_e1000_init() 
{
    transmit_init();
    receive_init();
}

int e1000_transmit(const char* buf, int size)
{
    uint32_t tdt_idx = e1000_base_addr[E1000_TDT >> 2];
    struct e1000_tx_desc* p_next_desc = tx_descs + tdt_idx;
    
    // pakcet is too big to transmit.
    if (size > TX_MAX_BUF_SIZE)
        return E_TX_PKTOF;

    if (tdt_idx >= TX_MAX_NDESC)
        panic("in e1000_transmit: get a wrong TDT value = %d", tdt_idx);
    
    if ((p_next_desc->upper.fields.status & TXD_STAT_DD) == 0) {
        // transmit queue is full.
        return -E_TX_RETRY;
    }
    
    memmove(tx_bufs[tdt_idx], buf, size);

    // update 
    p_next_desc->lower.flags.length = (uint16_t)size;
    // set RS bit in CMD field.
    p_next_desc->lower.flags.cmd |= TXD_CMD_RS | TXD_CMD_EOP;
    p_next_desc->upper.fields.status &= ~(TXD_STAT_DD);

    // update TDT
    tdt_idx = (tdt_idx + 1) % TX_MAX_NDESC;
    e1000_base_addr[E1000_TDT >> 2]  = tdt_idx;

    return size;
}

int pci_e1000_attach(struct pci_func* pcif) 
{
    pci_func_enable(pcif);
    e1000_base_addr = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    pci_e1000_init();
    // char buf[] = {"hello"};
    // e1000_transmit(buf, sizeof(buf));
    return 0;
}