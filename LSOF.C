/*
 * Check what files are opened by current process.
 * There's not much use of this in DOS itself as there's
 * usually only one process running (TSR excluded). This
 * is just a POC for writting python wrapper in gdb to
 * read these structures there.
 *
 * (c) 2019 Martin
 *
 */

#include <stdio.h>
#include <conio.h>
#include <dos.h>
#include <mem.h>

#define  SFTSIZE	59		// XXX: not in all DOS versions though

char far* get_sft();

int main() {
	char far *sft;
	int i,j,size,sum,seg,ofst,hnd;
	char far *fname;
	char buf[16];

	clrscr();

	sft = get_sft();
	sum = 0;

	for (;;) {
		seg = *(int far*)(sft+2);
		ofst = *(int far*)sft;

		size = *(int far*)(sft+4);
		hnd = *(int far*)(sft+6);

		printf ("SFT @ %Fp, entries: %d\n", sft, size);

		for (i= 0; i < size; i++) {
			if (hnd == 0) {
				//printf ("%d : free\n",sum+i);
			}
			else {
				fname = (char far*)(sft+i*SFTSIZE+0x26);

				// copy to local buf, fname is 8.3 format
				memset(buf, 0, sizeof buf);
				for (j = 0; j < 11; j++) {
					buf[j] = fname[j];
				}
				printf ("%d : %s , ref: %d\n", sum+i,buf,hnd);
			}
		}

		if (ofst == -1 ) break;

		sum += size;
		sft = (char far*)MK_FP(seg, ofst);
		//getch();
	}
	return 0;
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
