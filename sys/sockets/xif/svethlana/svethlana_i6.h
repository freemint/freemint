# ifndef _svethlana_i6_h
# define _svethlana_i6_h


// old handler
extern void (*old_i6_int)(void);

// interrupt wrapper routine
void interrupt_i6 (void);


void set_old_int_lvl(void);
void set_int_lvl6(void);

void int_off(void);
void int_on(void);

# endif // _svethlana_i6_h
