#ifndef Py_OPCODE_H
#define Py_OPCODE_H
#ifdef __cplusplus
extern "C" {
#endif

//#define NO_OPCODE_LGB

#ifndef NO_OPCODE_LGB
/* Instruction opcodes for compiled code */
#define STOP_CODE	0
#define POP_TOP		1
#define ROT_TWO		2
#define ROT_THREE	3
#define DUP_TOP		4
#define ROT_FOUR	5
#define NOP		9

#define UNARY_POSITIVE	10
#define UNARY_NEGATIVE	11
#define UNARY_NOT	12
#define UNARY_CONVERT	13

#define UNARY_INVERT	15

#define BINARY_POWER	19

#define BINARY_MULTIPLY	20
#define BINARY_DIVIDE	21
#define BINARY_MODULO	22
#define BINARY_ADD	23
#define BINARY_SUBTRACT	24
#define BINARY_SUBSCR	25
#define BINARY_FLOOR_DIVIDE 26
#define BINARY_TRUE_DIVIDE 27
#define INPLACE_FLOOR_DIVIDE 28
#define INPLACE_TRUE_DIVIDE 29

#define SLICE		30
/* Also uses 31-33 */
#define SLICE_1		31
#define SLICE_2		32
#define SLICE_3		33

#define STORE_SLICE	40
/* Also uses 41-43 */
#define STORE_SLICE_1	41
#define STORE_SLICE_2	42
#define STORE_SLICE_3	43

#define DELETE_SLICE	50
/* Also uses 51-53 */
#define DELETE_SLICE_1	51
#define DELETE_SLICE_2	52
#define DELETE_SLICE_3	53

#define STORE_MAP	54
#define INPLACE_ADD	55
#define INPLACE_SUBTRACT	56
#define INPLACE_MULTIPLY	57
#define INPLACE_DIVIDE	58
#define INPLACE_MODULO	59
#define STORE_SUBSCR	60
#define DELETE_SUBSCR	61

#define BINARY_LSHIFT	62
#define BINARY_RSHIFT	63
#define BINARY_AND	64
#define BINARY_XOR	65
#define BINARY_OR	66
#define INPLACE_POWER	67
#define GET_ITER	68

#define PRINT_EXPR	70
#define PRINT_ITEM	71
#define PRINT_NEWLINE	72
#define PRINT_ITEM_TO   73
#define PRINT_NEWLINE_TO 74
#define INPLACE_LSHIFT	75
#define INPLACE_RSHIFT	76
#define INPLACE_AND	77
#define INPLACE_XOR	78
#define INPLACE_OR	79
#define BREAK_LOOP	80
#define WITH_CLEANUP    81
#define LOAD_LOCALS	82
#define RETURN_VALUE	83
#define IMPORT_STAR	84
#define EXEC_STMT	85
#define YIELD_VALUE	86
#define POP_BLOCK	87
#define END_FINALLY	88
#define BUILD_CLASS	89

#define HAVE_ARGUMENT	90	/* Opcodes from here have an argument: */

#define STORE_NAME	90	/* Index in name list */
#define DELETE_NAME	91	/* "" */
#define UNPACK_SEQUENCE	92	/* Number of sequence items */
#define FOR_ITER	93
#define LIST_APPEND	94

#define STORE_ATTR	95	/* Index in name list */
#define DELETE_ATTR	96	/* "" */
#define STORE_GLOBAL	97	/* "" */
#define DELETE_GLOBAL	98	/* "" */
#define DUP_TOPX	99	/* number of items to duplicate */
#define LOAD_CONST	100	/* Index in const list */
#define LOAD_NAME	101	/* Index in name list */
#define BUILD_TUPLE	102	/* Number of tuple items */
#define BUILD_LIST	103	/* Number of list items */
#define BUILD_SET	104     /* Number of set items */
#define BUILD_MAP	105	/* Always zero for now */
#define LOAD_ATTR	106	/* Index in name list */
#define COMPARE_OP	107	/* Comparison operator */
#define IMPORT_NAME	108	/* Index in name list */
#define IMPORT_FROM	109	/* Index in name list */
#define JUMP_FORWARD	110	/* Number of bytes to skip */

#define JUMP_IF_FALSE_OR_POP 111 /* Target byte offset from beginning
                                    of code */
#define JUMP_IF_TRUE_OR_POP 112	/* "" */
#define JUMP_ABSOLUTE	113	/* "" */
#define POP_JUMP_IF_FALSE 114	/* "" */
#define POP_JUMP_IF_TRUE 115	/* "" */

#define LOAD_GLOBAL	116	/* Index in name list */

#define CONTINUE_LOOP	119	/* Start of loop (absolute) */
#define SETUP_LOOP	120	/* Target address (relative) */
#define SETUP_EXCEPT	121	/* "" */
#define SETUP_FINALLY	122	/* "" */

#define LOAD_FAST	124	/* Local variable number */
#define STORE_FAST	125	/* Local variable number */
#define DELETE_FAST	126	/* Local variable number */

#define RAISE_VARARGS	130	/* Number of raise arguments (1, 2 or 3) */
/* CALL_FUNCTION_XXX opcodes defined below depend on this definition */
#define CALL_FUNCTION	131	/* #args + (#kwargs<<8) */
#define MAKE_FUNCTION	132	/* #defaults */
#define BUILD_SLICE 	133	/* Number of items */

#define MAKE_CLOSURE    134     /* #free vars */
#define LOAD_CLOSURE    135     /* Load free variable from closure */
#define LOAD_DEREF      136     /* Load and dereference from closure cell */ 
#define STORE_DEREF     137     /* Store into cell */ 

/* The next 3 opcodes must be contiguous and satisfy
   (CALL_FUNCTION_VAR - CALL_FUNCTION) & 3 == 1  */
#define CALL_FUNCTION_VAR          140	/* #args + (#kwargs<<8) */
#define CALL_FUNCTION_KW           141	/* #args + (#kwargs<<8) */
#define CALL_FUNCTION_VAR_KW       142	/* #args + (#kwargs<<8) */

#define SETUP_WITH 143

/* Support for opargs more than 16 bits long */
#define EXTENDED_ARG  145

#define SET_ADD         146
#define MAP_ADD         147

#else

#define STOP_CODE       58
#define POP_TOP         12
#define ROT_TWO         91
#define ROT_THREE       7
#define DUP_TOP         33
#define ROT_FOUR        19
#define NOP             42

#define UNARY_POSITIVE  23
#define UNARY_NEGATIVE  64
#define UNARY_NOT       85
#define UNARY_CONVERT   3

#define UNARY_INVERT    97

#define BINARY_POWER    37

#define BINARY_MULTIPLY 14
#define BINARY_DIVIDE   72
#define BINARY_MODULO   29
#define BINARY_ADD      50
#define BINARY_SUBTRACT 61
#define BINARY_SUBSCR   81
#define BINARY_FLOOR_DIVIDE 9
#define BINARY_TRUE_DIVIDE 44
#define INPLACE_FLOOR_DIVIDE 67
#define INPLACE_TRUE_DIVIDE 26

#define SLICE           55
#define SLICE_1         93
#define SLICE_2         17
#define SLICE_3         76

#define STORE_SLICE     48
#define STORE_SLICE_1   21
#define STORE_SLICE_2   60
#define STORE_SLICE_3   84

#define DELETE_SLICE    35
#define DELETE_SLICE_1  70
#define DELETE_SLICE_2  5
#define DELETE_SLICE_3  99

#define STORE_MAP       45
#define INPLACE_ADD     18
#define INPLACE_SUBTRACT 77
#define INPLACE_MULTIPLY 53
#define INPLACE_DIVIDE  30
#define INPLACE_MODULO  66
#define STORE_SUBSCR    22
#define DELETE_SUBSCR   71

#define BINARY_LSHIFT   40
#define BINARY_RSHIFT   59
#define BINARY_AND      83
#define BINARY_XOR      11
#define BINARY_OR       49
#define INPLACE_POWER   68
#define GET_ITER        96

#define PRINT_EXPR      27
#define PRINT_ITEM      73
#define PRINT_NEWLINE   62
#define PRINT_ITEM_TO   34
#define PRINT_NEWLINE_TO 8
#define INPLACE_LSHIFT  79
#define INPLACE_RSHIFT  41
#define INPLACE_AND     57
#define INPLACE_XOR     20
#define INPLACE_OR      74
#define BREAK_LOOP      65
#define WITH_CLEANUP    51
#define LOAD_LOCALS     92
#define RETURN_VALUE    38
#define IMPORT_STAR     13
#define EXEC_STMT       78
#define YIELD_VALUE     63
#define POP_BLOCK       24
#define END_FINALLY     46
#define BUILD_CLASS     75

#define HAVE_ARGUMENT   94

#define STORE_NAME      2
#define DELETE_NAME     69
#define UNPACK_SEQUENCE 80
#define FOR_ITER        36
#define LIST_APPEND     95

#define STORE_ATTR      52
#define DELETE_ATTR     43
#define STORE_GLOBAL    25
#define DELETE_GLOBAL   56
#define DUP_TOPX        47
#define LOAD_CONST      10
#define LOAD_NAME       82
#define BUILD_TUPLE     31
#define BUILD_LIST      54
#define BUILD_SET       39
#define BUILD_MAP       89
#define LOAD_ATTR       90
#define COMPARE_OP      88
#define IMPORT_NAME     6
#define IMPORT_FROM     28
#define JUMP_FORWARD    87

#define JUMP_IF_FALSE_OR_POP 1
#define JUMP_IF_TRUE_OR_POP 16
#define JUMP_ABSOLUTE   86
#define POP_JUMP_IF_FALSE 4
#define POP_JUMP_IF_TRUE 32

#define LOAD_GLOBAL     98

#define CONTINUE_LOOP   15
#define SETUP_LOOP      100
#define SETUP_EXCEPT    101
#define SETUP_FINALLY   102

#define LOAD_FAST       103
#define STORE_FAST      104
#define DELETE_FAST     105

#define RAISE_VARARGS   106
#define CALL_FUNCTION   107
#define MAKE_FUNCTION   108
#define BUILD_SLICE     109

#define MAKE_CLOSURE    110
#define LOAD_CLOSURE    111
#define LOAD_DEREF      112
#define STORE_DEREF     113

#define CALL_FUNCTION_VAR    114
#define CALL_FUNCTION_KW     115
#define CALL_FUNCTION_VAR_KW 116

#define SETUP_WITH      117

#define EXTENDED_ARG    118

#define SET_ADD         119
#define MAP_ADD         120
#endif


enum cmp_op {PyCmp_LT=Py_LT, PyCmp_LE=Py_LE, PyCmp_EQ=Py_EQ, PyCmp_NE=Py_NE, PyCmp_GT=Py_GT, PyCmp_GE=Py_GE,
	     PyCmp_IN, PyCmp_NOT_IN, PyCmp_IS, PyCmp_IS_NOT, PyCmp_EXC_MATCH, PyCmp_BAD};

#define HAS_ARG(op) ((op) >= HAVE_ARGUMENT)

#ifdef __cplusplus
}
#endif
#endif /* !Py_OPCODE_H */
