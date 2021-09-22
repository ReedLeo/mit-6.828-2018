// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
#define IDX_PTE(x)	((uintptr_t)x >> PTXSHIFT)

extern volatile pde_t uvpd[];
extern volatile pte_t uvpt[];
extern void _pgfault_upcall(void);
//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;
	pde_t pde = 0;
	pte_t pte = 0;
	int perm = 0;
	void* pg_addr = NULL;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pde = uvpd[PDX(addr)];
	if ((pde & PTE_P) == 0)
		panic("[%x] not mapped: pde=%x\n", addr, pde);
	
	pte = uvpt[IDX_PTE(addr)];
	if ((pte & PTE_P) == 0)
		panic("[%x] not mapped: pte=%x\n", addr, pte);
	
	if (!(err & FEC_WR) || (pte & PTE_AVAIL) != PTE_COW)
		panic("[%x] is not a writable copy-on-wirte page. err=%x, perm=%x\n", addr, err, PGOFF(pte));

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	perm = PGOFF(pde) & PGOFF(pte);
	perm &= ~PTE_COW; 
	perm |= PTE_W;	// private writable page.
	pg_addr = ROUNDDOWN(addr, PGSIZE);

	if ((r = sys_page_alloc(0, PFTEMP, perm)) < 0)
		panic("pgfault: allocate page failed (%e)\n", r);

	memmove(PFTEMP, pg_addr, PGSIZE);

	if ((r = sys_page_map(0, PFTEMP, 0, pg_addr, perm)) < 0)
		panic("pgfault: map PGTEMP back to %x failed (%e)\n", pg_addr, r);

	// if ((r = sys_page_unmap(0, PFTEMP)) < 0)
	// 	panic("pgfault: unmap PFTEMP from current address space failed. (%e)\n", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void* va = (void*)(pn*PGSIZE);
	pde_t pde = 0;
	pte_t pte = 0;
	int perm = 0;
	// LAB 4: Your code here.

	// get all perm bits of current page table.
	pde = uvpd[PDX(va)];
	perm = PGOFF(pde);
	assert(pde & PTE_P);
	
	pte = uvpt[IDX_PTE(va)];
	assert(pte & PTE_P);
	
	perm &= PGOFF(pte);
	perm = PGOFF(uvpt[IDX_PTE(va)]);
	assert(perm & (PTE_COW | PTE_W));

	// Copy-on-Write page is read-only.
	perm |= PTE_COW;
	perm &= ~PTE_W;

	if ((r = sys_page_map(0, va, envid, va, perm)) < 0)
		return r;
	
	// self's mapping mapped PTE_COW
	r = sys_page_map(0, va, 0, va, perm);
	return r;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t eid = 0;
	pte_t pte = 0;
	pde_t pde = 0;
	int perm = 0;
	int r = 0;

	set_pgfault_handler(pgfault);
	eid = sys_exofork();
	if (eid < 0)
		return eid;
	if (eid == 0) { // child
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// we are parent.
	for (size_t pg = 0; pg < UTOP; pg += PGSIZE) {
		pde = uvpd[PDX(pg)];
		if (!(pde & PTE_P))
			continue;
		pte = uvpt[IDX_PTE(pg)];
		if (!(pte & PTE_P))
			continue;
		if (pg == UXSTACKTOP - PGSIZE)
			continue;

		perm = PGOFF(pte);
		if (perm & (PTE_W | PTE_COW)) {
			if ((r = duppage(eid, PGNUM(pg))) < 0)
				panic("duppage failed.(%e)\n", r);
		}
		else {
			if ((r = sys_page_map(0, (void*)pg, eid, (void*)pg, perm)) < 0)
				panic("fork: map shared page(%x) failed. %e\n", pg, r);
		}
	}

	// allocate exception stack for the child.
	if ((r = sys_page_alloc(eid, (char*)(UXSTACKTOP - PGSIZE), PTE_P|PTE_W|PTE_U)) < 0)
		panic("fork: allocate physical page for user exception stack failed. %e\n", r);
	
	if ((r = sys_env_set_pgfault_upcall(eid, _pgfault_upcall)) <0)
		panic("fork: set page fault for the child[%x] failed. %e\n", eid, r);

	sys_env_set_status(eid, ENV_RUNNABLE);
	return eid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
