#ifndef header_1662374371_2c6040b4_c674_427d_a970_f21c684dd81a_interpreter_h
#define header_1662374371_2c6040b4_c674_427d_a970_f21c684dd81a_interpreter_h

struct call_state;

/* Errors:
 * -EINVAL: Illegal instruction
 * -EFAULT: Execution error
 * -ENOMEM: Out of memory
 */
int interpreter_exec(struct call_state* callstate);

#endif

