# 代码
```
diff --git a/Lab2/lab/kern/pmap.c b/Lab2/lab/kern/pmap.c
index 4499a32..814ae63 100644
--- a/Lab2/lab/kern/pmap.c
+++ b/Lab2/lab/kern/pmap.c
@@ -265,15 +265,15 @@ page_init(void)
 		pp = &pages[i];
 
 		//[0, PGSIZE)
-		if(page2pa(pp) < PGSIZE)
+		if (page2pa(pp) < PGSIZE)
 			continue;
 		
 		//[IOPHYSMEM, EXTPHYSMEM)
-		if(page2pa(pp) >= IOPHYSMEM && page2pa(pp) < EXTPHYSMEM)
+		if (page2pa(pp) >= IOPHYSMEM && page2pa(pp) < EXTPHYSMEM)
 			continue;
 
 		//[EXTPHYSMEM, ...)
-		if(page2pa(pp) >= EXTPHYSMEM && page2pa(pp) < PADDR(va))
+		if (page2pa(pp) >= EXTPHYSMEM && page2pa(pp) < PADDR(va))
 			continue;
 
 		pp->pp_ref = 0;
@@ -300,7 +300,7 @@ page_alloc(int alloc_flags)
 	// Fill this function in
 	struct PageInfo *page_alloc = page_free_list;
 
-	if(page_alloc == NULL)
+	if (page_alloc == NULL)
 		return NULL;
 
 	page_free_list = page_alloc->pp_link;
@@ -324,7 +324,7 @@ page_free(struct PageInfo *pp)
 	// Fill this function in
 	// Hint: You may want to panic if pp->pp_ref is nonzero or
 	// pp->pp_link is not NULL.
-	if(pp->pp_ref || pp->pp_link)
+	if (pp->pp_ref || pp->pp_link)
 		return;
 
 	pp->pp_link = page_free_list;
@@ -368,7 +368,33 @@ pte_t *
 pgdir_walk(pde_t *pgdir, const void *va, int create)
 {
 	// Fill this function in
-	return NULL;
+	pte_t *pgtable = NULL;
+	struct PageInfo *pp = NULL;
+
+	if (pgdir == NULL)
+		return NULL;
+	
+	//PAGE DIR ENTRY
+	if ((pgdir[PDX(va)] & PTE_P) != PTE_P) //not exist
+	{
+		if (create == false)
+			return NULL;
+
+		//create the page table
+		pp = page_alloc(ALLOC_ZERO);
+		if (pp == NULL)
+			return NULL;
+
+		//reload to the page dir entry
+		pgdir[PDX(va)] = PTE_ADDR(page2pa(pp)) | PTE_U | PTE_W | PTE_P;
+		pp->pp_ref++;
+	}
+
+	//PAGE TABLE
+	pgtable = (pte_t *)KADDR(PTE_ADDR(pgdir[PDX(va)]));
+	
+	//PAGE TABLE ENTRY
+	return &pgtable[PTX(va)];
 }
 
 //
@@ -386,6 +412,25 @@ static void
 boot_map_region(pde_t *pgdir, uintptr_t va, size_t size, physaddr_t pa, int perm)
 {
 	// Fill this function in
+	uint32_t i = 0;
+	pte_t *pte = NULL;
+	uintptr_t real_va = 0;
+	size_t real_size = 0;
+	physaddr_t real_pa = 0;
+
+	assert(pgdir != NULL);
+
+	real_pa = ROUNDDOWN(pa, PGSIZE);
+	real_va = ROUNDDOWN(va, PGSIZE);
+	real_size = ROUNDUP(va+size, PGSIZE) - real_va;
+
+	for (i = 0; i < real_size; i+=PGSIZE)
+	{
+		pte = pgdir_walk(pgdir, (void *)(real_va + i), true);
+		assert(pte != NULL);
+		
+		*pte = PTE_ADDR(real_pa + i) | perm | PTE_P;
+	}
 }
 
 //
@@ -417,6 +462,31 @@ int
 page_insert(pde_t *pgdir, struct PageInfo *pp, void *va, int perm)
 {
 	// Fill this function in
+	pte_t *pte = NULL; 
+
+	if (pgdir == NULL || pp == NULL)
+		return -E_NO_MEM;
+
+	//try to create page table entry
+	pte = pgdir_walk(pgdir, va, true);
+	if (pte == NULL)
+	{
+		return -E_NO_MEM;
+	}
+
+	//consider the same page
+	//it should be incremented here 
+	//avoid to delete the same page
+	pp->pp_ref++;
+
+	if ((*pte & PTE_P) == PTE_P)
+	{
+		page_remove(pgdir, va);
+	}
+	
+	//reload the page frame to the page table entry
+	*pte = page2pa(pp) | perm | PTE_P;
+	
 	return 0;
 }
 
@@ -435,7 +505,25 @@ struct PageInfo *
 page_lookup(pde_t *pgdir, void *va, pte_t **pte_store)
 {
 	// Fill this function in
-	return NULL;
+	struct PageInfo *pp = NULL;
+
+	if (pgdir == NULL || pte_store == NULL)
+		return NULL;
+
+	//find the page table entry
+	*pte_store = pgdir_walk(pgdir, va, false);
+	if (*pte_store == NULL)
+	{
+		return NULL;
+	}
+
+	if ((**pte_store & PTE_P) != PTE_P)
+	{
+		return NULL;
+	}
+
+	pp = pa2page(PTE_ADDR(**pte_store));
+	return pp;
 }
 
 //
@@ -457,6 +545,23 @@ void
 page_remove(pde_t *pgdir, void *va)
 {
 	// Fill this function in
+	struct PageInfo *pp = NULL;
+	pte_t *pte = NULL; 
+	
+	pp = page_lookup(pgdir, va, &pte);
+	if (pp == NULL) //page frame not exist
+	{
+		return;
+	}
+
+	page_decref(pp);
+	
+	if (pte)
+	{
+		*pte = 0;
+	}
+
+	tlb_invalidate(pgdir, va);
 }
 
 //
```