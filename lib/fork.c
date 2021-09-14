// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800
#define IDX_PTE(x)	((uintptr_t)x >> PTXSHIFT)

extern pde_t* uvpd;
extern pte_t* uvpt;

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

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if ((err & PTE_W) && (uvpt[IDX_PTE(addr)] & PTE_COW)) {
		
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	panic("pgfault not implemented");
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
	struct Env* e = NULL;
	void* va = (void*)(pn*PGSIZE);
	int perm = 0;
	// LAB 4: Your code here.
	e = &envs[ENVX(envid)];
	// check target envid is valid.
	if (e->env_status == ENV_FREE || e->env_id != envid)
		return -E_BAD_ENV;

	// get all perm bits of current page table.
	perm = PGOFF(uvpt[IDX_PTE(va)]);
	if (perm & (PTE_COW | PTE_W))
		perm |= PTE_COW;
	
	r = sys_page_map(0, va, envid, va, perm);
	if (r < 0)
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
	set_pgfault_handler(pgfault);
	eid = sys_exofork();
	if (eid < 0)
		return eid;
	if (eid) { // parent
		for (size_t pn = 0; pn < PGNUM(UTOP); ++pn) {
			if (1) {
				duppage(eid, pn);
			}
		}
	} else { // child
		thisenv = &envs[sys_getenvid()];
	}

	return eid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
