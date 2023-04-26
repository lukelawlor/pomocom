/*
 * ansi_term.hh contains macros for strings containing ANSI terminal escape code sequences.
 */

#pragma once

#define	AT_CLEAR	"\033[2J\033[0;0H"
#define	AT_CLEAR_LINE	"\r\033[K"
#define	AT_CLEAR_REST	"\033[K"
#define	AT_CUR_UP(x)	"\033[" #x "A"
#define	AT_CUR_DOWN(x)	"\033[" #x "B"
#define	AT_CUR_LEFT(x)	"\033[" #x "D"
#define	AT_CUR_RIGHT(x)	"\033[" #x "C"
