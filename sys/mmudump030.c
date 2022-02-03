#define phys_get_long(addr)  *((cpuaddr *)(addr))

static void init_mmu_info_030(struct mmuinfo *info, tc_reg tcr)
{
	/* Note: 0 = Table A, 1 = Table B, 2 = Table C, 3 = Table D */
	int i, j;
	unsigned char shift;

	info->tc = tcr;
	info->mmu_enabled = tc.enable;

	/* Extract page size values from TC register */
	info->page_size_shift = tc.ps;
	info->page_size = 1UL << info->page_size_shift;
	info->page_mask = info->page_size - 1;

	/* Reset variables before extracting new values from TC */
	for (i = 0; i < 4; i++)
	{
		info->translation.table[i].mask = 0;
		info->translation.table[i].unused_fields_mask = 0;
		info->translation.table[i].shift = 0;
		info->translation.table[i].bits = 0;
	}
	
	/* Extract initial shift value from TC register */
	info->translation.init_shift = tc.is;

	info->translation.page.mask = info->page_size - 1;

	/* Calculate masks and shifts for later extracting table indices
	 * from logical addresses using: index = (addr&mask)>>shift */

	/* Get number of bits for each table index */
	info->translation.table[0].bits = tc.tia;
	info->translation.table[1].bits = tc.tib;
	info->translation.table[2].bits = tc.tic;
	info->translation.table[3].bits = tc.tid;

	/* Calculate masks and shifts for each table */
	info->translation.last_table = 0;
	shift = 32 - info->translation.init_shift;
	for (i = 0; i < 4 && info->translation.table[i].bits != 0; i++)
	{
		/* Get the shift */
		shift -= info->translation.table[i].bits;
		info->translation.table[i].shift = shift;
		/* Build the mask */
		for (j = 0; j < info->translation.table[i].bits; j++)
		{
			info->translation.table[i].mask |= (1UL << (shift + j));
		}
		/* Update until reaching the last table */
		info->translation.last_table = i;
	}
	for (j = 0; j < info->translation.init_shift; j++)
	{
		info->translation.table[0].unused_fields_mask |= (1UL << (32 - info->translation.init_shift + j));
	}
	for (i = 0; i < 4 && i <= info->translation.last_table; i++)
	{
		for (j = i + 1; j < 4 && j <= info->translation.last_table; j++)
		{
			info->translation.table[i].unused_fields_mask |= info->translation.table[j].mask;
		}
	}
	
	info->mmu_valid = TRUE;
	if (info->page_size_shift < 8)
		info->mmu_valid = FALSE;
	if ((shift - info->page_size_shift) != 0)
		info->mmu_valid = FALSE;
}


static void print_tc_info_030(struct mmuinfo *info)
{
	int i;
	unsigned char shift;
	unsigned long *p = (unsigned long *)&info->tc;
	
	FORCENONL("TCR    -> " PREG "\r\n", *p);
#define enabled(flag) info->tc.flag ? "enabled" : "disabled"
	FORCENONL("		  Paged address translation is %s\r\n", enabled(enable));
	FORCENONL("		  Page size is %lu 0x%lx shift = %u\r\n", info->page_size, info->page_size, info->page_size_shift);
	FORCENONL("		  Supervisor root pointer is %s\r\n", enabled(sre));
	FORCENONL("		  Function code lookup is %s\r\n", enabled(fcl));
	FORCENONL("		  Initial shift %u\r\n", info->translation.init_shift);
	for (i = 0; i < 4; i++)
	{
		FORCENONL("		  Table %c: mask " PREG ", unused " PREG ", bits %u, shift %u\r\n",
			'A' + i,
			info->translation.table[i].mask,
			info->translation.table[i].unused_fields_mask,
			info->translation.table[i].bits,
			info->translation.table[i].shift);
	}
	FORCENONL("		  Page mask 	" PREG "\r\n", info->page_mask);
	FORCENONL("		  Last Table: %c\r\n", 'A' + info->translation.last_table);
#undef enabled

	if (info->page_size_shift < 8)
	{
		FORCE("MMU Configuration Exception: Bad value in TC register! (bad page size: %u)",
			info->page_size_shift);
	}
	/* At least one table has to be defined using at least
	 * 1 bit for the index. At least 2 bits are necessary 
	 * if there is no second table. If these conditions are
	 * not met, it will automatically lead to a sum <32
	 * and cause an exception.
	 */
	if (info->translation.table[0].bits == 0)
	{
		FORCE("MMU Configuration Exception: Bad value in TC register! (no first table index defined)");
	} else if (info->translation.table[0].bits < 2 && info->translation.table[1].bits == 0)
	{
		FORCE("MMU Configuration Exception: Bad value in TC register! (no second table index defined and)");
		FORCE("MMU Configuration Exception: Bad value in TC register! (only 1 bit for first table index");
	}
	
	/* TI fields are summed up until a zero field is reached (see above
	 * loop). The sum of all TI field values plus page size and initial
	 * shift has to be 32: IS + PS + TIA + TIB + TIC + TID = 32
	 */
	shift = info->translation.init_shift + info->page_size_shift;
	for (i = 0; i < 4 && info->translation.table[i].bits != 0; i++)
		shift += info->translation.table[i].bits;
	if (shift != 32)
	{
		FORCE("MMU Configuration Exception: Bad value in TC register! (bad sum)");
	}
}


static void print_rp_info_030(const char *label, const cpuaddr *rp)
{
	unsigned char descriptor_type = (rp[0] & MMU030_DESCR_MASK) >> MMU030_DESCR_SHIFT;
	unsigned short table_limit = (rp[0] & MMU030_DESCR_LIMIT_MASK) >> MMU030_DESCR_LIMIT_SHIFT;
	cpureg first_addr = (rp[1] & MMU030_RP1_ADDR_MASK);

	FORCENONL("%s -> " PREG " " PREG "\r\n", label, rp[0], rp[1]);
	FORCENONL("		  descriptor type = %d (%s)\r\n",
		descriptor_type,
		descriptor_type == MMU030_DESCR_TYPE_INVALID ? "invalid descriptor" :
		descriptor_type == MMU030_DESCR_TYPE_PAGE	 ? "early termination page descriptor" :
		descriptor_type == MMU030_DESCR_TYPE_VALID4  ? "valid 4 byte descriptor" :
					/*	== MMU030_DESCR_TYPE_VALID8 */ "valid 8 byte descriptor");
	FORCENONL("		  %s limit = 0x%04x%s\r\n",
		(rp[0] & MMU030_DESCR_LOWER) ? "lower" : "upper",
		table_limit,
		(rp[0] & (MMU030_DESCR_LOWER|MMU030_DESCR_LIMIT_MASK)) == MMU030_DESCR_LIMIT_MASK ||
		(rp[0] & (MMU030_DESCR_LOWER|MMU030_DESCR_LIMIT_MASK)) == MMU030_DESCR_LOWER ?
		" (disabled)" : "");
	
	FORCENONL("		  first table address = " PREG "\r\n", first_addr);
}


#if 0 /* unused */
static void print_ttr_info_030(const char *label, cpureg tt)
{
	cpureg fc_mask, fc_base, addr_base, addr_mask;
	
	tt &= MMU030_TT_VALID_MASK;
	fc_mask = ~(((tt & MMU030_TT_FC_MASK_MASK) | ~MMU030_TT_FC_MASK_MASK) >> MMU030_TT_FC_MASK_SHIFT);
	fc_base = (tt & MMU030_TT_FC_BASE_MASK) >> MMU030_TT_FC_BASE_SHIFT;
	addr_base = tt & MMU030_TT_ADDR_BASE_MASK;
	addr_mask = ~(((tt & MMU030_TT_ADDR_MASK_MASK) << (MMU030_TT_ADDR_BASE_SHIFT - MMU030_TT_ADDR_MASK_SHIFT)) | ~MMU030_TT_ADDR_BASE_MASK);
	
	FORCENONL("%s -> " PREG "\r\n", label, tt);
#define enabled(flag) tt & flag ? "enabled" : "disabled"
	FORCENONL("		  transparent translation is %s\r\n", enabled(MMU030_TT_ENABLED));
	if (tt & MMU030_TT_ENABLED)
	{
		FORCENONL("		  caching is %s\r\n", tt & MMU030_TT_CI ? "inhibited" : "enabled");
		FORCENONL("		  read-modify-write is %s\r\n",
			tt & MMU030_TT_RWM ? "enabled" :
			tt & MMU030_TT_RW ? "disabled (read only)" : "disabled (write only)");
		FORCENONL("		  function code base: 0x%1lx\r\n", fc_base);
		FORCENONL("		  function code mask: 0x%1lx -> 0x%1lx\r\n",
			(tt & MMU030_TT_FC_MASK_MASK) >> MMU030_TT_FC_MASK_SHIFT,
			fc_mask);
		FORCENONL("		  address base: " PREG "\r\n", addr_base);
		FORCENONL("		  address mask: " PREG " -> " PREG "\r\n",
			(tt & MMU030_TT_ADDR_MASK_MASK) << (MMU030_TT_ADDR_BASE_SHIFT - MMU030_TT_ADDR_MASK_SHIFT),
			addr_mask);
	}
#undef enabled
}
#endif


static void indent(int level)
{
	int i;
	
	for (i = 0; i < level; i++)
		FORCENONL("        ");
}


static void mmu030_print_page_descriptor(const cpuaddr *descr, int descr_size, cpuaddr log_addr, cpuaddr page_addr, bool last)
{
	unsigned short table_limit;
	
	FORCENONL(PREG " -> " PREG, log_addr, page_addr);
	if (descr_size == 8 && (descr[0] & MMU030_DESCR_S))
		FORCENONL(", Super");
	if (descr[0] & MMU030_DESCR_WP)
		FORCENONL(", WP");
	if (descr[0] & MMU030_DESCR_U)
		FORCENONL(", Updated");
	if (last && (descr[0] & MMU030_DESCR_M))
		FORCENONL(", Modified");
	if (last && (descr[0] & MMU030_DESCR_CI))
		FORCENONL(", Cache-Inhibit");
	if (descr_size == 8)
	{
		table_limit = (descr[0] & MMU030_DESCR_LIMIT_MASK) >> MMU030_DESCR_LIMIT_SHIFT;
	
		FORCENONL(",%s limit = 0x%04x%s\r\n",
			(descr[0] & MMU030_DESCR_LOWER) ? "lower" : "upper",
			table_limit,
			(descr[0] & (MMU030_DESCR_LOWER|MMU030_DESCR_LIMIT_MASK)) == MMU030_DESCR_LIMIT_MASK ||
			(descr[0] & (MMU030_DESCR_LOWER|MMU030_DESCR_LIMIT_MASK)) == MMU030_DESCR_LOWER ?
			" (disabled)" : "");
	}
	FORCENONL("\r\n");
}


static void mmu030_print_table(struct mmuinfo *info, cpuaddr log_addr, int level, int descr_size, cpuaddr table_addr, int *indices)
{
	cpuaddr log_addr_inc;
	unsigned short table_index, count;
	cpuaddr descr[2];
	cpuaddr descr_addr;
	cpuaddr page_addr;
	unsigned char descr_type;
	int l;
	
	log_addr_inc = 1UL << info->translation.table[level].shift;
	count = 1 << info->translation.table[level].bits;
	descr_addr = table_addr;
	for (table_index = 0; table_index < count; table_index++, log_addr += log_addr_inc, descr_addr += descr_size)
	{
		descr[0] = phys_get_long(descr_addr);
		if (descr_size == 8)
		{
			descr[1] = phys_get_long(descr_addr + 4);
			page_addr = descr[1];
		} else
		{
			page_addr = descr[0];
		}
			
		descr_type = (descr[0] & MMU030_DESCR_MASK) >> MMU030_DESCR_SHIFT;
		indent(level);
		for (l = 0; l < level; l++)
			FORCENONL("%04x:", indices[l]);
		FORCENONL("%04x: %08lx: ", table_index, page_addr);
		switch (descr_type)
		{
		case MMU030_DESCR_TYPE_INVALID:
			/* stop table walk */
			if (level < info->translation.last_table)
			{
				FORCENONL("INVALID (" PREG ")\r\n", descr[0]);
			} else
			{
				FORCENONL("INVALID\r\n");
				page_addr &= MMU030_DESCR_PD_ADDR_MASK;
				indent(level + 1);
				mmu030_print_page_descriptor(descr, descr_size, log_addr, page_addr, TRUE);
			}
			break;
		case MMU030_DESCR_TYPE_EARLY_TERM:
			FORCENONL("Page Descriptor%s\r\n", level < info->translation.last_table ? " (early term)" : "");
			page_addr &= MMU030_DESCR_PD_ADDR_MASK;
			indent(level + 1);
			mmu030_print_page_descriptor(descr, descr_size, log_addr, page_addr, TRUE);
			break;
		case MMU030_DESCR_TYPE_VALID4:
			FORCENONL("Short Descriptors\r\n");
			page_addr &= MMU030_DESCR_TD_ADDR_MASK;
			indent(level + 1);
			mmu030_print_page_descriptor(descr, descr_size, log_addr, page_addr, FALSE);
			indices[level] = table_index;
			mmu030_print_table(info, log_addr, level + 1, 4, page_addr, indices);
			break;
		case MMU030_DESCR_TYPE_VALID8:
			FORCENONL("Long Descriptors\r\n");
			page_addr &= MMU030_DESCR_TD_ADDR_MASK;
			indent(level + 1);
			mmu030_print_page_descriptor(descr, descr_size, log_addr, page_addr, FALSE);
			indices[level] = table_index;
			mmu030_print_table(info, log_addr, level + 1, 8, page_addr, indices);
			break;
		}
	}
}


static void mmu030_print_tree(struct mmuinfo *info, const cpuaddr *crp)
{
	unsigned char descr_type;
	cpuaddr descr[2];
	/* During table walk up to 7 different descriptors are used:
	 * root pointer, descriptors fetched from function code lookup table,
	 * tables A, B, C and D and one indirect descriptor
	 */
	cpuaddr descr_addr;
	cpuaddr table_addr;
	cpuaddr page_addr;
	int next_size = 0;
	int descr_size = 8;
	unsigned short table_index;
	int indices[4];
	
	descr[0] = crp[0];
	descr[1] = crp[1];

	/* Check descriptor type of root pointer */
	descr_type = (descr[0] & MMU030_DESCR_MASK) >> MMU030_DESCR_SHIFT;

	page_addr = (descr[1] & MMU030_RP1_ADDR_MASK);
	FORCENONL("Root table --> " PREG " ", page_addr);
	switch (descr_type)
	{
	case MMU030_DESCR_TYPE_INVALID:
		FORCENONL("INVALID (" PREG ")\r\n", descr[0]);
		return;
	case MMU030_DESCR_TYPE_EARLY_TERM:
		/*
		 * A translation table for this root pointer does not exist. The MC68030
		 * internally calculates an ATC entry (page descriptor) for accesses using
		 * this root pointer within the current page by adding (unsigned) the value
		 * in the table address field to the incoming logical address. This results
		 * in direct mapping with a constant offset (the table address). For this
		 * case, the processor performs a limit check, regardless of the state of
		 * the FCL bit in the TC register.
		 */
		FORCENONL("Early termination\r\n");
		indent(1);
		mmu030_print_page_descriptor(descr, 8, 0, page_addr, FALSE);
		return;
	case MMU030_DESCR_TYPE_VALID4:
		FORCENONL("Short Descriptors\r\n");
		next_size = 4;
		break;
	case MMU030_DESCR_TYPE_VALID8:
		FORCENONL("Long Descriptors\r\n");
		next_size = 8;
		break;
	}

	/*
	 * When function code lookup is enabled, the first table of
	 * the translation table structure is indexed by function code. In this case, the
	 * limit field of CRP or SRP is ignored.
	 */
	if (info->tc.fcl)
	{
		table_addr = descr[1] & MMU030_RP1_ADDR_MASK;
		
		/* table index is function code */
		for (table_index = 0; table_index < 8; table_index++)
		{
			descr_addr = table_addr + (table_index << (next_size == 8 ? 3 : 2));
	
			descr[0] = phys_get_long(descr_addr);
			if (next_size == 8)
				descr[1] = phys_get_long(descr_addr + 4);
			
			descr_size = next_size;
	
			page_addr = descr[descr_size == 4 ? 0 : 1];
			/* Check descriptor type */
			descr_type = (descr[0] & MMU030_DESCR_MASK) >> MMU030_DESCR_SHIFT;
			switch (descr_type)
			{
			case MMU030_DESCR_TYPE_INVALID:
				/* stop table walk */
				FORCENONL("INVALID (" PREG ")\r\n", descr[0]);
				break;
			case MMU030_DESCR_TYPE_EARLY_TERM:
				FORCENONL("Page Descriptor\r\n");
				page_addr &= MMU030_DESCR_PD_ADDR_MASK;
				indent(1);
				mmu030_print_page_descriptor(descr, descr_size, 0, page_addr, TRUE);
				break;
			case MMU030_DESCR_TYPE_VALID4:
				FORCENONL("Short Descriptors\r\n");
				page_addr &= MMU030_DESCR_TD_ADDR_MASK;
				indent(1);
				mmu030_print_page_descriptor(descr, descr_size, 0, page_addr, FALSE);
				mmu030_print_table(info, 0, 0, 4, page_addr, indices);
				break;
			case MMU030_DESCR_TYPE_VALID8:
				FORCENONL("Long Descriptors\r\n");
				page_addr &= MMU030_DESCR_TD_ADDR_MASK;
				indent(1);
				mmu030_print_page_descriptor(descr, descr_size, 0, page_addr, FALSE);
				mmu030_print_table(info, 0, 0, 8, page_addr, indices);
				break;
			}
		}
		return;
	}
	mmu030_print_table(info, 0, 0, next_size, page_addr, indices);
}
