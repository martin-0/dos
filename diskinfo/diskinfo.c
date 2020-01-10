/* martin (c) 2020 */

#include <stdio.h>
#include <conio.h>
#include <mem.h>
#include <dos.h>

#define	VERSION	0.11

#define OFFSETOF(type,field)	((unsigned long) &(((type*)0)->field))

// 0x42 is the max size of the structure
struct drive_ext_t {
	int struct_size;				// word
	int flags;                 		// word
	unsigned long int c;			// dword
	unsigned long int h;   	        // dword
	unsigned long int s;  	        // dword
	unsigned long int lo_total_s;	// qword lo+hi
	unsigned long int hi_total_s;
	unsigned int bps;				// word

	char pad[40];
};

int bios_inst_check(unsigned char drive);
int bios_drive_param(unsigned char drive, struct drive_ext_t* d);
void dump_buffer(struct drive_ext_t* d);

int main() {
	struct drive_ext_t d;
	unsigned long int total_from_chs;
	int i;

	clrscr();

	// let's try for 10 disks
	for (i=0; i < 10; i++) {
		if (! bios_inst_check(0x80+i)) {
			memset(&d, 0, sizeof(d));
			d.struct_size = 0x42;

			if (!bios_drive_param(0x80+i, &d)) {

				total_from_chs = d.c*d.h*d.s*d.bps;
				printf ("\nCHS: %ld/%ld/%ld, sector size: %d\n",
					 d.c, d.h, d.s, d.bps);

				dump_buffer(&d);

			}
			printf ("\n");
		}
		else {
			break;
		}
	}
	return 0;
}

int bios_drive_param(unsigned char drive, struct drive_ext_t* d) {
	int i;

	union REGS regs;
	struct SREGS sregs;

	regs.h.ah = 0x48;
	regs.h.dl = drive;
	regs.x.si = FP_OFF(d);
	sregs.ds =  FP_SEG(d);

	int86x(0x13, &regs, &regs, &sregs);

	if (regs.x.cflag) {
		// printf ("error occured\n");
		return 1;
	}
	return 0;
}

void dump_buffer(struct drive_ext_t* d) {
	int i;
	printf ("\n");
	for (i = 0 ; i < 0x42; i++) {
		printf ("%.02x ", *( ((unsigned char*)d)+i));
		if ( (i+1) % 16 == 0) { printf("\n"); }
	}
	printf ("\n");
}

int bios_inst_check(unsigned char drive) {
	union REGS r;

	r.h.ah = 0x41;
	r.x.bx = 0x55aa;
	r.h.dl = drive;

	int86(0x13, &r,&r);
	if (r.x.cflag) {
		//printf ("error occured: %d\n", r.x.ax);
		return r.x.ax;
	}

	if (r.x.bx == 0xaa55) {
		printf ("disk: %d\nmajor version of extensions: ", drive-0x80);
		switch(r.x.ax >> 8) {
		case 0x1:	printf ("1.x\n");
				break;

		case 0x20:	printf ("2.0 / EDD-1.0\n");
				break;

		case 0x21:	printf ("2.1 / EDD-1.1\n");
				break;

		case 0x30:	printf ("EDD-3.0\n");
				break;

		default:
				printf ("unknown\n");
		}

		printf ("API subset support bitmap:\n");
		if (r.x.cx & 1) {
			printf ("\textended disk access functions (42h-44h,47h,48h)\n");
		}
		if ((r.x.cx >> 1) & 1) {
			printf ("\tremovable drive controller functions (45h,46h,48h,49h,INT 15h/AH=52h)\n");
		}

		if ((r.x.cx >> 2) & 1) {
			printf ("\tenhanced disk drive (EDD) functions (48h,4Eh)\n");
		}
	}
	else {
		printf ("drive not installed.\n");
	}

	return 0;
}
