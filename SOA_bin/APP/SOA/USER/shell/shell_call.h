#ifndef __SHELL_CALL_H
#define __SHELL_CALL_H

eExecStatus shell_clear_process( int ac, 
                   signed char *av[],
                   signed char **ppcStringReply);

eExecStatus shell_set_process( int ac,
                      signed char *av[],
                      signed char **ppcStringReply);

eExecStatus shell_show_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply);

eExecStatus shell_event_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply);

eExecStatus shell_ctrl_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply);

eExecStatus shell_hb_process( int ac, 
                          signed char *av[],
                          signed char **ppcStringReply);

eExecStatus shell_log_process( int ac,
                                signed char *av[],
                                signed char **ppcStringReply);
#endif