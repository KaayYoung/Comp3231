#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/tlb.h>

/* Place your page table functions here */


static struct lock *pagetable_lock;
struct pagetable_entry *pagetable = 0;

static uint32_t hpt_hash(struct addrspace *as, vaddr_t faultaddr)
{
    uint32_t index;
    index = (((uint32_t )as) ^ (faultaddr >> PAGE_BITS)) % hpt_size;
    return index;
}

static region find_region(struct addrspace *as, vaddr_t faultaddress)
{
        for (as_region region = as->first_region; region; region = region->next) {
                if (region->vbase <= faultaddress && (region->vbase + region->size) > faultaddress) {
                        return region;
                }
        }
        return NULL;
}

void vm_bootstrap(void)
{
        /* Initialise VM sub-system.  You probably want to initialise your 
           frame table here as well.
        */
	frametable_init();
	
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{

	// Three different types of faulttype
    switch (faulttype) {
        case VM_FAULT_READONLY:
            return EFAULT;
        case VM_FAULT_READ:
        case VM_FAULT_WRITE:
            break;
        default:
            return EINVAL;
    } 

    if (curproc == NULL) {
        return EFAULT;
    }

    struct addrspace *cur_addr = proc_getas();
    if (cur_addr == NULL) {
        return EFAULT;
    }
    uint32_t hash = hpt_hash(cur_addr, faultaddress);

    lock_acquire(pagetable_lock);
    // if tlb miss, look up page table
    int entry = faultaddress / hpt_size;
    bool found = false;

    while (pagetable[entry].pid != pid || pagetable[entry].virtual_addr != faultaddress - faultaddress % PAGE_SIZE) {
    	entry = pagetable[entry].next_entry;
    	if (entry == 0) return EFAULT;
    	found = true;
    }

    paddr_t paddr = pagetable[index].frame_addr;
    if(paddr == 0 || pagetable[index].pid != pid) return EFAULT;
    lock_release(pagetable_lock);
    
    // Because not in the page table, we need to look up in the region
    if (found == false) {
    	region cur_region = find_region(cur_addr, faultaddress);
    	if (cur_region == NULL) {
    		return EFAULT;
    	}

    	vaddr_t vaddr = alloc_kpages(1);
        if (vaddr == 0) {
            return ENOMEM;
        }
        bzero((void*) vaddr, PAGE_SIZE);

        struct pagetable_entry *new = kmalloc(sizeof(struct pagetable_entry));
        if (new == NULL) {
        	free_kpages(vaddr);
        	return ENOME;
        }
        new->pid = (uint32_t) cur_addr;
        new->virtual_addr = faultaddress;
        lock_acquire(page_table_lock);
        new->next = hash;
        pagetable[hash] = new;
        lock_release(pagetable_lock);
    }

    // Update tlb
    int spl = splhigh();
    uint32_t ehi = a & TLBHI_VPAGE;
    uint32_t elo = KVADDR_TO_PADDR(paddr)| TLBLO_DIRTY | TLBLO_VALID;

    tlb_random(ehi, elo | TLBLO_VALID | TLBLO_DIRTY);
    splx(spl);

    return 0;
}

/*
 *
 * SMP-specific functions.  Unused in our configuration.
 */

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
        (void)ts;
        panic("vm tried to do tlb shootdown?!\n");
}

