/* martin (c) 2020 */

/* WIP */

#include <stdio.h>
#include <conio.h>
#include <mem.h>
#include <dos.h>

#define	VERSION	0.121

#define OFFSETOF(type,field)	((unsigned long) &(((type*)0)->field))

// XXX: one standard says the size should not be more than 30
// XXX: bufsize > 30 is adjusted back to 30/1Eh on return
struct drive_buf_t {
	int bufsize;
	int flags;

	/* physical CHS (could be incorrect); use int 13h/AH=08 to get logical CHS */
	unsigned long int c;				// one higher than max usable
	unsigned long int h;				// one higher than max usable
	unsigned long int s;
	unsigned long int lo_total_s;
	unsigned long int hi_total_s;
	unsigned int bps;					// one higher than max usable
	int far* edd_param;

	char pad[32];
};

// standard fixed disk parameter table
// res values could be used for translated table (cyl > 1024)
// but still this applies only to drives 80h and 81h
struct fdpt_t {
	unsigned int phys_c;			// physical number of cylinders
	unsigned char phys_h;			// physical number of heads
	char res1[2];					// reserved
	unsigned int precomp;			// precompensation (obsolete)
	char res3;						// reserved
	char dcb;						// drive control byte
	int res4;						// reserved
	char res5;						// reserved
	int lz;							// landing zone (obsolete)
	unsigned char spt;				// sectors per track
	char res6;						// reserved
};

int bios_inst_check(unsigned char drive);
int bios_drive_param(unsigned char drive, struct drive_buf_t* d);
void dump_buffer(struct drive_buf_t* d);

int main() {
	struct drive_buf_t dbuf;
	unsigned long int total_from_chs;
	int i;

	struct fdpt_t far* f;

	clrscr();

	// let's try for 10 disks
	for (i=0; i < 10; i++) {
		if (! bios_inst_check(0x80+i)) {
			memset(&dbuf, 0, sizeof(dbuf));
			dbuf.bufsize = 0x42;

			if (!bios_drive_param(0x80+i, &dbuf)) {

				total_from_chs = dbuf.c*dbuf.h*dbuf.s*dbuf.bps;
				printf ("\nCHS: %ld/%ld/%ld, sector size: %d, size: %ld\n",
					 dbuf.c, dbuf.h, dbuf.s, dbuf.bps, total_from_chs);

				dump_buffer(&dbuf);

				printf ("flags: %x\n", dbuf.flags);

				printf ("EDD param ptr far: %Fp\n", dbuf.edd_param);

				printf ("dbuf: %p:%p\n", FP_SEG(&dbuf), FP_OFF(&dbuf));
				getch();

			}
			printf ("\n");
		}
		else {
			break;
		}
	}
	return 0;
}

int bios_drive_param(unsigned char drive, struct drive_buf_t* d) {
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

void dump_buffer(struct drive_buf_t* d) {
	int i;
	printf ("\nstruct dump:\n");

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

		// if this is true then edd_param makes sense (except FFFF:FFFF)
		if ((r.x.cx >> 2) & 1) {
			printf ("\tenhanced disk drive (EDD) functions (48h,4Eh)\n");
		}
	}
	else {
		printf ("drive not installed.\n");
	}
	return 0;
}
