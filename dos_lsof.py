#
# Attempt to write custom gdb addon to debug DOS programs
#
# v0.000000001
#
# (c) 2019 martin


# XXX: this can't be static ( AH=52H, int 0x21 - maybe inject call, get vals ?)
DOS_INVARS_SEGM = 0xc9
DOS_INVARS_OFST = 0x26
DOS_INVARS_SFT = 4			# offset in in_vars table where SFT is

DOS_SFT_SIZE	= 59			# XXX: not in DOS v2
DOS_SFT_ENTRIES	= 4
DOS_SFT_REFCOUNT = 6
DOS_SFT_FNAME = 0x26

is_verbose = 1				# XXX: maybe this should be a member of the class only ?

DEBUG = 0

def dword_to_phys( dword ):
	"""Convert dword into the actual physical address.i Return tripplet: segment, offset, physical"""
	if DEBUG: print ("dword_to_phys: converting %x"%dword)	

	segm = (dword & 0xffff0000) >> 16
	ofst = dword & 0xffff
	phys = seg2phys(segm,ofst)

	return [segm,ofst,phys]


def b2int( raw_bytes ):
	""" Convert raw bytes into number."""
	return int.from_bytes(raw_bytes, byteorder='little')


def seg2phys( segm,ofst ):
	""" Create physical address from segment, offset arguments."""
	return segm*0x10+ofst


def parse_segm_addr( saddr ):
	"""Parse address from argument in segment:offset form. Return segment,offset and the actual physical address."""
	[ raw_segm, raw_ofst ] = saddr.split(":")

	segm = int(raw_segm, 16)
	ofst = int(raw_ofst, 16)
	phys = seg2phys(segm,ofst)

	if DEBUG: print ("parse_segm_addr: %x:%x = %x"%(segm,ofst,phys))
	return [segm,ofst,phys]


def peek( addr_start, size ):
	""" Peek into the current process memory. Returns the memoryview of given size"""
	return gdb.selected_inferior().read_memory(addr_start, size)

def peek_dword( addr_start ):
	return b2int(gdb.selected_inferior().read_memory(addr_start, 4))

def peek_word( addr_start ):
	return b2int(gdb.selected_inferior().read_memory(addr_start, 2))

def peek_str( addr_start, size ):
	return gdb.selected_inferior().read_memory(addr_start, size).tobytes().decode('ascii')


class dos_lsof(gdb.Command):
	"""lsof like behavior in DOS"""

	def __init__ (self):
		super (dos_lsof, self).__init__ ("dos-lsof", gdb.COMMAND_USER)

	def invoke (self, arg, from_tty):
		# DOS invars address can be specified from arg ; default is to used saved vals
		if len(arg) != 0:
			# get segment, offset from there
			[invars_segm,invars_ofst,addr_phys_invars] = parse_segm_addr(arg.split(" ")[0])
		else:
			invars_segm = DOS_INVARS_SEGM
			invars_ofst = DOS_INVARS_OFST
			addr_phys_invars = seg2phys(invars_segm, invars_ofst)

		# print header
		print ("\n\nin_vars:\t%.04x:%.04x\nSFT entry size:\t%d\nfname offset:\t%d\n"%(invars_segm,invars_ofst,DOS_SFT_SIZE,DOS_SFT_FNAME))

		# first SFT table location 
		sftloc = peek_dword(addr_phys_invars + DOS_INVARS_SFT)
		[ sft_segm,sft_ofst,sft_phys ] = dword_to_phys(sftloc)

		cur_fd = 0
		# walk SFT tables
		while (True):
			sft_entries = peek_word ( sft_phys + DOS_SFT_ENTRIES )

			print ("SFT @ %.04x:%.04x, entries: %d"%(sft_segm,sft_ofst,sft_entries))
			for i in range(sft_entries):
				sft_fname = peek_str ( sft_phys + i*DOS_SFT_SIZE + DOS_SFT_FNAME, 11 )
				sft_refc = peek_word ( sft_phys + i*DOS_SFT_SIZE + DOS_SFT_REFCOUNT )

				if (sft_refc == 0):
					if is_verbose: print(" %d : free"%(cur_fd + i))
				else:
					print (" %d : %11s %d"%(cur_fd + i, sft_fname, sft_refc))

			cur_fd += sft_entries
			# entries we are interested in

			sftloc = peek_dword(sft_phys)
			[ sft_segm,sft_ofst,sft_phys ] = dword_to_phys(sftloc)
			if sft_ofst == 0xffff: break

			print ("\n")
dos_lsof()

