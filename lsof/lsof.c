/*
 * Check what files are opened by current process.
 * There's not much use of this in DOS itself as there's
 * usually only one process running.
 * This is just a POC for writting python wrapper for gdb to
 * read these structures during remote debugging.
 *
 * (c) 2019 martin
 *
 */

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>

/*
 * SFT size is not part of the header, it's internal to particular DOS version
 *
 *	DOS 2-		XXX: unknown ?
 *	DOS 3.x 	0x35
 *	DOS 4.x+	0x3B
 */

struct sft_header {
	unsigned int ofst;
	unsigned int segm;
	unsigned int entries;
} PACKED;

char far* get_sft(void);
char get_sftsize(void);

int main(void) {
	struct sft_header sfth;
	char far *SFT;
	char sftsz;

	int i,j,sum,hnd;
	char far *fname;
	char buf[16];

	clrscr();

	// SFT entry size depends on DOS version
	if ((sftsz = get_sftsize()) == -1) {
		printf ("oops: unsupported DOS version.\n");
		return 1;
	}

	printf ("SFT entry size: %d\n", sftsz);

	// get first SFT
	SFT = get_sft();
	sum = 0;
	for (;;) {
		sfth.ofst = *(int far*)SFT;
		sfth.segm = *(int far*)(SFT+2);
		sfth.entries = *(int far*)(SFT+4);
		sum += sfth.entries;

		printf ("\nSFT @ %Fp, entries: %d\n", SFT,sfth.entries);

		for (i= 0; i < sfth.entries; i++) {
			hnd = *(int far*)(SFT+i*sftsz+0x6);

			if (hnd == 0) {
				printf ("%d : free\n",sum+i);
			}
			else {
				fname = (char far*)(SFT+i*sftsz+0x26);

				// copy to local buf, fname is 8.3 format
				memset(buf, 0, sizeof buf);
				for (j = 0; j < 11; j++) {
					buf[j] = fname[j];
				}

				printf("%d : %s , ref: %d\n", sum+i,buf,hnd);
			}
		}
		// last SFT
		if (sfth.ofst == 0xffff ) break;

		SFT = (char far*)MK_FP(sfth.segm, sfth.ofst);
	}

	printf ("FILES=%d\n", sum);
	return 0;
}

char get_sftsize() {
	char size;
	asm {
		mov ah, 0x30
		int 0x21

		cmp al, 4
		jge dos4
		cmp al, 3
		je dos3

		mov [size], -1
		jmp end
	}

dos3:
	asm {
		mov [size], 0x35
		jmp end
	}

dos4:
	asm {
		mov [size], 0x3b
	}

end:
	return size;
}

char far* get_sft() {
	char far *sft;
	asm {
		push es
		push di
		push bx

		mov ah, 0x52			// AH=52h get list of lists
		int 0x21

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