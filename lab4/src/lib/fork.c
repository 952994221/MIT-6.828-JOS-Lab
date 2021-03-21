// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];
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

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if((err&FEC_WR)==0 || (uvpt[PGNUM(addr)]&PTE_COW)==0)
		panic("pgfault: Invalid user trapframe\n");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	//panic("pgfault not implemented");
	envid_t envid = sys_getenvid();
	r = sys_page_alloc(envid, (void*)PFTEMP, PTE_U|PTE_P|PTE_W);
	if(r < 0)
		panic("pgfault: Page alloc failed\n");

	addr = ROUNDDOWN(addr, PGSIZE);
	memmove(PFTEMP, addr, PGSIZE);

	r = sys_page_unmap(envid, addr);
	if(r < 0)
		panic("pgfault: Page unmap failed\n");

	r = sys_page_map(envid, PFTEMP, envid, addr, PTE_U|PTE_P|PTE_W);
	if(r < 0)
		panic("pgfault: Page map failed\n");

	r = sys_page_unmap(envid, PFTEMP);
	if(r < 0)
		panic("pgfault: Page unmap failed\n");
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

	// LAB 4: Your code here.
	//panic("duppage not implemented");
	envid_t eid = sys_getenvid();
	void* va = (void*)(pn*PGSIZE);
	int perm = uvpt[pn]&0xfff;
	if((perm&PTE_W) || (perm&PTE_COW))
	{
		perm |= PTE_COW;
		perm &= ~PTE_W;
	}

	perm &= PTE_SYSCALL;

	r = sys_page_map(eid, va, envid, va, perm);
	if(r < 0)
		panic("duppage: Page map failed\n");

	r = sys_page_map(eid, va, eid, va, perm);
	if(r < 0)
		panic("duppage: Page map failed\n");

	return 0;
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
	//panic("fork not implemented");
	
	set_pgfault_handler(pgfault);
	envid_t envid = sys_exofork();

	if (envid < 0)
		panic("fork: Fork child env %e failed\n", envid);
	if (envid == 0) 
	{
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	uintptr_t addr;
	for (addr = UTEXT; addr < UXSTACKTOP-PGSIZE; addr += PGSIZE)
		if((uvpd[PDX(addr)]&PTE_P) && (uvpt[PGNUM(addr)]&(PTE_P|PTE_U)))
			duppage(envid, PGNUM(addr));

	int ret = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U|PTE_P|PTE_W);
	if(ret < 0)
		panic("fork: Page alloc failed\n");

	ret = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if(ret < 0)
		panic("fork: Set pgfault upcall failed\n");

	ret = sys_env_set_status(envid, ENV_RUNNABLE);
	if(ret < 0)
		panic("fork: Set status failed\n");

	return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
