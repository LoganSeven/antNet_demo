/* stub_headers/stdbool.h
   Minimal C99 boolean definitions for pycparser preprocessing */
   #ifndef _STDBOOL_H
   #define _STDBOOL_H
   
   #ifndef __cplusplus      /* C++ has built-in bool */
   #  define bool  _Bool
   #  define true  1
   #  define false 0
   #endif
   
   #define __bool_true_false_are_defined 1
   #endif /* _STDBOOL_H */
   