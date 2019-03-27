/*
 * show DOS information
 *
 * (c) 2019 martin
 *
 */

#include <stdio.h>
#include <dos.h>

struct dosver {
	char maj;
	char min;
};

char far* dos_invars(void);
void dos_version(struct dosver* v);

int main(void) {

	char far* invars;
	struct dosver ver;

	invars = dos_invars();
	dos_version(&ver);

	printf ("\nDOS version:\t%d.%d\n", ver.maj, ver.min);

	// XXX: is the in_vars segment the DOSGROUP segemnt ?
	// XXX: or is the in_vars variable actually defined
	// https://github.com/Microsoft/MS-DOS/blob/master/v2.0/source/GETSET.ASM
	printf ("in_vars:\t%Fp\n", invars);

	return 0;
}
void dos_version(struct dosver* v) {
	struct REGPACK regs;
	regs.r_ax = 0x3000;
	intr(0x21, &regs);

	v->min = (regs.r_ax >> 8) & 0xff;
	v->maj = regs.r_ax & 0xff;
}

char far* dos_invars(void) {
	struct REGPACK regs;

	regs.r_ax = 0x5200;
	intr(0x21, &regs);

	return (char far*)MK_FP(regs.r_es, regs.r_bx);
}
