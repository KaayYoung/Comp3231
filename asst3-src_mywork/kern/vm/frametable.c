#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <addrspace.h>
#include <vm.h>

/* Place your frametable data-structures here 
 * You probably also want to write a frametable initialisation
 * function and call it from vm_bootstrap
 */

struct frametable_entry *frametable;

static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

/* Note that this function returns a VIRTUAL address, not a physical 
 * address
 * WARNING: this function gets called very early, before
 * vm_bootstrap().  You may wish to modify main.c to call your
 * frame table initialisation function, or check to see if the
 * frame table has been initialised and call ram_stealmem() otherwise.
 */

void frametable_init(void) {
    
    paddr_t top_of_ram;
    paddr_t bot_size;
    
    size_t frametable_size;

    top_of_ram = ram_getsize();
    bot_size = ram_getfirstfree();
    
    unsigned int npages, usedpages, i;
    npages = top_of_ram / PAGE_SIZE;
    hpt_size = 2 * npages;

	paddr_t location = top_of_ram - (nframes * sizeof(struct frametable_entry)); 

	frametable = (struct frametable_entry *) PADDR_TO_KVADDR(location);

    usedpages = bot_size / PAGE_SIZE;
    
    for (i = 0; i < usedpages; i++) {
    	frametable_entry[i].used = 1;
        frametable_entry[i].next = VM_INVALID_INDEX;
    }
    for (i = usedpages; i < npages; i++) {
    	frametable_entry[i].used = 0;
        frametable_entry[i].next = i + 1;
    }


    frametable[npages - 1].next = VM_INVALID_INDEX;

    /* init the page table */
    for(i = 0; i < hpt_size; i++) {
        hpt[i] = NULL;
    }
}

vaddr_t alloc_kpages(unsigned int npages)
{
        /*
         * IMPLEMENT ME.  You should replace this code with a proper
         *                implementation.
         */

        paddr_t addr;

        if (frametable == NULL) {
        	spinlock_acquire(&stealmem_lock);
        	addr = ram_stealmem(npages);
        	spinlock_release(&stealmem_lock);

        	if(addr == 0)
                return 0;
        	return PADDR_TO_KVADDR(addr);

        } else {
        	if (npages > 1){
                panic("Can not deal with more than one page");
                return 0;
            }
        	spinlock_acquire(&stealmem_lock);
        	entry = PADDR_TO_KVADDR(addr)
        	int next_free_page;
            if (frametable[entry].next_free ==  VM_INVALID_INDEX) {
            	next_free_page = VM_INVALID_INDEX;
                addr = 0;
            } else {
            	next_free_page = frametable[next_free].next_free
            }
            ft[c_index].fe_next = VM_INVALID_INDEX;
            spinlock_release(&stealmem_lock);
            return addr;
        }

}

void free_kpages(vaddr_t addr)
{		
		if (frametable == NULL) {
                return NULL;
        }

        paddr_t paddr = KVADDR_TO_PADDR(addr);
        unsigned entry = paddr / PAGE_SIZE;

        spinlock_acquire(&stealmem_lock);

        frametable[entry].used = 0;
        frametable[entry].next_free = entry;
        spinlock_release(&stealmem_lock);
}

