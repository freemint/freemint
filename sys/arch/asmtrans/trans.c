/*
 * translation routines; sometimes we pass data on
 * through directly (asm syntax->asm syntax, with
 * some macro expansion), and other times we also
 * do asm syntax->gas syntax translation
 */

#include "asmtrans.h"

int syntax = ASM;  /* GAS; */

char *
immediate(char *op)
{
	return concat("#", op);
}

char *
indirect(char *op)
{
	if (syntax == GAS) {
		return concat(op, "@");
	} else {
		return concat3("(", op, ")");
	}
}

char *
postinc(char *op)
{
	if (syntax == GAS) {
		return concat(op, "@+");
	} else {
		return concat3("(", op, ")+");
	}
}

char *
predec(char *op)
{
	if (syntax == GAS) {
		return concat(op, "@-");
	} else {
		return concat3("-(", op, ")");
	}
}

char *
indexed(char *op1, char *op2)
{
	if (syntax == GAS) {
		return concat4(op2, "@(", op1, ")");
	} else {
		return concat4(op1, "(", op2, ")");
	}
}

char *
sizedop(char *op, char *size)
{
	if (syntax == GAS) {
		return changesiz(concat(op, size));
	} else {
		return concat4("(", op, ")", size);
	}
}

char *
twoindex(char *disp, char *base, char *index)
{
	if (syntax == GAS) {
		return concat6(base, "@(", disp, ",", changesiz2(index), ")");
	} else {
		return concat6(disp, "(", base, ",", index, ")");
	}
}

char *
bitfield(char *op1, char *op2, char *op3)
{
	if (syntax == GAS) {
		return concat6(op1, "{#", op2, ":#", op3, "}");
	} else {
		return concat6(op1, "{", op2, ":", op3, "}");
	}
}

char *
postindex(char *bd, char *an, char *index, char *od)
{
	if (syntax == GAS) {
		return concat8(an, "@(", bd, ")@(", od, ",", changesiz2(index), ")");
	} else {
		return concat9("([",an, ",", bd, "],", index, ",", od ,")");
	}
}

char *
postindex0(char *bd)
{
	if (syntax == GAS) {
		return concat3("@(", bd, ")@()");
	} else {
		return concat3("([", bd, "])");
	}
}

char *
postindex1(char *bd, char *od)
{
	if (syntax == GAS) {
		return concat5("@(", bd, ")@(", od, ")");
	} else {
		return concat5("([", bd, "],", od, ")");
	}
}

char *
preindex(char *bd, char *an, char *index, char *od)
{
	if (syntax == GAS) {
		return concat8(an, "@(", bd, ",", changesiz2(index), ")@(", od, ")");
	} else {
		return concat9("([",an, ",", bd, ",", index, "],", od, ")");
	}
}

char *
do_ops(char *label, char *opcode, char *space, char *operand)
{
	static char temp[LINSIZ];
	static char optemp[40];
	char *to, *from;
	char c;

	if (syntax == GAS) {
	    if (!strcmp(opcode, "ds.l")) {
		strcpy(temp, "\t.even\n\t.comm");
		strcat(temp, space);
		strcat(temp, label);
		strcat(temp, ",");
		strcat(temp, "4*");
		strcat(temp, operand);
		return strdup(temp);
	    } else if (!strcmp(opcode, "ds.w")) {
		strcpy(temp, "\t.even\n\t.comm");
		strcat(temp, space);
		strcat(temp, label);
		strcat(temp, ",");
		strcat(temp, "2*");
		strcat(temp, operand);
		return strdup(temp);
	    } else if (!strcmp(opcode, "ds.b")) {
		strcpy(temp, "\t.comm");
		strcat(temp, space);
		strcat(temp, label);
		strcat(temp, ",");
		strcat(temp, operand);
		return strdup(temp);
	    } else {
		to = optemp;
		from = opcode;
		for(;;) {
			c = *from++;
			if (!c) break;
	/* change 'foo.b' -> 'foob' */
	/* special case: bra.s -> 'bra', since gas automatically
	 * selects offset sizes and since some gas's actually
	 * mess up if an explicit '.s' is given
	 */
			if (c == '.' && *from && from[1] == 0) {
				if (*from == 's') from++;
				continue;
			}
			*to++ = c;
		}
		*to = 0;
		opcode = optemp;
	    }
	}

	to = temp;

	if (*label) {
		int colonseen = 0;
		char c;

		for (from = label; (c = *from++) != 0;) {
			if (c == ':') colonseen = 1;
			*to++ = c;
		}
/* gas labels must have a ':' after them */
		if (!colonseen && syntax == GAS)
			*to++ = ':';
	}

	*to++ = '\t';
	strcpy(to, opcode);
	strcat(temp, space);
	strcat(temp, operand);
	return strdup(temp);
}

char *
changesiz(char *op)
{
	char *r = op;

	if (syntax != GAS) return op;

	while (*op) {
		if (*op == '.') *op = ':';
		op++;
	}
	return r;
}

char *
changesiz2(char *op)		/* rw: hack for scaled index	*/
	         
{
	char *r = op;

	if (syntax != GAS) return op;

	while (*op) {
		if (*op == '.' || *op == '*' ) *op = ':';
		op++;
	}
	return r;
}

char *
hexop(char *op)
{
	if (syntax == GAS)
		return concat("0x", op);
	else
		return concat("$", op);
}
