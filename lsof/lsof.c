/*
 * Print currently used file descriptors and their names.
 * Walk the System File Tables to extract the information.
 *
 * This is just a POC for writting python wrapper for gdb to
 * read these structures during remote debugging.
 *
 * (c) 2019 martin
 *
 */

#include <stdio.h>
#include <conio.h>
#include <string.h>
#include <dos.h>

/*
 * sft_size is not part of the SFT header; varies between DOS versions
 *
 *	DOS 2.x		src: https://github.com/Microsoft/MS-DOS/blob/master/v2.0/source/DOSSYM.ASM:606
 *	DOS 3.x 	0x35
 *	DOS 4.x+	0x3B
 */

struct sft_header {
	unsigned int ofst;
	unsigned int segm;
	unsigned int entries;
};

struct fcb_idx {
	char dosver;
	unsigned int entrysz;
	char fname_offset;
};

char far* get_sft(void);
char far* get_invars(void);

void get_sftvars(struct fcb_idx* fdx);
void print_header(struct fcb_idx* fdx);

int main(int argc, char** argv) {
	struct sft_header sfth;
	struct fcb_idx fdx;
	char far *SFT;
	char far *invars;

	unsigned int ref_count;
	int i,sum,verbose;
	char far *fname;

	get_sftvars(&fdx);
	if (fdx.dosver < 2) {
		printf ("DOS version %d not supported\n", fdx.dosver);
		return 1;
	}

	verbose = 0;
	if (argc == 2) {
		if (!strcmp(argv[1], "-v")) verbose = 1;
	}

	print_header(&fdx);

	// get first SFT
	SFT = get_sft();
	sum = 0;
	for (;;) {
		sfth.ofst = *(int far*)SFT;
		sfth.segm = *(int far*)(SFT+2);
		sfth.entries = *(int far*)(SFT+4);

		printf ("\n SFT @ %Fp, entries: %d\n --------------------\n", SFT,sfth.entries);

		for (i= 0; i < sfth.entries; i++) {
			ref_count = *(int far*)(SFT+i*fdx.entrysz+0x6);

			// in DOS 2.x ref_count is byte
			if (fdx.dosver == 2) ref_count &= 0xff;

			if (ref_count == 0) {
				if (verbose) printf ("  %d : free\n",sum+i);
			}
			else {
				fname = (char far*)(SFT+i*fdx.entrysz+fdx.fname_offset);
				printf("  %d : %.11Fs  %d\n", sum+i,fname,ref_count);
			}
		}
		sum += sfth.entries;

		// last SFT
		if (sfth.ofst == 0xffff ) break;

		SFT = (char far*)MK_FP(sfth.segm, sfth.ofst);
	}
	printf ("\nFILES=%d\n", sum);
	return 0;
}

void print_header(struct fcb_idx* fdx) {
	printf ("\nDOS version:\t%d\nin_vars:\t%Fp\nSFT entry size:\t%d\nfname offset:\t%d\n",
		fdx->dosver,get_invars(),fdx->entrysz,fdx->fname_offset);
}

void get_sftvars(struct fcb_idx* fdx) {
	asm {
		push bx
		mov ah, 0x30
		int 0x21

		mov bx, [fdx]
		mov byte [bx], al
		mov byte [bx+3], 0x26		// only DOS v2 has offset 0x0a

		cmp al, 4
		jge dos4
		cmp al, 3
		je dos3
		cmp al, 2
		je dos2

		mov [fdx], -1
		jmp end
	}

dos2:
	asm {
		mov word [bx+1], 0x28
		mov byte [bx+3], 0x0a
		jmp end
	}
dos3:
	asm {
		mov word [bx+1], 0x35
		jmp end
	}
dos4:
	asm {
		mov word [bx+1], 0x3b
	}
end:
	asm {
		pop bx
	}
}

char far* get_sft() {
	char far *sft;
	asm {
		push es
		push di
		push bx

		mov ah, 0x52			// AH=52h get list of lists
		int 0x21				// ES:BX invars

		mov di, [es:bx+4]		// SFT table start dword
		mov es, [es:bx+6]

		mov word [sft+2],es
		mov word [sft],di

		pop bx
		pop di
		pop es
		xor ax,ax
	}
	return sft;
}

char far* get_invars(void) {
	char far* invars;
	asm {
		mov ah, 0x52
		int 0x21
		mov word [invars+2], es
		mov word [invars], bx
		xor ax,ax
	}
	return invars;
}
