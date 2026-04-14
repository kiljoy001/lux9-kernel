# 0 "wasm/wasm_runtime.c"
# 1 "/home/scott/Repo/lux9-kernel/kernel//"
# 0 "<built-in>"
#define __STDC__ 1
# 0 "<built-in>"
#define __STDC_VERSION__ 201112L
# 0 "<built-in>"
#define __STDC_UTF_16__ 1
# 0 "<built-in>"
#define __STDC_UTF_32__ 1
# 0 "<built-in>"
#define __STDC_HOSTED__ 0
# 0 "<built-in>"
#define __GNUC__ 13
# 0 "<built-in>"
#define __GNUC_MINOR__ 3
# 0 "<built-in>"
#define __GNUC_PATCHLEVEL__ 0
# 0 "<built-in>"
#define __VERSION__ "13.3.0"
# 0 "<built-in>"
#define __ATOMIC_RELAXED 0
# 0 "<built-in>"
#define __ATOMIC_SEQ_CST 5
# 0 "<built-in>"
#define __ATOMIC_ACQUIRE 2
# 0 "<built-in>"
#define __ATOMIC_RELEASE 3
# 0 "<built-in>"
#define __ATOMIC_ACQ_REL 4
# 0 "<built-in>"
#define __ATOMIC_CONSUME 1
# 0 "<built-in>"
#define __FINITE_MATH_ONLY__ 0
# 0 "<built-in>"
#define _LP64 1
# 0 "<built-in>"
#define __LP64__ 1
# 0 "<built-in>"
#define __SIZEOF_INT__ 4
# 0 "<built-in>"
#define __SIZEOF_LONG__ 8
# 0 "<built-in>"
#define __SIZEOF_LONG_LONG__ 8
# 0 "<built-in>"
#define __SIZEOF_SHORT__ 2
# 0 "<built-in>"
#define __SIZEOF_FLOAT__ 4
# 0 "<built-in>"
#define __SIZEOF_DOUBLE__ 8
# 0 "<built-in>"
#define __SIZEOF_LONG_DOUBLE__ 16
# 0 "<built-in>"
#define __SIZEOF_SIZE_T__ 8
# 0 "<built-in>"
#define __CHAR_BIT__ 8
# 0 "<built-in>"
#define __BIGGEST_ALIGNMENT__ 16
# 0 "<built-in>"
#define __ORDER_LITTLE_ENDIAN__ 1234
# 0 "<built-in>"
#define __ORDER_BIG_ENDIAN__ 4321
# 0 "<built-in>"
#define __ORDER_PDP_ENDIAN__ 3412
# 0 "<built-in>"
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
# 0 "<built-in>"
#define __FLOAT_WORD_ORDER__ __ORDER_LITTLE_ENDIAN__
# 0 "<built-in>"
#define __SIZEOF_POINTER__ 8
# 0 "<built-in>"
#define __GNUC_EXECUTION_CHARSET_NAME "UTF-8"
# 0 "<built-in>"
#define __GNUC_WIDE_EXECUTION_CHARSET_NAME "UTF-32LE"
# 0 "<built-in>"
#define __SIZE_TYPE__ long unsigned int
# 0 "<built-in>"
#define __PTRDIFF_TYPE__ long int
# 0 "<built-in>"
#define __WCHAR_TYPE__ int
# 0 "<built-in>"
#define __WINT_TYPE__ unsigned int
# 0 "<built-in>"
#define __INTMAX_TYPE__ long int
# 0 "<built-in>"
#define __UINTMAX_TYPE__ long unsigned int
# 0 "<built-in>"
#define __CHAR16_TYPE__ short unsigned int
# 0 "<built-in>"
#define __CHAR32_TYPE__ unsigned int
# 0 "<built-in>"
#define __SIG_ATOMIC_TYPE__ int
# 0 "<built-in>"
#define __INT8_TYPE__ signed char
# 0 "<built-in>"
#define __INT16_TYPE__ short int
# 0 "<built-in>"
#define __INT32_TYPE__ int
# 0 "<built-in>"
#define __INT64_TYPE__ long int
# 0 "<built-in>"
#define __UINT8_TYPE__ unsigned char
# 0 "<built-in>"
#define __UINT16_TYPE__ short unsigned int
# 0 "<built-in>"
#define __UINT32_TYPE__ unsigned int
# 0 "<built-in>"
#define __UINT64_TYPE__ long unsigned int
# 0 "<built-in>"
#define __INT_LEAST8_TYPE__ signed char
# 0 "<built-in>"
#define __INT_LEAST16_TYPE__ short int
# 0 "<built-in>"
#define __INT_LEAST32_TYPE__ int
# 0 "<built-in>"
#define __INT_LEAST64_TYPE__ long int
# 0 "<built-in>"
#define __UINT_LEAST8_TYPE__ unsigned char
# 0 "<built-in>"
#define __UINT_LEAST16_TYPE__ short unsigned int
# 0 "<built-in>"
#define __UINT_LEAST32_TYPE__ unsigned int
# 0 "<built-in>"
#define __UINT_LEAST64_TYPE__ long unsigned int
# 0 "<built-in>"
#define __INT_FAST8_TYPE__ signed char
# 0 "<built-in>"
#define __INT_FAST16_TYPE__ long int
# 0 "<built-in>"
#define __INT_FAST32_TYPE__ long int
# 0 "<built-in>"
#define __INT_FAST64_TYPE__ long int
# 0 "<built-in>"
#define __UINT_FAST8_TYPE__ unsigned char
# 0 "<built-in>"
#define __UINT_FAST16_TYPE__ long unsigned int
# 0 "<built-in>"
#define __UINT_FAST32_TYPE__ long unsigned int
# 0 "<built-in>"
#define __UINT_FAST64_TYPE__ long unsigned int
# 0 "<built-in>"
#define __INTPTR_TYPE__ long int
# 0 "<built-in>"
#define __UINTPTR_TYPE__ long unsigned int
# 0 "<built-in>"
#define __GXX_ABI_VERSION 1018
# 0 "<built-in>"
#define __SCHAR_MAX__ 0x7f
# 0 "<built-in>"
#define __SHRT_MAX__ 0x7fff
# 0 "<built-in>"
#define __INT_MAX__ 0x7fffffff
# 0 "<built-in>"
#define __LONG_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __LONG_LONG_MAX__ 0x7fffffffffffffffLL
# 0 "<built-in>"
#define __WCHAR_MAX__ 0x7fffffff
# 0 "<built-in>"
#define __WCHAR_MIN__ (-__WCHAR_MAX__ - 1)
# 0 "<built-in>"
#define __WINT_MAX__ 0xffffffffU
# 0 "<built-in>"
#define __WINT_MIN__ 0U
# 0 "<built-in>"
#define __PTRDIFF_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __SIZE_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __SCHAR_WIDTH__ 8
# 0 "<built-in>"
#define __SHRT_WIDTH__ 16
# 0 "<built-in>"
#define __INT_WIDTH__ 32
# 0 "<built-in>"
#define __LONG_WIDTH__ 64
# 0 "<built-in>"
#define __LONG_LONG_WIDTH__ 64
# 0 "<built-in>"
#define __WCHAR_WIDTH__ 32
# 0 "<built-in>"
#define __WINT_WIDTH__ 32
# 0 "<built-in>"
#define __PTRDIFF_WIDTH__ 64
# 0 "<built-in>"
#define __SIZE_WIDTH__ 64
# 0 "<built-in>"
#define __INTMAX_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INTMAX_C(c) c ## L
# 0 "<built-in>"
#define __UINTMAX_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __UINTMAX_C(c) c ## UL
# 0 "<built-in>"
#define __INTMAX_WIDTH__ 64
# 0 "<built-in>"
#define __SIG_ATOMIC_MAX__ 0x7fffffff
# 0 "<built-in>"
#define __SIG_ATOMIC_MIN__ (-__SIG_ATOMIC_MAX__ - 1)
# 0 "<built-in>"
#define __SIG_ATOMIC_WIDTH__ 32
# 0 "<built-in>"
#define __INT8_MAX__ 0x7f
# 0 "<built-in>"
#define __INT16_MAX__ 0x7fff
# 0 "<built-in>"
#define __INT32_MAX__ 0x7fffffff
# 0 "<built-in>"
#define __INT64_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __UINT8_MAX__ 0xff
# 0 "<built-in>"
#define __UINT16_MAX__ 0xffff
# 0 "<built-in>"
#define __UINT32_MAX__ 0xffffffffU
# 0 "<built-in>"
#define __UINT64_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __INT_LEAST8_MAX__ 0x7f
# 0 "<built-in>"
#define __INT8_C(c) c
# 0 "<built-in>"
#define __INT_LEAST8_WIDTH__ 8
# 0 "<built-in>"
#define __INT_LEAST16_MAX__ 0x7fff
# 0 "<built-in>"
#define __INT16_C(c) c
# 0 "<built-in>"
#define __INT_LEAST16_WIDTH__ 16
# 0 "<built-in>"
#define __INT_LEAST32_MAX__ 0x7fffffff
# 0 "<built-in>"
#define __INT32_C(c) c
# 0 "<built-in>"
#define __INT_LEAST32_WIDTH__ 32
# 0 "<built-in>"
#define __INT_LEAST64_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INT64_C(c) c ## L
# 0 "<built-in>"
#define __INT_LEAST64_WIDTH__ 64
# 0 "<built-in>"
#define __UINT_LEAST8_MAX__ 0xff
# 0 "<built-in>"
#define __UINT8_C(c) c
# 0 "<built-in>"
#define __UINT_LEAST16_MAX__ 0xffff
# 0 "<built-in>"
#define __UINT16_C(c) c
# 0 "<built-in>"
#define __UINT_LEAST32_MAX__ 0xffffffffU
# 0 "<built-in>"
#define __UINT32_C(c) c ## U
# 0 "<built-in>"
#define __UINT_LEAST64_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __UINT64_C(c) c ## UL
# 0 "<built-in>"
#define __INT_FAST8_MAX__ 0x7f
# 0 "<built-in>"
#define __INT_FAST8_WIDTH__ 8
# 0 "<built-in>"
#define __INT_FAST16_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INT_FAST16_WIDTH__ 64
# 0 "<built-in>"
#define __INT_FAST32_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INT_FAST32_WIDTH__ 64
# 0 "<built-in>"
#define __INT_FAST64_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INT_FAST64_WIDTH__ 64
# 0 "<built-in>"
#define __UINT_FAST8_MAX__ 0xff
# 0 "<built-in>"
#define __UINT_FAST16_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __UINT_FAST32_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __UINT_FAST64_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __INTPTR_MAX__ 0x7fffffffffffffffL
# 0 "<built-in>"
#define __INTPTR_WIDTH__ 64
# 0 "<built-in>"
#define __UINTPTR_MAX__ 0xffffffffffffffffUL
# 0 "<built-in>"
#define __GCC_IEC_559 0
# 0 "<built-in>"
#define __GCC_IEC_559_COMPLEX 0
# 0 "<built-in>"
#define __FLT_EVAL_METHOD__ 0
# 0 "<built-in>"
#define __FLT_EVAL_METHOD_TS_18661_3__ 0
# 0 "<built-in>"
#define __DEC_EVAL_METHOD__ 2
# 0 "<built-in>"
#define __FLT_RADIX__ 2
# 0 "<built-in>"
#define __FLT_MANT_DIG__ 24
# 0 "<built-in>"
#define __FLT_DIG__ 6
# 0 "<built-in>"
#define __FLT_MIN_EXP__ (-125)
# 0 "<built-in>"
#define __FLT_MIN_10_EXP__ (-37)
# 0 "<built-in>"
#define __FLT_MAX_EXP__ 128
# 0 "<built-in>"
#define __FLT_MAX_10_EXP__ 38
# 0 "<built-in>"
#define __FLT_DECIMAL_DIG__ 9
# 0 "<built-in>"
#define __FLT_MAX__ 3.40282346638528859811704183484516925e+38F
# 0 "<built-in>"
#define __FLT_NORM_MAX__ 3.40282346638528859811704183484516925e+38F
# 0 "<built-in>"
#define __FLT_MIN__ 1.17549435082228750796873653722224568e-38F
# 0 "<built-in>"
#define __FLT_EPSILON__ 1.19209289550781250000000000000000000e-7F
# 0 "<built-in>"
#define __FLT_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F
# 0 "<built-in>"
#define __FLT_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __DBL_MANT_DIG__ 53
# 0 "<built-in>"
#define __DBL_DIG__ 15
# 0 "<built-in>"
#define __DBL_MIN_EXP__ (-1021)
# 0 "<built-in>"
#define __DBL_MIN_10_EXP__ (-307)
# 0 "<built-in>"
#define __DBL_MAX_EXP__ 1024
# 0 "<built-in>"
#define __DBL_MAX_10_EXP__ 308
# 0 "<built-in>"
#define __DBL_DECIMAL_DIG__ 17
# 0 "<built-in>"
#define __DBL_MAX__ ((double)1.79769313486231570814527423731704357e+308L)
# 0 "<built-in>"
#define __DBL_NORM_MAX__ ((double)1.79769313486231570814527423731704357e+308L)
# 0 "<built-in>"
#define __DBL_MIN__ ((double)2.22507385850720138309023271733240406e-308L)
# 0 "<built-in>"
#define __DBL_EPSILON__ ((double)2.22044604925031308084726333618164062e-16L)
# 0 "<built-in>"
#define __DBL_DENORM_MIN__ ((double)4.94065645841246544176568792868221372e-324L)
# 0 "<built-in>"
#define __DBL_HAS_DENORM__ 1
# 0 "<built-in>"
#define __DBL_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __DBL_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __DBL_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __LDBL_MANT_DIG__ 64
# 0 "<built-in>"
#define __LDBL_DIG__ 18
# 0 "<built-in>"
#define __LDBL_MIN_EXP__ (-16381)
# 0 "<built-in>"
#define __LDBL_MIN_10_EXP__ (-4931)
# 0 "<built-in>"
#define __LDBL_MAX_EXP__ 16384
# 0 "<built-in>"
#define __LDBL_MAX_10_EXP__ 4932
# 0 "<built-in>"
#define __DECIMAL_DIG__ 21
# 0 "<built-in>"
#define __LDBL_DECIMAL_DIG__ 21
# 0 "<built-in>"
#define __LDBL_MAX__ 1.18973149535723176502126385303097021e+4932L
# 0 "<built-in>"
#define __LDBL_NORM_MAX__ 1.18973149535723176502126385303097021e+4932L
# 0 "<built-in>"
#define __LDBL_MIN__ 3.36210314311209350626267781732175260e-4932L
# 0 "<built-in>"
#define __LDBL_EPSILON__ 1.08420217248550443400745280086994171e-19L
# 0 "<built-in>"
#define __LDBL_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951L
# 0 "<built-in>"
#define __LDBL_HAS_DENORM__ 1
# 0 "<built-in>"
#define __LDBL_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __LDBL_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __LDBL_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __FLT32_MANT_DIG__ 24
# 0 "<built-in>"
#define __FLT32_DIG__ 6
# 0 "<built-in>"
#define __FLT32_MIN_EXP__ (-125)
# 0 "<built-in>"
#define __FLT32_MIN_10_EXP__ (-37)
# 0 "<built-in>"
#define __FLT32_MAX_EXP__ 128
# 0 "<built-in>"
#define __FLT32_MAX_10_EXP__ 38
# 0 "<built-in>"
#define __FLT32_DECIMAL_DIG__ 9
# 0 "<built-in>"
#define __FLT32_MAX__ 3.40282346638528859811704183484516925e+38F32
# 0 "<built-in>"
#define __FLT32_NORM_MAX__ 3.40282346638528859811704183484516925e+38F32
# 0 "<built-in>"
#define __FLT32_MIN__ 1.17549435082228750796873653722224568e-38F32
# 0 "<built-in>"
#define __FLT32_EPSILON__ 1.19209289550781250000000000000000000e-7F32
# 0 "<built-in>"
#define __FLT32_DENORM_MIN__ 1.40129846432481707092372958328991613e-45F32
# 0 "<built-in>"
#define __FLT32_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT32_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT32_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT32_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __FLT64_MANT_DIG__ 53
# 0 "<built-in>"
#define __FLT64_DIG__ 15
# 0 "<built-in>"
#define __FLT64_MIN_EXP__ (-1021)
# 0 "<built-in>"
#define __FLT64_MIN_10_EXP__ (-307)
# 0 "<built-in>"
#define __FLT64_MAX_EXP__ 1024
# 0 "<built-in>"
#define __FLT64_MAX_10_EXP__ 308
# 0 "<built-in>"
#define __FLT64_DECIMAL_DIG__ 17
# 0 "<built-in>"
#define __FLT64_MAX__ 1.79769313486231570814527423731704357e+308F64
# 0 "<built-in>"
#define __FLT64_NORM_MAX__ 1.79769313486231570814527423731704357e+308F64
# 0 "<built-in>"
#define __FLT64_MIN__ 2.22507385850720138309023271733240406e-308F64
# 0 "<built-in>"
#define __FLT64_EPSILON__ 2.22044604925031308084726333618164062e-16F64
# 0 "<built-in>"
#define __FLT64_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F64
# 0 "<built-in>"
#define __FLT64_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT64_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT64_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT64_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __FLT128_MANT_DIG__ 113
# 0 "<built-in>"
#define __FLT128_DIG__ 33
# 0 "<built-in>"
#define __FLT128_MIN_EXP__ (-16381)
# 0 "<built-in>"
#define __FLT128_MIN_10_EXP__ (-4931)
# 0 "<built-in>"
#define __FLT128_MAX_EXP__ 16384
# 0 "<built-in>"
#define __FLT128_MAX_10_EXP__ 4932
# 0 "<built-in>"
#define __FLT128_DECIMAL_DIG__ 36
# 0 "<built-in>"
#define __FLT128_MAX__ 1.18973149535723176508575932662800702e+4932F128
# 0 "<built-in>"
#define __FLT128_NORM_MAX__ 1.18973149535723176508575932662800702e+4932F128
# 0 "<built-in>"
#define __FLT128_MIN__ 3.36210314311209350626267781732175260e-4932F128
# 0 "<built-in>"
#define __FLT128_EPSILON__ 1.92592994438723585305597794258492732e-34F128
# 0 "<built-in>"
#define __FLT128_DENORM_MIN__ 6.47517511943802511092443895822764655e-4966F128
# 0 "<built-in>"
#define __FLT128_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT128_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT128_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT128_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __FLT32X_MANT_DIG__ 53
# 0 "<built-in>"
#define __FLT32X_DIG__ 15
# 0 "<built-in>"
#define __FLT32X_MIN_EXP__ (-1021)
# 0 "<built-in>"
#define __FLT32X_MIN_10_EXP__ (-307)
# 0 "<built-in>"
#define __FLT32X_MAX_EXP__ 1024
# 0 "<built-in>"
#define __FLT32X_MAX_10_EXP__ 308
# 0 "<built-in>"
#define __FLT32X_DECIMAL_DIG__ 17
# 0 "<built-in>"
#define __FLT32X_MAX__ 1.79769313486231570814527423731704357e+308F32x
# 0 "<built-in>"
#define __FLT32X_NORM_MAX__ 1.79769313486231570814527423731704357e+308F32x
# 0 "<built-in>"
#define __FLT32X_MIN__ 2.22507385850720138309023271733240406e-308F32x
# 0 "<built-in>"
#define __FLT32X_EPSILON__ 2.22044604925031308084726333618164062e-16F32x
# 0 "<built-in>"
#define __FLT32X_DENORM_MIN__ 4.94065645841246544176568792868221372e-324F32x
# 0 "<built-in>"
#define __FLT32X_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT32X_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT32X_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT32X_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __FLT64X_MANT_DIG__ 64
# 0 "<built-in>"
#define __FLT64X_DIG__ 18
# 0 "<built-in>"
#define __FLT64X_MIN_EXP__ (-16381)
# 0 "<built-in>"
#define __FLT64X_MIN_10_EXP__ (-4931)
# 0 "<built-in>"
#define __FLT64X_MAX_EXP__ 16384
# 0 "<built-in>"
#define __FLT64X_MAX_10_EXP__ 4932
# 0 "<built-in>"
#define __FLT64X_DECIMAL_DIG__ 21
# 0 "<built-in>"
#define __FLT64X_MAX__ 1.18973149535723176502126385303097021e+4932F64x
# 0 "<built-in>"
#define __FLT64X_NORM_MAX__ 1.18973149535723176502126385303097021e+4932F64x
# 0 "<built-in>"
#define __FLT64X_MIN__ 3.36210314311209350626267781732175260e-4932F64x
# 0 "<built-in>"
#define __FLT64X_EPSILON__ 1.08420217248550443400745280086994171e-19F64x
# 0 "<built-in>"
#define __FLT64X_DENORM_MIN__ 3.64519953188247460252840593361941982e-4951F64x
# 0 "<built-in>"
#define __FLT64X_HAS_DENORM__ 1
# 0 "<built-in>"
#define __FLT64X_HAS_INFINITY__ 1
# 0 "<built-in>"
#define __FLT64X_HAS_QUIET_NAN__ 1
# 0 "<built-in>"
#define __FLT64X_IS_IEC_60559__ 1
# 0 "<built-in>"
#define __DEC32_MANT_DIG__ 7
# 0 "<built-in>"
#define __DEC32_MIN_EXP__ (-94)
# 0 "<built-in>"
#define __DEC32_MAX_EXP__ 97
# 0 "<built-in>"
#define __DEC32_MIN__ 1E-95DF
# 0 "<built-in>"
#define __DEC32_MAX__ 9.999999E96DF
# 0 "<built-in>"
#define __DEC32_EPSILON__ 1E-6DF
# 0 "<built-in>"
#define __DEC32_SUBNORMAL_MIN__ 0.000001E-95DF
# 0 "<built-in>"
#define __DEC64_MANT_DIG__ 16
# 0 "<built-in>"
#define __DEC64_MIN_EXP__ (-382)
# 0 "<built-in>"
#define __DEC64_MAX_EXP__ 385
# 0 "<built-in>"
#define __DEC64_MIN__ 1E-383DD
# 0 "<built-in>"
#define __DEC64_MAX__ 9.999999999999999E384DD
# 0 "<built-in>"
#define __DEC64_EPSILON__ 1E-15DD
# 0 "<built-in>"
#define __DEC64_SUBNORMAL_MIN__ 0.000000000000001E-383DD
# 0 "<built-in>"
#define __DEC128_MANT_DIG__ 34
# 0 "<built-in>"
#define __DEC128_MIN_EXP__ (-6142)
# 0 "<built-in>"
#define __DEC128_MAX_EXP__ 6145
# 0 "<built-in>"
#define __DEC128_MIN__ 1E-6143DL
# 0 "<built-in>"
#define __DEC128_MAX__ 9.999999999999999999999999999999999E6144DL
# 0 "<built-in>"
#define __DEC128_EPSILON__ 1E-33DL
# 0 "<built-in>"
#define __DEC128_SUBNORMAL_MIN__ 0.000000000000000000000000000000001E-6143DL
# 0 "<built-in>"
#define __REGISTER_PREFIX__ 
# 0 "<built-in>"
#define __USER_LABEL_PREFIX__ 
# 0 "<built-in>"
#define __GNUC_STDC_INLINE__ 1
# 0 "<built-in>"
#define __NO_INLINE__ 1
# 0 "<built-in>"
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_1 1
# 0 "<built-in>"
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_2 1
# 0 "<built-in>"
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 1
# 0 "<built-in>"
#define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_8 1
# 0 "<built-in>"
#define __GCC_ATOMIC_BOOL_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_CHAR_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_CHAR16_T_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_CHAR32_T_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_WCHAR_T_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_SHORT_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_INT_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_LONG_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_LLONG_LOCK_FREE 2
# 0 "<built-in>"
#define __GCC_ATOMIC_TEST_AND_SET_TRUEVAL 1
# 0 "<built-in>"
#define __GCC_DESTRUCTIVE_SIZE 64
# 0 "<built-in>"
#define __GCC_CONSTRUCTIVE_SIZE 64
# 0 "<built-in>"
#define __GCC_ATOMIC_POINTER_LOCK_FREE 2
# 0 "<built-in>"
#define __HAVE_SPECULATION_SAFE_VALUE 1
# 0 "<built-in>"
#define __GCC_HAVE_DWARF2_CFI_ASM 1
# 0 "<built-in>"
#define __PRAGMA_REDEFINE_EXTNAME 1
# 0 "<built-in>"
#define __SIZEOF_INT128__ 16
# 0 "<built-in>"
#define __SIZEOF_WCHAR_T__ 4
# 0 "<built-in>"
#define __SIZEOF_WINT_T__ 4
# 0 "<built-in>"
#define __SIZEOF_PTRDIFF_T__ 8
# 0 "<built-in>"
#define __amd64 1
# 0 "<built-in>"
#define __amd64__ 1
# 0 "<built-in>"
#define __x86_64 1
# 0 "<built-in>"
#define __x86_64__ 1
# 0 "<built-in>"
#define _SOFT_FLOAT 1
# 0 "<built-in>"
#define __SIZEOF_FLOAT80__ 16
# 0 "<built-in>"
#define __SIZEOF_FLOAT128__ 16
# 0 "<built-in>"
#define __ATOMIC_HLE_ACQUIRE 65536
# 0 "<built-in>"
#define __ATOMIC_HLE_RELEASE 131072
# 0 "<built-in>"
#define __GCC_ASM_FLAG_OUTPUTS__ 1
# 0 "<built-in>"
#define __k8 1
# 0 "<built-in>"
#define __k8__ 1
# 0 "<built-in>"
#define __code_model_kernel__ 1
# 0 "<built-in>"
#define __FXSR__ 1
# 0 "<built-in>"
#define __SEG_FS 1
# 0 "<built-in>"
#define __SEG_GS 1
# 0 "<built-in>"
#define __CET__ 3
# 0 "<built-in>"
#define __gnu_linux__ 1
# 0 "<built-in>"
#define __linux 1
# 0 "<built-in>"
#define __linux__ 1
# 0 "<built-in>"
#define linux 1
# 0 "<built-in>"
#define __unix 1
# 0 "<built-in>"
#define __unix__ 1
# 0 "<built-in>"
#define unix 1
# 0 "<built-in>"
#define __ELF__ 1
# 0 "<built-in>"
#define __DECIMAL_BID_FORMAT__ 1
# 0 "<command-line>"
#define _PLAN9_SOURCE 1
# 0 "<command-line>"
#define __PLAN9_KERNEL__ 1
# 0 "<command-line>"
#define KERNEL 1
# 0 "<command-line>"
#define KTZERO 0xffffffff80110000
# 1 "wasm/wasm_runtime.c"
# 14 "wasm/wasm_runtime.c"
# 1 "wasm/../include/dat.h" 1
       


# 1 "wasm/../include/mem.h" 1

#define _MEM_H_ 




#define KiB 1024u
#define MiB 1048576u
#define GiB 1073741824u
#define TiB 1099511627776ull
#define PiB 1125899906842624ull
#define EiB 1152921504606846976ull

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define ALIGNED(p,a) (!(((uintptr)(p)) & ((a) - 1)))




#define BI2BY 8
#define BI2WD (sizeof(ulong) * 8)
#define BY2WD sizeof(ulong)
#define BY2V sizeof(uvlong)
#define BY2PG (0x1000ull)
#define WD2PG (BY2PG / BY2WD)
#define PGSHIFT 12
#define ROUND(s,sz) (((s) + ((sz) - 1)) & ~((sz) - 1))
#define PGROUND(s) ROUND(s, BY2PG)
#define BLOCKALIGN 64
#define FPalign 64

#define MAXMACH 128

#define KSTACK (256 * KiB)






#define HZ (100)
#define MS2HZ (1000 / HZ)
#define TK2SEC(t) ((t) / HZ)




#define UTZERO (0x0000000000200000ull)
#define UADDRMASK (0x00007fffffffffffull)
#define USTKTOP (0x00007ffffffff000ull)
#define USTKSIZE (16 * MiB)


#define P9_VA_REGION_PAGES 256
#define P9_VA_REGION_SIZE (P9_VA_REGION_PAGES * BY2PG)
#define P9_VA_REGION_BASE (USTKTOP - USTKSIZE - P9_VA_REGION_SIZE)




#define KZERO (0xffffffff80000000ull)
#define PADDR(a) ((uintptr)(a) - KZERO)




#define VMAP (0xfffffe8000000000ull)
#define VMAPSIZE (1024ull * GiB)

#define KMAP (0xfffffe0000000000ull)
#define KMAPSIZE (2 * MiB)




#define CONFADDR (KZERO + 0x1200ull)
#define APBOOTSTRAP (KZERO + 0x7000ull)
#define IDTADDR (KZERO + 0x10000ull)
#define REBOOTADDR (0x11000)

#define CPU0PML4 (KZERO + 0x13000ull)
#define CPU0PDP (KZERO + 0x14000ull)
#define CPU0PD0 (KZERO + 0x15000ull)
#define CPU0PD1 (KZERO + 0x16000ull)

#define CPU0GDT (KZERO + 0x217000ull)


#define CPU0MACH (KZERO + 0x218000ull)


#define CPU0END (CPU0MACH + MACHSIZE)

#define MACHSIZE (2 * KSTACK)





#define BOOTLINE ((char *)CONFADDR)
#define BOOTLINELEN 64
#define BOOTARGS ((char *)(CONFADDR + BOOTLINELEN))
#define BOOTARGSLEN (0x6000 - 0x200 - BOOTLINELEN)




#define NULLSEG 0
#define KESEG 1
#define KDSEG 2
#define UE32SEG 3
#define UDSEG 4
#define UD64SEG 5
#define UESEG 6
#define TSSSEG 8

#define NGDT 10

#define SELGDT (0 << 2)
#define SELLDT (1 << 2)

#define SELECTOR(i,t,p) (((i) << 3) | (t) | (p))

#define NULLSEL SELECTOR(NULLSEG, SELGDT, 0)
#define KDSEL SELECTOR(KDSEG, SELGDT, 0)


#define KESEL SELECTOR(KESEG, SELGDT, 0)
#define UESEL SELECTOR(UESEG, SELGDT, 3)
#define UDSEL SELECTOR(UDSEG, SELGDT, 3)
#define UE32SEL SELECTOR(UE32SEG, SELGDT, 3)
#define UD64SEL SELECTOR(UD64SEG, SELGDT, 3)
#define TSSSEL SELECTOR(TSSSEG, SELGDT, 0)




#define SEGDATA (0x10 << 8)
#define SEGEXEC (0x18 << 8)
#define SEGTSS (0x9 << 8)
#define SEGCG (0x0C << 8)
#define SEGIG (0x0E << 8)
#define SEGTG (0x0F << 8)
#define SEGLDT (0x02 << 8)
#define SEGTYPE (0x1F << 8)

#define SEGP (1 << 15)
#define SEGPL(x) ((x) << 13)
#define SEGB (1 << 22)
#define SEGD (1 << 22)
#define SEGE (1 << 10)
#define SEGW (1 << 9)
#define SEGR (1 << 9)
#define SEGL (1 << 21)
#define SEGG (1 << 23)




#define PTEMAPMEM (1ull * MiB)
#define PTEPERTAB (PTEMAPMEM / BY2PG)
#define SEGMAPSIZE 65536
#define SSEGMAPSIZE 16
#define PPN(x) ((x) & ~(1ull << 63 | BY2PG - 1))




#define PTEVALID (1ull << 0)
#define PTEWT (1ull << 3)
#define PTEUNCACHED (1ull << 4)
#define PTECACHED (0ull << 4)
#define PTEWRITE (1ull << 1)
#define PTERONLY (0ull << 1)
#define PTEKERNEL (0ull << 2)
#define PTEUSER (1ull << 2)
#define PTEACCESSED (1ull << 5)
#define PTEDIRTY (1ull << 6)
#define PTESIZE (1ull << 7)
#define PTEGLOBAL (1ull << 8)
#define PTENOEXEC ((uvlong)m->havenx << 63)
# 193 "wasm/../include/mem.h"
#define PTSZ (4 * KiB)
#define PTSHIFT 9

#define PTLX(v,l) (((v) >> (((l) * PTSHIFT) + PGSHIFT)) & ((1 << PTSHIFT) - 1))
#define PGLSZ(l) (1ull << (((l) * PTSHIFT) + PGSHIFT))

#define getpgcolor(a) 0


#define PATWC 7

#define RMACH R15
#define RUSER R14
# 5 "wasm/../include/dat.h" 2
# 1 "wasm/../include/portlib.h" 1

#define _PORTLIB_H_ 


# 1 "wasm/../include/u.h" 1


#define _U_H_ 




#define static_assert _Static_assert







typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long uvlong;
typedef long long vlong;

typedef unsigned long usize;
typedef long ssize;
typedef unsigned long uintptr;
typedef long intptr;


typedef unsigned char u8int;
typedef unsigned short u16int;
typedef unsigned int u32int;
typedef unsigned long long u64int;
typedef signed char s8int;
typedef signed short s16int;
typedef signed int s32int;
typedef signed long long s64int;

typedef u32int Rune;

typedef struct {
  u8int data[16];
} uuid_t;

#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define offsetof(s,m) (ulong)(&(((s *)0)->m))

#define nil ((void *)0)


#define USED(...) if (__VA_ARGS__) { }




_Static_assert(sizeof(ulong) == sizeof(void *), "ulong must match pointer size");
_Static_assert(sizeof(uintptr) == sizeof(void *),
              "uintptr must match pointer size");
_Static_assert(sizeof(usize) == sizeof(void *), "usize must match pointer size");
_Static_assert(sizeof(ssize) == sizeof(void *), "ssize must match pointer size");
# 6 "wasm/../include/portlib.h" 2
# 1 "wasm/../include/acsl_bounds.h" 1
# 9 "wasm/../include/acsl_bounds.h"
#define _ACSL_BOUNDS_H 




#define ACSL_MAXSTR 4096
#define ACSL_MAXBUF 8192
#define ACSL_MAXPATH 1024
#define ACSL_MAXNAME 256




#define ACSL_MAX_FMT_ARGS 32
#define ACSL_MAX_FMT_LEN 512




#define ACSL_MAX_INT32 2147483647
#define ACSL_MIN_INT32 (-2147483647 - 1)
#define ACSL_MAX_UINT32 4294967295U
#define ACSL_MAX_INT64 9223372036854775807LL
#define ACSL_MIN_INT64 (-9223372036854775807LL - 1)
#define ACSL_MAX_UINT64 18446744073709551615ULL




#define ACSL_MAX_ALLOC (1ULL << 40)
#define ACSL_PAGE_SIZE 4096
# 7 "wasm/../include/portlib.h" 2





# 1 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdarg.h" 1 3 4
# 31 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdarg.h" 3 4
#define _STDARG_H 
#define _ANSI_STDARG_H_ 

#undef __need___va_list




#define __GNUC_VA_LIST 

# 40 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdarg.h" 3 4
typedef __builtin_va_list __gnuc_va_list;
# 50 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdarg.h" 3 4
#define va_start(v,l) __builtin_va_start(v,l)

#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)


#define va_copy(d,s) __builtin_va_copy(d,s)

#define __va_copy(d,s) __builtin_va_copy(d,s)
# 103 "/usr/lib/gcc/x86_64-linux-gnu/13/include/stdarg.h" 3 4
typedef __gnuc_va_list va_list;





#define _VA_LIST_ 


#define _VA_LIST 


#define _VA_LIST_DEFINED 


#define _VA_LIST_T_H 


#define __va_list__ 
# 13 "wasm/../include/portlib.h" 2


# 14 "wasm/../include/portlib.h"
typedef unsigned int Rune;

#define nelem(x) (sizeof(x) / sizeof((x)[0]))
#define offsetof(s,m) (ulong)(&(((s *)0)->m))
#define assert(x) if (x) { } else { print("ASSERT FAILED: %s:%d %s\n", __FILE__, __LINE__, #x); }
# 29 "wasm/../include/portlib.h"
extern void *memccpy(void *, const void *, int, usize);




extern void *memset(void *s, int c, usize n);
extern int memcmp(const void *, const void *, usize);




extern void *memmove(void *dst, const void *src, usize n);
extern void *memchr(const void *, int, usize);




extern char *strcat(char *, char *);
extern char *strchr(char *, int);
extern char *strrchr(char *, int);





extern int strcmp(char *s1, char *s2);
extern char *strcpy(char *, char *);
extern char *strecpy(char *, char *, char *);
extern char *strncat(char *, char *, long);
extern char *strncpy(char *, char *, long);
extern int strncmp(char *, char *, long);





extern long strlen(char *s);
extern char *strstr(char *, char *);
extern int atoi(char *);
extern int fullrune(char *, int);
extern int cistrcmp(char *, char *);
extern int cistrncmp(char *, char *, int);



enum {
  UTFmax = 4,
  Runesync = 0x80,
  Runeself = 0x80,
  Runeerror = 0xFFFD,
  Runemax = 0x10FFFF,
};






extern int runetochar(char *, Rune *);
extern int chartorune(Rune *, char *);
extern char *utfecpy(char *s1, char *es1, char *s2);
extern char *utfrune(char *, long);
extern int utflen(char *);
extern int utfnlen(char *, long);
extern int runelen(long);




extern int rand(void);
extern int nrand(int);
extern long lrand(void);
extern long lnrand(long);

extern int abs(int);





#define _FMT_TYPEDEF_ 
typedef struct Fmt Fmt;
struct Fmt {
  uchar runes;
  void *start;
  void *to;
  void *stop;
  int (*flush)(Fmt *);
  void *farg;
  int nfmt;
  va_list args;
  int r;
  int width;
  int prec;
  ulong flags;
};

typedef int (*Fmts)(Fmt *);

extern int print(char *, ...);
extern char *seprint(char *, char *, char *, ...);
extern char *vseprint(char *, char *, char *, va_list);
extern int snprint(char *, int, char *, ...);
extern int vsnprint(char *, int, char *, va_list);
extern int sprint(char *, char *, ...);


#pragma varargck argpos fmtprint 2
#pragma varargck argpos print 1
#pragma varargck argpos seprint 3
#pragma varargck argpos snprint 3
#pragma varargck argpos sprint 2

#pragma varargck type "llb" vlong
#pragma varargck type "lld" vlong
#pragma varargck type "llx" vlong
#pragma varargck type "llb" uvlong
#pragma varargck type "lld" uvlong
#pragma varargck type "llx" uvlong
#pragma varargck type "lb" long
#pragma varargck type "ld" long
#pragma varargck type "lx" long
#pragma varargck type "lb" ulong
#pragma varargck type "ld" ulong
#pragma varargck type "lx" ulong
#pragma varargck type "zd" intptr
#pragma varargck type "zo" intptr
#pragma varargck type "zx" intptr
#pragma varargck type "zb" intptr
#pragma varargck type "zd" uintptr
#pragma varargck type "zo" uintptr
#pragma varargck type "zx" uintptr
#pragma varargck type "zb" uintptr
#pragma varargck type "b" int
#pragma varargck type "d" int
#pragma varargck type "x" int
#pragma varargck type "c" int
#pragma varargck type "C" int
#pragma varargck type "b" uint
#pragma varargck type "d" uint
#pragma varargck type "x" uint
#pragma varargck type "c" uint
#pragma varargck type "C" uint
#pragma varargck type "s" char *
#pragma varargck type "q" char *
#pragma varargck type "S" Rune *
#pragma varargck type "%" void
#pragma varargck type "p" uintptr
#pragma varargck type "p" void *
#pragma varargck flag ','


extern int fmtstrinit(Fmt *);
extern int fmtinstall(int, int (*)(Fmt *));
extern void quotefmtinstall(void);
extern int fmtprint(Fmt *, char *, ...);
extern int fmtstrcpy(Fmt *, char *);
extern char *fmtstrflush(Fmt *);




extern char *cleanname(char *);
extern uintptr getcallerpc(void *);

extern long strtol(char *, char **, int);
extern ulong strtoul(char *, char **, int);
extern vlong strtoll(char *, char **, int);
extern uvlong strtoull(char *, char **, int);
extern char etext[];
extern char edata[];
extern char end[];
extern int getfields(char *, char **, int, int, char *);
extern int tokenize(char *, char **, int);
extern int dec64(uchar *, int, char *, int);
extern int dec16(uchar *, int, char *, int);
extern int encodefmt(Fmt *);
extern void qsort(void *, usize, usize, int (*)(void *, void *));




#define MORDER 0x0003
#define MREPL 0x0000
#define MBEFORE 0x0001
#define MAFTER 0x0002
#define MCREATE 0x0004
#define MCACHE 0x0010
#define MMASK 0x0017

#define OREAD 0
#define OWRITE 1
#define ORDWR 2
#define OEXEC 3
#define OTRUNC 16
#define OCEXEC 32
#define ORCLOSE 64
#define OEXCL 0x1000

#define NCONT 0
#define NDFLT 1
#define NSAVE 2
#define NRSTR 3

typedef struct Qid Qid;
typedef struct Dir Dir;
typedef struct OWaitmsg OWaitmsg;
typedef struct Waitmsg Waitmsg;

#define ERRMAX 128
#define KNAMELEN 28


#define QTDIR 0x80
#define QTAPPEND 0x40
#define QTEXCL 0x20
#define QTMOUNT 0x10
#define QTAUTH 0x08
#define QTFILE 0x00


#define DMDIR 0x80000000
#define DMAPPEND 0x40000000
#define DMEXCL 0x20000000
#define DMMOUNT 0x10000000
#define DMREAD 0x4
#define DMWRITE 0x2
#define DMEXEC 0x1


#define _QID_TYPEDEF_ 
struct Qid {
  uvlong path;
  ulong vers;
  uchar type;
};



#define _DIR_TYPEDEF_ 
struct Dir {

  ushort type;
  uint dev;

  Qid qid;
  ulong mode;
  ulong atime;
  ulong mtime;
  vlong length;
  char *name;
  char *uid;
  char *gid;
  char *muid;
};


struct OWaitmsg {
  char pid[12];
  char time[3 * 12];
  char msg[64];
};


#define _WAITMSG_TYPEDEF_ 
struct Waitmsg {
  int pid;
  ulong time[3];
  char msg[128];
};
# 6 "wasm/../include/dat.h" 2
# 1 "wasm/../include/types_fwd.h" 1
# 9 "wasm/../include/types_fwd.h"
#define _TYPES_FWD_H_ 



#define _PROC_DEFINED 
typedef struct Proc Proc;




#define _MACH_DEFINED 
typedef struct Mach Mach;




#define _LABEL_DEFINED 
typedef struct Label Label;




#define _LOCK_DEFINED 
typedef struct Lock Lock;




#define _CHAN_DEFINED 
typedef struct Chan Chan;




#define _FCALL_DEFINED 
typedef struct Fcall Fcall;




#define _UREG_DEFINED 
typedef struct Ureg Ureg;




#define _RENDEZ_DEFINED 
typedef struct Rendez Rendez;
# 7 "wasm/../include/dat.h" 2


typedef struct Conf Conf;
typedef struct Confmem Confmem;
typedef struct FPssestate FPssestate;
typedef struct FPavxstate FPavxstate;
typedef struct FPalloc FPalloc;
typedef struct FPsave FPsave;
typedef struct PFPU PFPU;
typedef struct ISAConf ISAConf;
typedef struct Label Label;
typedef struct MMU MMU;
typedef struct PCArch PCArch;
typedef struct Pcidev Pcidev;
typedef struct PCMmap PCMmap;
typedef struct PCMslot PCMslot;
typedef struct Page Page;
typedef struct PMMU PMMU;
typedef struct Segdesc Segdesc;
typedef vlong Tval;
typedef struct Vctl Vctl;

#pragma incomplete Pcidev
#pragma incomplete Ureg

#define MAXSYSARG 5




#define AOUT_MAGIC (S_MAGIC)

# 1 "wasm/../include/lock.h" 1

#define _LOCK_H_ 




struct Lock {
  ulong key;
  ulong sr;
  uintptr pc;
  Proc *p;
  Mach *m;
  ushort isilock;
  long lockcycles;
} __attribute__((aligned(64)));
# 40 "wasm/../include/dat.h" 2



struct Label {
  uintptr sp;
  uintptr pc;
  uintptr rbp;
  uintptr rbx;
  uintptr r12;
  uintptr r13;
  uintptr r14;
  uintptr r15;
};

struct FPssestate {
  u16int fcw;
  u16int fsw;
  u8int ftw;
  u8int zero;
  u16int fop;
  u64int rip;
  u64int rdp;
  u32int mxcsr;
  u32int mxcsrmask;
  uchar st[128];
  uchar xmm[256];
  uchar ign[96];
} __attribute__((aligned(64)));

struct FPavxstate {
  FPssestate sse_state;
  uchar header[64];
  uchar ymm[256];
};

struct FPsave {
  FPavxstate avx_state;
};

struct FPalloc {
  FPsave fp_save;

  FPalloc *link;
};

enum {
  FPinit,
  FPactive,
  FPprotected,
  FPinactive,

  FPnotify = 0x100,
};

#define KFPSTATE 

struct PFPU {
  int fpstate;
  int kfpstate;
  FPalloc *fpsave;
  FPalloc *kfpsave;
};

struct Confmem {
  uintptr base;
  ulong npage;
  uintptr kbase;
  uintptr klimit;
};

struct Conf {
  ulong nmach;
  ulong nproc;
  ulong monitor;
  ulong npage;
  ulong upages;
  ulong nimage;
  ulong nswap;
  int nswppo;
  ulong copymode;
  ulong ialloc;
  ulong pipeqsize;
  int nuart;
  Confmem mem[64];
};

struct Segdesc {
  u32int d0;
  u32int d1;
};




struct MMU {
  MMU *next;
  uintptr *page;
  void *alloc;
  int index;
  int level;
} __attribute__((aligned(64)));




#define NCOLOR 1
struct PMMU {
  MMU *mmuhead;
  MMU *mmutail;
  MMU *kmaphead;
  MMU *kmaptail;
  ulong kmapcount;
  ulong kmapindex;
  ulong mmucount;

  u64int dr[8];
  void *vmx;
};

#define inittxtflush(p) 
#define settxtflush(p,c) 

# 1 "wasm/../include/portdat.h" 1

#define _PORTDAT_H_ 


# 1 "wasm/../include/uuid.h" 1

#define _UUID_H_ 




void uuid_clear(uuid_t *u);
int uuid_compare(const uuid_t *a, const uuid_t *b);
void uuid_copy(uuid_t *dst, const uuid_t *src);
int uuid_parse(const char *in, uuid_t *uu);
void uuid_unparse(const uuid_t *uu, char *out);
int uuid_is_null(const uuid_t *uu);


void uuid_new_v8(uuid_t *u);

void uuid_pack_v8(uuid_t *u, unsigned long long data_a, unsigned short data_b,
                  unsigned long long data_c);


typedef struct {
  unsigned int token;
  unsigned int generation;
  unsigned short index;
} pebble_uuid_data_t;

void uuid_pack_pebble(uuid_t *u, unsigned int token, unsigned int generation,
                      unsigned short index);
int uuid_unpack_pebble(const uuid_t *u, unsigned int *token,
                       unsigned int *generation, unsigned short *index);
# 43 "wasm/../include/uuid.h"
void uuid_pack_capability(uuid_t *u, const unsigned char *pa_hash,
                          unsigned short epoch, unsigned char type,
                          unsigned char perms);

int uuid_unpack_capability(const uuid_t *u, unsigned short *epoch,
                           unsigned char *type, unsigned char *perms);


void uuid_get_pa_hash_bits(const uuid_t *u, unsigned char *pa_hash_out);


void uuid_pack_pid_lux9(uuid_t *u, const uuid_t *parent_uuid,
                        const u8int *namespace_cid, const u8int *code_hash);


int uuid_verify_pid_lux9(const uuid_t *pid2, const uuid_t *parent_uuid,
                         const u8int *namespace_cid, const u8int *code_hash);
# 6 "wasm/../include/portdat.h" 2

typedef struct Alarms Alarms;
typedef struct Block Block;
typedef struct Bpool Bpool;
typedef struct Cmdbuf Cmdbuf;
typedef struct Cmdtab Cmdtab;
typedef struct Confmem Confmem;
typedef struct Dev Dev;
typedef struct Dirtab Dirtab;
typedef struct Edf Edf;
typedef struct Egrp Egrp;
typedef struct Evalue Evalue;
typedef struct Fgrp Fgrp;
typedef struct DevConf DevConf;
typedef struct Image Image;
typedef struct Log Log;
typedef struct Logflag Logflag;
typedef struct Mntcache Mntcache;
typedef struct Mount Mount;
typedef struct Mntrah Mntrah;
typedef struct Mntrpc Mntrpc;
typedef struct Mntproc Mntproc;
typedef struct Mnt Mnt;
typedef struct Mhead Mhead;
typedef struct Note Note;
typedef struct Page Page;
typedef struct Path Path;
typedef struct Palloc Palloc;
typedef struct Perf Perf;
typedef struct PhysUart PhysUart;
typedef struct Pgrp Pgrp;
typedef struct Physseg Physseg;
typedef struct Pte Pte;
typedef struct PMach PMach;
typedef struct QLock QLock;
typedef struct Queue Queue;





typedef struct Ref Ref;
typedef struct Rendezq Rendezq;
typedef struct Rgrp Rgrp;
typedef struct RWLock RWLock;
typedef struct Sargs Sargs;
typedef struct Schedq Schedq;
typedef struct Segment Segment;
typedef struct Segio Segio;
typedef struct Sema Sema;
typedef struct Timer Timer;
typedef struct Timers Timers;
typedef struct Uart Uart;
typedef struct Waitq Waitq;
typedef struct Walkqid Walkqid;
typedef struct Watchpt Watchpt;
typedef struct Watchdog Watchdog;
typedef int Devgen(Chan *, char *, Dirtab *, int, int, Dir *);
# 77 "wasm/../include/portdat.h"
#pragma incomplete DevConf
#pragma incomplete Edf
#pragma incomplete Mntcache
#pragma incomplete Mntrpc

# 1 "wasm/../include/lock_dag.h" 1
# 10 "wasm/../include/lock_dag.h"
       

struct Proc;

enum {
 LOCKDAG_MAX_NODES = 128,
 LOCKDAG_STACK_DEPTH = 32,
};

typedef struct LockDagNode LockDagNode;

struct LockDagNode {
 const char *name;
 int id;
};

#define LOCKDAG_NODE(label) { .name = (label), .id = -1 }

struct LockDagEntry {
 LockDagNode *node;
 uintptr key;
};

struct LockDagContext {
 struct LockDagEntry stack[LOCKDAG_STACK_DEPTH];
 int depth;
 int overflow;
};

void lockdag_init(void);
int lockdag_register_node(LockDagNode *node);
int lockdag_allow_edge(LockDagNode *from, LockDagNode *to);
void lockdag_record_acquire(Proc *p, LockDagNode *node, uintptr key);
void lockdag_record_release(Proc *p, LockDagNode *node, uintptr key);
# 83 "wasm/../include/portdat.h" 2

# 1 "wasm/../include/pebble.h" 1
       
# 15 "wasm/../include/pebble.h"
#define PEBBLE_DEFAULT_BUDGET 0
#define PEBBLE_BOOT_BUDGET (256 * 1024 * 1024)
#define PEBBLE_INIT_BUDGET (64 * 1024 * 1024)

#define PEBBLE_MAX_TOKENS 4096
#define PEBBLE_DEBUG 1


#define PEBBLE_BYTES_PER_TOKEN 8






enum PebbleColor {
  PEBBLE_COLOR_COLORLESS = 0,
  PEBBLE_COLOR_WHITE = 1,
  PEBBLE_COLOR_BLACK = 2,
  PEBBLE_COLOR_RED = 3,
  PEBBLE_COLOR_BLUE = 4,
};


#define PEBBLE_PROC_COST (1024 * 1024 / PEBBLE_BYTES_PER_TOKEN)

#define PEBBLE_PIPE_COST (64 * 1024 / PEBBLE_BYTES_PER_TOKEN)

#define PEBBLE_MOUNT_COST (4 * 1024 / PEBBLE_BYTES_PER_TOKEN)

#define PEBBLE_FD_COST (1 * 1024 / PEBBLE_BYTES_PER_TOKEN)




extern int pebble_enabled;
extern int pebble_debug;







extern Lock pebble_bank_lock;
extern ulong pebble_global_colorless_bank;
extern ulong pebble_total_system_tokens;


#define PEBBLE_E_PERM "permission denied"
#define PEBBLE_E_AGAIN "resource temporarily unavailable"
#define PEBBLE_E_NOMEM "out of memory"
#define PEBBLE_E_BADARG "bad argument"
#define PEBBLE_E_BUSY "resource busy"


#define PEBBLE_CAP_BLACK (1 << 0)
#define PEBBLE_CAP_ACTIVE (1 << 1)
#define PEBBLE_CAP_DEVICE (1 << 2)
#define PEBBLE_CAP_IOPORT (1 << 3)
#define PEBBLE_CAP_NET (1 << 4)
#define PEBBLE_CAP_IRQ (1 << 5)
#define PEBBLE_CAP_DMA (1 << 6)
#define PEBBLE_CAP_PCI (1 << 7)
#define PEBBLE_CAP_FS (1 << 8)
#define PEBBLE_CAP_ADMIN (1 << 9)




#define has_capability(p,cap) ((p)->capabilities & (cap))





#define PEBBLE_WAVE_MASK 0x7ULL
#define PEBBLE_PTR_ADDR(p) ((void *)((uintptr)(p) & ~PEBBLE_WAVE_MASK))
#define PEBBLE_PTR_WAVE(p) ((int)((uintptr)(p) & PEBBLE_WAVE_MASK))


#define PEBBLE_WAVE_0 0
#define PEBBLE_WAVE_1 1
#define PEBBLE_WAVE_2 2
#define PEBBLE_WAVE_3 3
#define PEBBLE_WAVE_4 4
#define PEBBLE_WAVE_5 5
#define PEBBLE_WAVE_6 6
#define PEBBLE_WAVE_7 7


#define PEBBLE_PROJECT(p,wave) ((void *)((uintptr)PEBBLE_PTR_ADDR(p) | ((wave) & PEBBLE_WAVE_MASK)))

#define PEBBLE_TUNED(p,wave) (PEBBLE_PTR_WAVE(p) == (wave))

# 1 "wasm/../include/blind_ledger.h" 1
# 22 "wasm/../include/blind_ledger.h"
#define _BLIND_LEDGER_H_ 

# 1 "wasm/../include/../include/rbtree.h" 1
# 22 "wasm/../include/../include/rbtree.h"
#define RBTREE_H 



typedef unsigned long uintptr;
# 36 "wasm/../include/../include/rbtree.h"
struct rb_node {
  uintptr __rb_parent_color;
  struct rb_node *rb_right;
  struct rb_node *rb_left;
};




struct rb_root {
  struct rb_node *rb_node;
};

#define RB_ROOT (struct rb_root) { nil }



#define RB_RED 0
#define RB_BLACK 1

#define __rb_parent(pc) ((struct rb_node *)(pc & ~3))
#define __rb_color(pc) ((pc) & 1)
#define __rb_is_black(pc) __rb_color(pc)
#define __rb_is_red(pc) (!__rb_color(pc))
#define rb_color(rb) __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb) __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb) __rb_is_black((rb)->__rb_parent_color)

#define rb_parent(r) ((struct rb_node *)((r)->__rb_parent_color & ~3))

#define rb_entry(ptr,type,member) ((type *)((char *)(ptr) - (uintptr)(&((type *)0)->member)))


#define RB_EMPTY_ROOT(root) ((root)->rb_node == nil)
#define RB_EMPTY_NODE(node) ((node)->__rb_parent_color == (uintptr)(node))
#define RB_CLEAR_NODE(node) ((node)->__rb_parent_color = (uintptr)(node))


void rb_insert_color(struct rb_node *node, struct rb_root *root);
void rb_erase(struct rb_node *node, struct rb_root *root);


static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link) {
  node->__rb_parent_color = (uintptr)parent;
  node->rb_left = node->rb_right = ((void *)0);

  *rb_link = node;
}


static inline void rb_set_parent_color(struct rb_node *rb, struct rb_node *p,
                                       int color) {
  rb->__rb_parent_color = (uintptr)p | (uintptr)color;
}

static inline void rb_set_parent(struct rb_node *rb, struct rb_node *p) {
  rb->__rb_parent_color = (((rb)->__rb_parent_color) & 1) | (uintptr)p;
}

static inline void rb_set_black(struct rb_node *rb) {
  rb->__rb_parent_color |= 1;
}

static inline void rb_set_red(struct rb_node *rb) {
  rb->__rb_parent_color &= ~1UL;
}


struct rb_node *rb_first(const struct rb_root *root);
struct rb_node *rb_last(const struct rb_root *root);
struct rb_node *rb_next(const struct rb_node *node);
struct rb_node *rb_prev(const struct rb_node *node);


void rb_replace_node(struct rb_node *victim, struct rb_node *new_node,
                     struct rb_root *root);


typedef void (*rb_augment_f)(struct rb_node *node, void *data);

void rb_insert_augmented(struct rb_node *node, struct rb_root *root,
                         rb_augment_f augment_rotate, void *data);
void rb_erase_augmented(struct rb_node *node, struct rb_root *root,
                        rb_augment_f augment_rotate, void *data);
# 25 "wasm/../include/blind_ledger.h" 2




#define BLIND_LEDGER_SECRET_SIZE 32
#define BLIND_LEDGER_TOKEN_UNIT 8
#define BLIND_LEDGER_CAP_SIZE 32
#define BLIND_LEDGER_SECRET_SIZE 32
# 45 "wasm/../include/blind_ledger.h"
typedef struct UserCapability {
  uuid_t uuid;
  u8int
      hash[32];
  u64int size;
  u32int type;
  u32int perms;
} UserCapability;


enum {
  CAP_TYPE_MEMORY = 1,
  CAP_TYPE_CHANNEL = 2,
  CAP_TYPE_DEVICE = 3,
  CAP_TYPE_IPC = 4,
  CAP_TYPE_SPAWN = 5,
};


enum {
  CAP_PERM_READ = 1 << 0,
  CAP_PERM_WRITE = 1 << 1,
  CAP_PERM_EXEC = 1 << 2,
  CAP_PERM_TRANSFER = 1 << 3,
  CAP_PERM_GRANT = 1 << 4,
};


typedef enum BlindLedgerState {
  BLIND_LEDGER_STATE_INACTIVE = 0,
  BLIND_LEDGER_STATE_ACTIVE = 1,
  BLIND_LEDGER_STATE_BURNED = 2,
  BLIND_LEDGER_STATE_COW_RED = 3,
  BLIND_LEDGER_STATE_COW_BLUE = 4,
} BlindLedgerState;


typedef u8int BlindLedgerHash[32];







typedef struct BlindLedgerEntry {
  UserCapability capability;
  uintptr physical_address;
  Proc *owner;
  u8int secret[32];

  u64int epoch;
  u64int span_len;
  u32int permissions;
  BlindLedgerState state;
  BlindLedgerHash leaf_hash;
  BlindLedgerHash process_hash;



  BlindLedgerHash parent_hash;
  BlindLedgerHash derivation_sig;
} BlindLedgerEntry;


typedef enum BlindLedgerError {
  BLIND_LEDGER_OK = 0,
  BLIND_LEDGER_EINVAL = 1,
  BLIND_LEDGER_ENOMEM = 2,
  BLIND_LEDGER_EPERM = 3,
  BLIND_LEDGER_ENOTFOUND = 4,
  BLIND_LEDGER_EEXPIRED = 5,
  BLIND_LEDGER_EFAULT = 6,
  BLIND_LEDGER_EBUSY = 7,
} BlindLedgerError;


void blind_ledger_init(void);
BlindLedgerError ledger_mint(UserCapability *out_cap, uintptr pa, ulong len,
                             Proc *owner, u32int permissions,
                             const u8int *vault_secret);
BlindLedgerError ledger_verify(const UserCapability *cap,
                               BlindLedgerEntry *out_entry);
BlindLedgerError ledger_verify_by_uuid(const uuid_t *uuid,
                                       BlindLedgerEntry *out_entry);
BlindLedgerError ledger_transfer(const UserCapability *cap, Proc *from_owner,
                                 Proc *to_owner);
BlindLedgerError ledger_burn(const UserCapability *cap, Proc *owner);
BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
void blind_ledger_update_merkle_root(void);
const u8int *blind_ledger_get_merkle_root(void);


u64int ledger_get_current_epoch(void);
void ledger_advance_epoch(void);


BlindLedgerError ledger_generate_secret(u8int *secret_out);
BlindLedgerError ledger_destroy_secret(const u8int *secret);


typedef struct LedgerRollbackToken {
  UserCapability
      original_capability;
  Proc *original_owner;
  BlindLedgerHash
      original_process_hash;
  u64int is_valid;
} LedgerRollbackToken;

#define ROLLBACK_TOKEN_MAGIC 0x524F4C4C4241434BULL

BlindLedgerError
ledger_transfer_reversible(const UserCapability *cap, Proc *from_owner,
                           Proc *to_owner, LedgerRollbackToken *rollback_token);

typedef struct BlindLedgerStats {
  u64int active_entries;
  u64int burned_entries;
  u64int total_memory_tracked;
  u64int tree_depth;
  u64int epoch;
} BlindLedgerStats;

BlindLedgerError blind_ledger_get_stats(BlindLedgerStats *stats);


BlindLedgerError blind_ledger_attest_root(u8int *out_signature,
                                          u32int *out_len);





#define MAX_DERIVATION_DEPTH 16




typedef struct DerivationStep {
  BlindLedgerHash parent_hash;
  BlindLedgerHash derivation_sig;
  u32int constraints;
} DerivationStep;







typedef struct DerivationProof {
  BlindLedgerHash target_hash;
  u32int chain_length;
  DerivationStep chain[16];
} DerivationProof;







BlindLedgerError ledger_derive(const UserCapability *parent_cap, Proc *owner,
                               u32int child_constraints,
                               UserCapability *out_child_cap);






BlindLedgerError ledger_get_derivation_proof(const UserCapability *cap,
                                             Proc *owner,
                                             DerivationProof *out_proof);







BlindLedgerError ledger_verify_derivation_proof(const DerivationProof *proof);





BlindLedgerError ledger_lookup_by_pa_and_owner(uintptr pa, Proc *owner,
                                               UserCapability *out_cap,
                                               BlindLedgerEntry *out_entry);
# 111 "wasm/../include/pebble.h" 2
# 1 "wasm/../include/borrowchecker.h" 1





       







enum BorrowState {
  BORROW_FREE = 0,
  BORROW_EXCLUSIVE,
  BORROW_SHARED_OWNED,
  BORROW_MUT_LENT,
};


struct IdentKey {
  u64int gen;
  u64int nonce;
};


enum BorrowSystemOwner {
  OWNER_BOOTLOADER = 0,
  OWNER_KERNEL,
  OWNER_TRAMPOLINE,
};


enum AllocSource {
  ALLOC_BOOTSTRAP,
  ALLOC_XALLOC,
};


struct MemoryRange {
  uintptr start;
  uintptr end;
  enum BorrowSystemOwner owner;
  struct MemoryRange *next;
};


struct BorrowOwner {

  uintptr key;
  struct IdentKey key_cap;


  Proc *owner;
  enum BorrowState state;
  enum BorrowSystemOwner system_owner;
  int is_system_owned;


  int shared_count;
  Proc *mut_borrower;
  struct SharedBorrower *shared_list;


  uvlong acquired_ns;
  uvlong borrow_deadline_ns;


  ulong borrow_count;


  enum AllocSource alloc_source;
  struct BorrowOwner *next;
};


struct SharedBorrower {
  Proc *proc;
  enum AllocSource alloc_source;
  struct SharedBorrower *next;
};


struct BorrowBucket {
  struct BorrowOwner *head;
};


struct BorrowPool {
  Lock lock;
  struct BorrowBucket *owners;
  ulong nbuckets;
  ulong nowners;
  ulong nshared;
  ulong nmut;
  u8int *bloom;
  ulong bloom_bits;
  u32int bloom_hashes;
};


enum MemoryCoordinationState {
  MEMORY_BOOTLOADER = 0,
  MEMORY_COORDINATED,
  MEMORY_KERNEL_ACTIVE,
};


struct MemoryCoordination {
  enum MemoryCoordinationState state;
  enum BorrowSystemOwner current_owner;
  int coordination_enabled;
};


enum BorrowError {
  BORROW_OK = 0,
  BORROW_EALREADY,
  BORROW_ENOTOWNER,
  BORROW_EBORROWED,
  BORROW_EMUTBORROW,
  BORROW_ESHAREDBORROW,
  BORROW_ENOTBORROWED,
  BORROW_EINVAL,
  BORROW_ENOMEM,
  BORROW_ENOTFOUND,
};


extern struct BorrowPool borrowpool;


void borrowinit(void);
enum BorrowError borrow_acquire(Proc *p, uintptr key);
enum BorrowError borrow_release(Proc *p, uintptr key);
enum BorrowError borrow_transfer(Proc *from, Proc *to, uintptr key);


enum BorrowError borrow_borrow_shared(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_borrow_mut(Proc *owner, Proc *borrower, uintptr key);
enum BorrowError borrow_return_shared(Proc *borrower, uintptr key);
enum BorrowError borrow_return_mut(Proc *borrower, uintptr key);


enum BorrowError borrow_acquire_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_release_system(uintptr key,
                                       enum BorrowSystemOwner owner);
enum BorrowError borrow_transfer_system(enum BorrowSystemOwner from,
                                        enum BorrowSystemOwner to, uintptr key);
enum BorrowSystemOwner borrow_get_system_owner(uintptr key);
int borrow_is_owned_by_system(uintptr key, enum BorrowSystemOwner owner);


enum BorrowError borrow_acquire_range_phys(uintptr start_pa, usize size,
                                           enum BorrowSystemOwner owner);
int borrow_range_owned_by_system(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner owner);
int borrow_can_access_range_phys(uintptr start_pa, usize size,
                                 enum BorrowSystemOwner requester);


int borrow_is_owned(uintptr key);
Proc *borrow_get_owner(uintptr key);
int borrow_get_owner_snapshot(uintptr key, struct BorrowOwner *out);
enum BorrowState borrow_get_state(uintptr key);
int borrow_can_borrow_shared(uintptr key);
int borrow_can_borrow_mut(uintptr key);


void borrow_cleanup_process(Proc *p);


void memory_range_init(void);
void memory_range_add(uintptr start, uintptr end, enum BorrowSystemOwner owner);
void memory_range_add_discovered(uintptr start, uintptr end,
                                 enum BorrowSystemOwner owner);
void memory_range_remove(uintptr start, uintptr end);
void memory_range_dump(void);
int memory_range_capacity(void);
enum BorrowSystemOwner memory_range_get_owner(uintptr addr);
int memory_range_check_access(uintptr addr, enum BorrowSystemOwner requester);


void boot_memory_coordination_init(void);
void transfer_bootloader_to_kernel(void);
void establish_memory_ownership_zones(void);
void establish_memory_ownership_zones_dynamic(void);
int validate_memory_coordination_ready(void);
int memory_system_ready_before_cr3(void);
int post_cr3_memory_system_operational(void);


void borrow_stats(void);
void borrow_dump_resource(uintptr key);


ulong borrow_hash(uintptr key);
# 112 "wasm/../include/pebble.h" 2



typedef struct PebbleWhite {
  u32int token;
  u32int generation;
  void *data_ptr;
  ulong size;
} PebbleWhite;


typedef struct PebbleBlue {
  void *blue_data;
  ulong blue_size;
  ulong flags;
  struct PebbleBlue *next;
} PebbleBlue;


typedef struct PebbleRed {
  void *red_data;
  ulong red_size;
  ulong flags;
  struct PebbleRed *next;
} PebbleRed;

typedef struct PebbleBlack {
  UserCapability capability;
  void *
      physical_addr;
  uintptr user_vaddr;
  ulong size;
  ulong flags;
  struct PebbleBlack *next;
} PebbleBlack;


typedef struct PebbleState {
  ulong colorless_bank;
  ulong black_inuse;
  ulong blue_inuse;
  ulong red_inuse;
  ulong white_verified;
  ulong white_pending;
  ulong red_count;
  ulong blue_count;
  ulong total_allocs;
  ulong total_frees;

  uintptr vbase;


  PebbleBlack *black_list;
  PebbleBlue *blue_list;
  PebbleRed *red_list;


  int in_syscall;
  ulong drop_budget;


  PebbleWhite whites[4096];
  uchar whites_active[4096];
  ulong white_generation;
  int white_head;
} PebbleState;
# 191 "wasm/../include/pebble.h"
typedef struct arena_branch {
  Lock lock;
  ulong local_colorless;
  ulong borrowed_from_proc;
  ulong max_tokens;
  ulong low_water;
  ulong high_water;
  ulong total_allocated;
  ulong total_freed;
  PebbleState *owner_ps;
} arena_branch_t;


void arena_branch_init(arena_branch_t *branch, PebbleState *ps,
                       ulong initial_budget);
int arena_branch_alloc(arena_branch_t *branch,
                       ulong size);
void arena_branch_free(arena_branch_t *branch,
                       ulong size);
int arena_branch_refill(arena_branch_t *branch);
void arena_branch_drain(arena_branch_t *branch);


extern Lock pebble_global_lock;


int pebble_black_alloc(PebbleWhite *white, void *buf, ulong size,
                       UserCapability *out_cap);
void *pebble_get_black_addr(const UserCapability *cap);
int pebble_black_free(const UserCapability *cap);
int pebble_white_verify(PebbleWhite *white_cap, void **black_cap);
int pebble_create_token_uuid(PebbleWhite *white, uuid_t *out_uuid);
int pebble_alloc_with_white(ulong size, UserCapability *out_cap,
                            void **out_addr);


PebbleBlue *pebble_blue_alloc(ulong size);
int pebble_blue_free(PebbleBlue *blue);
PebbleRed *pebble_red_alloc(ulong size);
int pebble_red_free(PebbleRed *red);
int pebble_red_snapshot(PebbleBlue *blue,
                        PebbleRed **out_red);


int pebble_red_copy(PebbleBlue *blue_obj,
                    PebbleRed **red_copy);
int pebble_blue_discard(PebbleBlue *blue_obj);


PebbleState *pebble_state(void);
int pebble_set_budget(ulong budget);
ulong pebble_get_budget(void);
int pebble_increase_budget(ulong size, u64int nonce);
void pebble_auto_verify(Proc *p, Ureg *ureg);

void pebble_red_blue_exit(void);
# 267 "wasm/../include/pebble.h"
int pebble_valid_white_token(PebbleState *ps, PebbleWhite *white);
# 282 "wasm/../include/pebble.h"
PebbleWhite *pebble_issue_white(PebbleState *ps, void *data, ulong size);







void pebble_return_white(PebbleState *ps, PebbleWhite *white);







PebbleBlack *pebble_lookup_black(PebbleState *ps, void *handle);





PebbleBlack *pebble_lookup_black_by_addr(PebbleState *ps, void *addr);
int pebble_blue_exists(PebbleState *ps, PebbleBlue *blue);
int pebble_has_matching_red(PebbleState *ps, PebbleBlue *blue);
PebbleRed *pebble_duplicate_blue(PebbleState *ps, PebbleBlue *blue);
void pebble_mark_red(PebbleState *ps, PebbleBlue *blue, PebbleRed *red);
void pebble_ensure_red_snapshots(PebbleState *ps);

void pebble_cleanup(struct Proc *p);
void pebble_selftest(void);
void pebble_sip_issue_test(void);



#define pebble_dprint(fmt,...) bprint("PEBBLE: " fmt "\n", ##__VA_ARGS__)





void pebbleinit(void);
void pebbleprocinit(Proc *p);
void *pebble_meta_alloc(ulong size);
void pebble_meta_free(void *v);


#define PEBBLE_TOKEN_MAGIC 0x50454242
#define PEBBLE_MEM_PER_TOKEN 8
#define PEBBLE_MIN_ALLOC 8
#define PEBBLE_MAX_ALLOC (1024 * 1024 * 1024)






#define POW_OP_ALLOC 1
#define POW_OP_SPAWN 2
#define POW_OP_NET_BIND 3
#define POW_OP_REALTIME 4
#define POW_OP_STACK_ALLOC 5
#define POW_OP_MSGORD 6

int pow_calculate_difficulty(int op_class, ulong magnitude);
int pow_verify(u64int nonce, u64int context, int required_diff);
void pow_gate_init(void);


#define ROUNDUP(n,sz) (((n) + ((sz) - 1)) & ~((sz) - 1))
# 85 "wasm/../include/portdat.h" 2
#pragma incomplete Queue
#pragma incomplete Timers

# 1 "../kernel/include/fcall.h" 1

#define _FCALL_H_ 

# 1 "../kernel/include/portlib.h" 1
# 5 "../kernel/include/fcall.h" 2
# 1 "../kernel/include/u.h" 1
# 6 "../kernel/include/fcall.h" 2


#pragma src "/sys/src/libc/9sys"
#pragma lib "libc.a"


#define VERSION9P "9P2000"

#define MAXWELEM 16

typedef struct Fcall {
  uchar type;
  u32int fid;
  ushort tag;
  union {
    struct {
      u32int msize;
      char *version;
    };
    struct {
      ushort oldtag;
    };
    struct {
      char *ename;
    };
    struct {
      Qid qid;
      u32int iounit;
    };
    struct {
      Qid aqid;
    };
    struct {
      u32int afid;
      char *uname;
      char *aname;
    };
    struct {
      u32int perm;
      char *name;
      uchar mode;
    };
    struct {
      u32int newfid;
      ushort nwname;
      char *wname[16];
    };
    struct {
      ushort nwqid;
      Qid wqid[16];
    };
    struct {
      vlong offset;
      u32int count;
      char *data;
    };
    struct {
      ushort nstat;
      uchar *stat;
    };
    struct {
      u32int scallnr;
      u32int sflags;
      uchar *sdata;
      u32int scount;
      u64int retval;
    };

    struct {
      u32int flags;
      u32int pid;
    };
    struct {
      char *path;
      char **argv;
      u32int argc;
      char
          *args[16];
    };
    struct {
      u64int addr;
    };
    struct {
      char *oldpath;
      u32int fd;
    };
    struct {
      u32int fid0;
      u32int fid1;
    };
    struct {
      int whence;
    };
    struct {
      u64int handler;
    };
  };
} Fcall;

#define GBIT8(p) (((uchar *)(p))[0])
#define GBIT16(p) (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8))
#define GBIT32(p) (((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24))


#define GBIT64(p) ((u32int)(((uchar *)(p))[0] | (((uchar *)(p))[1] << 8) | (((uchar *)(p))[2] << 16) | (((uchar *)(p))[3] << 24)) | ((uvlong)(((uchar *)(p))[4] | (((uchar *)(p))[5] << 8) | (((uchar *)(p))[6] << 16) | (((uchar *)(p))[7] << 24)) << 32))






#define PBIT8(p,v) do { ((uchar *)(p))[0] = (uchar)(v); } while (0)



#define PBIT16(p,v) do { ((uchar *)(p))[0] = (uchar)(v); ((uchar *)(p))[1] = (uchar)((v) >> 8); } while (0)




#define PBIT32(p,v) do { ((uchar *)(p))[0] = (uchar)(v); ((uchar *)(p))[1] = (uchar)((v) >> 8); ((uchar *)(p))[2] = (uchar)((v) >> 16); ((uchar *)(p))[3] = (uchar)((v) >> 24); } while (0)






#define PBIT64(p,v) do { ((uchar *)(p))[0] = (uchar)(v); ((uchar *)(p))[1] = (uchar)((v) >> 8); ((uchar *)(p))[2] = (uchar)((v) >> 16); ((uchar *)(p))[3] = (uchar)((v) >> 24); ((uchar *)(p))[4] = (uchar)((v) >> 32); ((uchar *)(p))[5] = (uchar)((v) >> 40); ((uchar *)(p))[6] = (uchar)((v) >> 48); ((uchar *)(p))[7] = (uchar)((v) >> 56); } while (0)
# 145 "../kernel/include/fcall.h"
#define BIT8SZ 1
#define BIT16SZ 2
#define BIT32SZ 4
#define BIT64SZ 8
#define QIDSZ (BIT8SZ + BIT32SZ + BIT64SZ)



#define STATFIXLEN (BIT16SZ + QIDSZ + 5 * BIT16SZ + 4 * BIT32SZ + 1 * BIT64SZ)



#define NOTAG (ushort) ~0U
#define NOFID (u32int) ~0U
#define IOHDRSZ 24

enum {
  Tversion = 100,
  Rversion,
  Tauth = 102,
  Rauth,
  Tattach = 104,
  Rattach,
  Terror = 106,
  Rerror,
  Tflush = 108,
  Rflush,
  Twalk = 110,
  Rwalk,
  Topen = 112,
  Ropen,
  Tcreate = 114,
  Rcreate,
  Tread = 116,
  Rread,
  Twrite = 118,
  Rwrite,
  Tclunk = 120,
  Rclunk,
  Tremove = 122,
  Rremove,
  Tstat = 124,
  Rstat,
  Twstat = 126,
  Rwstat,
  Tmax,


  Texec = 128,
  Rexec,




  Tsyscall = 130,
  Rsyscall,



  Tsysopen = 132,
  Rsysopen,
  Tsyscreate = 134,
  Rsyscreate,
  Tsysread = 136,
  Rsysread,
  Tsyswrite = 138,
  Rsyswrite,
  Tsysclose = 140,
  Rsysclose,
  Tsyspread = 142,
  Rsyspread,
  Tsyspwrite = 144,
  Rsyspwrite,
  Tsysremove = 146,
  Rsysremove,


  Tsysstat = 148,
  Rsysstat,
  Tsysfstat = 150,
  Rsysfstat,
  Tsyswstat = 152,
  Rsyswstat,
  Tsysfwstat = 154,
  Rsysfwstat,


  Tsysfork = 160,
  Rsysfork,
  Tsysexec = 162,
  Rsysexec,
  Tsysexit = 164,
  Rsysexit,
  Tsyswait = 166,
  Rsyswait,
  Tsysbrk = 168,
  Rsysbrk,
  Tsyssleep = 170,
  Rsyssleep,


  Tsysbind = 180,
  Rsysbind,
  Tsysmount = 182,
  Rsysmount,
  Tsysunmount = 184,
  Rsysunmount,
  Tsyschdir = 186,
  Rsyschdir,


  Tsysdup = 190,
  Rsysdup,
  Tsyspipe = 192,
  Rsyspipe,
  Tsysfd2path = 194,
  Rsysfd2path,


  Tsysseek = 200,
  Rsysseek,
  Tsysnotify = 202,
  Rsysnotify,
  Tsysalarm = 204,
  Rsysalarm,

  Tsysmax,
};

uint convM2S(uchar *, uint, Fcall *);
uint convS2M(Fcall *, uchar *, uint);
uint sizeS2M(Fcall *);

int statcheck(uchar *abuf, uint nbuf);
uint convM2D(uchar *, uint, Dir *, char *);
uint convD2M(Dir *, uchar *, uint);
uint sizeD2M(Dir *);

int fcallfmt(Fmt *);
int dirfmt(Fmt *);
int dirmodefmt(Fmt *);

int read9pmsg(int, void *, uint);


#pragma varargck type "F" Fcall *
#pragma varargck type "M" ulong
#pragma varargck type "D" Dir *



enum {
  SYS_OPEN = 1,
  SYS_CLOSE,
  SYS_READ,
  SYS_WRITE,
  SYS_PREAD,
  SYS_PWRITE,
  SYS_CREATE,
  SYS_REMOVE = 25,
  SYS_EXIT,
  SYS_FORK,
  SYS_STAT,
  SYS_WSTAT,
  SYS_RFORK = 19,
  SYS_PIPE = 21,
  SYS_SEEK = 39,
  SYS_MOUNT = 46,
  SYS_NSEC = 53,
  SYS_BRK = 55,
  SYS_PEBBLE_ALLOC = 59,
  SYS_PEBBLE_FREE = 60,
  SYS_PEBBLE_INCREASE_BUDGET = 61,
  SYS_WASM_COMPILE = 160,
  SYS_WASM_EXECUTE = 161,
  SYS_WASM_DESTROY = 162,
  SYS_GETPID2 = 66,
  SYS_EXCHANGE_ALLOC = 67,
  SYS_EXCHANGE_FREE = 68,
  SYS_EXCHANGE_PUBLISH = 69,
  SYS_EXCHANGE_SUBSCRIBE = 70,
  SYS_EXCHANGE_UNSUBSCRIBE = 71,
  SYS_EXCHANGE_RECEIVE = 72,
  SYS_WAIT = 166

};
# 89 "wasm/../include/portdat.h" 2

struct Ref {
  long ref;
};

struct Rendez {
  Lock lock;
  Proc *p;
};

struct QLock {
  Lock use;
  Proc *head;
  Proc *tail;
  uintptr pc;
  int locked;
};

struct Rendezq {
  QLock qlock;
  Rendez rendez;
};

struct RWLock {
  Lock use;
  Proc *head;
  Proc *tail;
  uintptr wpc;
  int writer;
  int readers;
};

struct Alarms {
  QLock lock;
  Proc *head;
};

struct Sargs {
  uchar args[5 * sizeof(ulong)];
};




enum {
  Aaccess,
  Abind,
  Atodir,
  Aopen,
  Amount,
  Acreate,
  Aremove,
  Aunmount,

  COPEN = 0x0001,
  CMSG = 0x0002,

  CCEXEC = 0x0008,
  CFREE = 0x0010,
  CRCLOSE = 0x0020,
  CCACHE = 0x0080,
};


enum {
  BINTR = (1 << 0),
  BFREE = (1 << 1),
  Bipck = (1 << 2),
  Budpck = (1 << 3),
  Btcpck = (1 << 4),
  Bpktck = (1 << 5),
};

struct Block {
  Block *next;
  Block *list;
  uchar *rp;
  uchar *wp;
  uchar *lim;
  uchar *base;
  Bpool *pool;
  ushort flag;
  ushort checksum;
};

#define BLEN(s) ((s)->wp - (s)->rp)
#define BALLOC(s) ((s)->lim - (s)->base)

struct Bpool {
  ulong size;
  ulong align;

  Lock lock;
  Block *head;
};

struct Chan {
  long ref;
  Lock lock;
  Chan *next;
  Chan *link;
  vlong offset;
  vlong devoffset;
  ushort type;
  ulong dev;
  ushort mode;
  ushort flag;
  Qid qid;
  int fid;
  ulong iounit;
  Mhead *umh;
  Chan *umc;
  QLock umqlock;
  int uri;
  int dri;
  uchar *dirrock;
  int nrock;
  int mrock;
  QLock rockqlock;
  int ismtpt;
  Mntcache *mcp;
  Mnt *mux;
  union {
    void *aux;
    ulong mid;
  };
  Chan *mchan;
  Qid mqid;
  Path *path;
  char *srvname;
};

struct Path {
  long ref;
  char *s;
  Chan **mtpt;
  int len;
  int alen;
  int mlen;
  int malen;
};

struct Dev {
  int dc;
  char *name;

  void (*reset)(void);
  void (*init)(void);
  void (*shutdown)(void);
  Chan *(*attach)(char *);
  Walkqid *(*walk)(Chan *, Chan *, char **, int);
  int (*stat)(Chan *, uchar *, int);
  Chan *(*open)(Chan *, int);
  Chan *(*create)(Chan *, char *, int, ulong);
  void (*close)(Chan *);
  long (*read)(Chan *, void *, long, vlong);
  Block *(*bread)(Chan *, long, ulong);
  long (*write)(Chan *, void *, long, vlong);
  long (*bwrite)(Chan *, Block *, ulong);
  void (*remove)(Chan *);
  int (*wstat)(Chan *, uchar *, int);
  void (*power)(int);
  int (*config)(int, char *, DevConf *);
};

struct Dirtab {
  char name[28];
  Qid qid;
  vlong length;
  long perm;
};

struct Walkqid {
  Chan *clone;
  int nqid;
  Qid qid[1];
};

struct Mount {
  uvlong mountid;
  int mflag;
  Mount *next;
  Mount *order;
  Chan *to;
  char spec[];
};

struct Mhead {
  long ref;
  RWLock lock;
  Chan *from;
  Mount *mount;
  Mhead *hash;
};

struct Mntrah {
  Rendez rendez;

  ulong vers;

  vlong off;
  vlong seq;

  uint i;
  Mntrpc *r[8];
};

struct Mntproc {
  Rendez rendez;

  Mnt *m;
  Mntrpc *r;
  void *a;
  void (*f)(Mntrpc *, void *);
};

struct Mnt {
  Lock lock;


  Chan *c;
  Proc *rip;
  Mntrpc *queue;
  Mntproc defered[8];
  ulong id;
  Mnt *list;
  int flags;
  int msize;
  char *version;
  Queue *q;
};

enum {
  NUser,
  NExit,
  NDebug,
};

struct Note {
  char msg[128];
  int flag;
  long ref;
};

enum {
  PG_MOD = 0x01,
  PG_REF = 0x02,
  PG_PRIV = 0x04,
};

struct Page {
  long ref;
  Page *next;
  uintptr pa;
  uintptr va;
  uintptr daddr;
  Image *image;
  ushort refage;
  char modref;
  char color;
  char token_color;
# 364 "wasm/../include/portdat.h"
} __attribute__((aligned(64)));

struct Swapalloc {
  Lock lock;
  int free;
  uchar *swmap;
  uchar *alloc;
  uchar *last;
  uchar *top;
  Rendez r;
  ulong highwater;
  ulong headroom;
  ulong xref;
};

extern struct Swapalloc swapalloc;

struct Pte {
  Page *pages[((1ull * 1048576u) / (0x1000ull))];
  Page **first;
  Page **last;
};


enum {
  SG_TYPE = 07,
  SG_TEXT = 00,
  SG_DATA = 01,
  SG_BSS = 02,
  SG_STACK = 03,
  SG_SHARED = 04,
  SG_PHYSICAL = 05,
  SG_FIXED = 06,
  SG_STICKY = 07,

  SG_RONLY = 0040,
  SG_CEXEC = 0100,
  SG_FAULT = 0200,
  SG_CACHED = 0400,
  SG_DEVICE = 01000,
  SG_NOEXEC = 02000,
  SG_WASM = 04000,
};

#define PG_ONSWAP 1
#define onswap(s) (((uintptr)s) & PG_ONSWAP)
#define pagedout(s) (((uintptr)s) == 0 || onswap(s))
#define swapaddr(s) (((uintptr)s) & ~PG_ONSWAP)

#define SEGMAXSIZE (1ULL * SEGMAPSIZE * PTEMAPMEM)

struct Physseg {
  int attr;
  char *name;
  uintptr pa;
  uintptr size;
  Physseg *next;
  Physseg *prev;
};

struct Sema {
  Rendez rendez;
  long *addr;
  int waiting;
  Sema *next;
  Sema *prev;
};

struct Segment {
  QLock qlock;
  long ref;
  int type;
  ulong size;

  uintptr base;
  uintptr top;
  uintptr fstart;
  uintptr flen;

  int flushme;
  Image *image;
  Physseg *pseg;
  ulong *profile;
  Pte **map;
  int mapsize;
  Pte *ssegmap[16];

  ulong used;
  ulong swapped;

  Sema sema;
};

struct Segio {
  QLock lock;
  Rendez cmdwait;
  Rendez replywait;

  Proc *p;
  Segment *s;

  char *data;
  char *addr;
  int dlen;
  int cmd;
  char *err;
};

enum {
  RENDLOG = 5,
  RENDHASH = 1 << RENDLOG,
  MNTLOG = 5,
  MNTHASH = 1 << MNTLOG,
  NFD = 100,
  ENVLOG = 5,
  ENVHASH = 1 << ENVLOG,
};
#define REND(p,s) ((p)->rendhash[(s) & ((1 << RENDLOG) - 1)])
#define MOUNTH(p,qid) ((p)->mnthash[(qid).path & ((1 << MNTLOG) - 1)])
#define PGHASH(i,daddr) ((i)->pghash[((daddr) >> PGSHIFT) & ((i)->pghsize - 1)])


struct Image {
  Lock lock;
  long ref;

  long pgref;

  ulong nattach;

  Image **link;
  Image *next;

  Image *hash;

  Segment *s;

  Chan *c;
  Qid qid;
  ulong dev;
  ushort type;
  char notext;

  ulong pghsize;
  Page *pghash[];
};

struct Pgrp {
  long ref;
  RWLock ns;
  u64int notallowed[4];
  Mhead *mnthash[MNTHASH];


  u8int identity_hash[16];
  u8int namespace_cid[32];

  Lock spawn_lock;
  u32int spawn_limit;
  u32int spawn_count;
};

struct Rgrp {
  long ref;
  Lock lock;
  Proc *rendhash[RENDHASH];
};

struct Evalue {
  char *value;
  int len;
  ulong vers;
  uvlong path;
  Evalue *hash;
  char name[];
};

struct Egrp {
  Ref ref;
  RWLock rwlock;
  Evalue **ent;
  int nent;
  int low;
  int alloc;
  ulong path;
  ulong vers;
  Evalue *hash[ENVHASH];
};

struct Fgrp {
  Lock lock;
  Ref ref;
  Chan **fd;
  uchar *flag;
  int nfd;
  int maxfd;
  int exceed;
};

enum {
  DELTAFD = 20
};

struct Palloc {
  Lock lock;
  Page *head;
  ulong freecount;
  Page *pages;
  ulong user;
  Rendezq pwait[2];
};

struct Waitq {
  Waitmsg w;
  Waitq *next;
};




enum {

  Trelative,
  Tperiodic,
};

struct Timer {

  Lock lock;

  int tmode;
  vlong tns;
  void (*tf)(Ureg *, Timer *);
  void *ta;

  Mach *tactive;
  Timers *tt;
  Tval tticks;
  Tval twhen;
  Timer *tnext;
};

enum {
  RFNAMEG = (1 << 0),
  RFENVG = (1 << 1),
  RFFDG = (1 << 2),
  RFNOTEG = (1 << 3),
  RFPROC = (1 << 4),
  RFMEM = (1 << 5),
  RFNOWAIT = (1 << 6),
  RFCNAMEG = (1 << 10),
  RFCENVG = (1 << 11),
  RFCFDG = (1 << 12),
  RFREND = (1 << 13),
  RFNOMNT = (1 << 14),
};




enum {
  SSEG,
  TSEG,
  DSEG,
  BSEG,
  ESEG,
  P9SEG,
  LSEG,
  SEG1,
  SEG2,
  SEG3,
  SEG4,
  NSEG
};

enum {
  Dead = 0,
  Moribund,
  New,
  Ready,
  Scheding,
  Running,
  Queueing,
  QueueingR,
  QueueingW,
  Wakeme,
  Broken,
  Stopped,
  Rendezvous,
  Waitrelease,

  Proc_stopme = 1,
  Proc_exitme,
  Proc_traceme,
  Proc_exitbig,
  Proc_tracesyscall,

  TUser = 0,
  TSys,
  TReal,
  TCUser,
  TCSys,
  TCReal,

  NERR = 32,
  NNOTE = 5,

  Npriq = 20,
  Nrq = Npriq + 2,
  PriRelease = Npriq,
  PriEdf = Npriq + 1,
  PriNormal = 10,
  PriExtra = Npriq - 1,
  PriKproc = 13,
  PriRoot = 13,
};
#define PROC_GUARD_MAGIC_POST 0xBABECAFEDEADBEEFULL

struct Schedq {
  Lock lock;
  Proc *head;
  Proc *tail;
  int n;
};

struct Proc {
  Label sched;

  union {
    Timer timer;
    struct {
      Lock tlock;
      int tmode;
      vlong tns;
      void (*tf)(Ureg *, Timer *);
      void *ta;
      Mach *tactive;
      Timers *tt;
      Tval tticks;
      Tval twhen;
      Timer *tnext;
    };
  };

  Mach *mach;
  char *text;
  char *user;

  uintptr entry_point;


  char *args;
  int nargs;
  int setargs;

  Proc *rnext;
  Proc *qnext;

  char *psstate;
  int state;
  ushort state_trace;
  ushort hdr_checksum;


  void *p9page;



  uvlong p9page_phys;
  uintptr p9uaddr;

  void *exchange_channel;



  u32int fid_counter;
  u32int dot_fid;
  vlong fid_offsets[256];

  ulong pid;
  uuid_t pid2;
  ulong noteid;
  ulong parentpid;
  ulong index;

  Proc *parent;
  Lock exl;
  Waitq *waitq;
  int nchild;
  int nwait;
  QLock qwaitr;
  Rendez waitr;

  QLock seglock;
  Segment *seg[NSEG];

  Pgrp *pgrp;
  Egrp *egrp;
  Fgrp *fgrp;
  Rgrp *rgrp;

  Fgrp *closingfgrp;

  int insyscall;
  ulong time[6];


  uintptr waiting_for_key;
  struct LockDagContext lockdag;

  uvlong kentry;
# 783 "wasm/../include/portdat.h"
  vlong pcycles;

  QLock debug;
  Proc *pdbg;
  ulong procmode;
  int privatemem;
  int noswap;
  int hang;
  int procctl;

  Lock rlock;
  Rendez *r;
  Rendez sleep;
  int notepending;
  int kp;
  Proc *palarm;
  ulong alarm;
  int newtlb;

  Proc *vforkp;

  uintptr rendtag;
  uintptr rendval;
  Proc *rendhash;

  Rendez *trend;
  int (*tfn)(void *);
  void (*kpfun)(void *);
  void *kparg;

  Sargs s;
  int scallnr;
  int nerrlab;
  Label errlab[NERR];
  char *syserrstr;
  char *errstr;
  char errbuf0[128];
  char errbuf1[128];
  char genbuf[4096];
  Chan *slash;
  Chan *dot;

  Note *lastnote;
  Note *note[NNOTE];
  short nnote;
  short notified;
  int (*notify)(void *, char *);

  Lock *lastlock;
  Lock *lastilock;

  int nlocks;

  ulong delaysched;
  ulong priority;
  ulong basepri;
  uchar fixedpri;
  uchar wired;
  int affinity;
  ulong cpu;
  ulong lastupdate;
  uchar *kstack;

  Edf *edf;
  int trace;

  uintptr qpc;
  uintptr pc;
  QLock *eql;

  void *noteureg;
  void *dbgreg;


  int fpstate;
  int kfpstate;
  FPalloc *fpsave;
  FPalloc *kfpsave;

  MMU *mmuhead;
  MMU *mmutail;
  MMU *kmaphead;
  MMU *kmaptail;
  ulong kmapcount;
  ulong kmapindex;
  ulong mmucount;
  u64int dr[8];
  void *vmx;

  char *syscalltrace;

  Watchpt *watchpt;
  int nwatchpt;


  ulong capabilities;


  Walkqid *walkq;
  Chan *walkclone;
  int walkalloc;


  PebbleState pebble;


  u64int pow_nonce;


  uchar text_hash[64];


#define CLR_TLS_SLOTS 64
  void *clr_tls[64];
  int clr_tls_next_slot;





  struct {
    int initialized;
    void *runtime;
    void *module;
    void *env;
    u8int *linear_memory;
    u32int memory_size;
    u32int memory_pages;
    u32int linear_charged;
    u8int *heap_base;
    u32int heap_size;
    u32int heap_used;
    void *heap_head;
    u32int heap_live;
    arena_branch_t branch;
    void *wasi_ctx;
    void *module_bytes;
    u32int module_bytes_len;
  } wasm;



  uuid_t spawn_cap;
  u32int spawn_max_children;
  u32int spawn_children;




  u8int spawn_bound_binary[64];
} __attribute__((aligned(64)));

enum {
  PRINTSIZE = 256,
  NUMSIZE = 12,
  MB = (1024 * 1024),

  READSTR = 8000,
};

extern Conf conf;
extern char *conffile;
extern int cpuserver;
extern Dev *devtab[];
extern char *eve;
extern char hostdomain[];
extern uchar initcode[];
extern Queue *kprintoq;
extern int nsyscall;
extern Palloc palloc;
extern int panicking;
extern Queue *serialoq;
extern char *statename[];
extern Image *swapimage;
extern Image *fscache;
extern char *sysname;
extern uint qiomaxatomic;
extern char *sysctab[];

enum {
  LRESPROF = 3,
};




struct Log {
  Lock lock;
  int opens;
  char *buf;
  char *end;
  char *rptr;
  int len;
  int nlog;
  int minread;

  int logmask;

  QLock readq;
  Rendez readr;
};

struct Logflag {
  char *name;
  int mask;
};

enum { NCMDFIELD = 128 };

struct Cmdbuf {
  char *buf;
  char **f;
  int nf;
};

struct Cmdtab {
  int index;
  char *cmd;
  int narg;
};




struct PhysUart {
  char *name;
  Uart *(*pnp)(void);
  void (*enable)(Uart *, int);
  void (*disable)(Uart *);
  void (*kick)(Uart *);
  void (*dobreak)(Uart *, int);
  int (*baud)(Uart *, int);
  int (*bits)(Uart *, int);
  int (*stop)(Uart *, int);
  int (*parity)(Uart *, int);
  void (*modemctl)(Uart *, int);
  void (*rts)(Uart *, int);
  void (*dtr)(Uart *, int);
  char *(*status)(Uart *, char *, char *);
  void (*fifo)(Uart *, int);
  void (*power)(Uart *, int);
  int (*getc)(Uart *);
  void (*putc)(Uart *, int);
};

enum { Stagesize = 2048 };




struct Uart {
  void *regs;
  void *saveregs;
  char *name;
  ulong freq;
  int bits;
  int stop;
  int parity;
  int baud;
  PhysUart *phys;
  int console;
  int special;
  Uart *next;

  QLock lock;
  int type;
  int dev;
  int opens;

  int enabled;
  Uart *elist;

  int perr;
  int ferr;
  int oerr;
  int berr;
  int serr;


  int (*putc)(Queue *, int);
  Queue *iq;
  Queue *oq;

  Lock rlock;
  uchar istage[Stagesize];
  uchar *iw;
  uchar *ir;
  uchar *ie;

  Lock tlock;
  uchar ostage[Stagesize];
  uchar *op;
  uchar *oe;
  int drain;

  int modem;
  int xonoff;
  int blocked;
  int cts, dsr, dcd;
  int ctsbackoff;
  int hup_dsr, hup_dcd;
  int dohup;

  Rendez r;
};

extern Uart *consuart;




struct Perf {
  ulong intrts;
  ulong inintr;
  ulong avg_inintr;
  ulong inidle;
  ulong avg_inidle;
  ulong last;
  ulong period;
};

struct Watchdog {
  void (*enable)(void);
  void (*disable)(void);
  void (*restart)(void);
  void (*stat)(char *, char *);
};

struct Watchpt {
  enum {
    WATCHRD = 1,
    WATCHWR = 2,
    WATCHEX = 4,
  } type;
  uintptr addr, len;
};

struct PMach {
  Proc *readied;
  Label sched;
  ulong ticks;
  ulong schedticks;

  int pfault;
  int cs;
  int syscall;
  int load;
  int intr;
  int ilockdepth;

  int flushmmu;

  int tlbfault;
  int tlbpurge;

  Perf perf;

  uvlong cyclefreq;
};


enum {

  Qstarve = (1 << 0),
  Qmsg = (1 << 1),
  Qclosed = (1 << 2),
  Qflow = (1 << 3),
  Qcoalesce = (1 << 4),
  Qkick = (1 << 5),
};

#define DEVDOTDOT -1


#pragma varargck type "I" uchar *
#pragma varargck type "V" uchar *
#pragma varargck type "E" uchar *
#pragma varargck type "M" uchar *






struct Kmesg {
  Lock lk;
  uint n;
  char buf[16384];
};

extern struct Kmesg kmesg;
# 163 "wasm/../include/dat.h" 2

typedef struct {
  u32int _0_;
  u32int rsp0[2];
  u32int rsp1[2];
  u32int rsp2[2];
  u32int _28_[2];
  u32int ist[14];
  u16int _92_[5];
  u16int iomap;
} Tss __attribute__((aligned(64)));

struct Mach {
  int machno;
  uintptr splpc;
  Proc *proc;


  uintptr rbx_restore;
  Proc *readied;
  Label sched;
  ulong ticks;
  ulong schedticks;
  int pfault;
  int cs;
  int syscall;
  int load;
  int intr;
  int ilockdepth;
  int flushmmu;
  int tlbfault;
  int tlbpurge;
  Perf perf;
  uvlong cyclefreq;

  uvlong tscticks;
  ulong spuriousintr;
  int lastintr;

  int loopconst;
  int delaylcycles;
  int cpumhz;
  uvlong cpuhz;

  int cpuidax;
  int cpuidcx;
  int cpuiddx;
  char cpuidid[16];
  char *cpuidtype;
  uchar cpuidfamily;
  uchar cpuidmodel;
  uchar cpuidstepping;

  char havetsc;
  char havepge;
  char havewatchpt8;
  char havenx;
  char haveaes;
  char havesha;
  char havepclmul;
  char haverdrand;

  int fpstate;
  FPalloc *fpsave;

  uintptr *pml4;
  Tss *tss;
  Segdesc *gdt;

  u64int dr7;
  u64int xcr0;
  void *vmx;

  MMU *mmufree;
  ulong mmucount;
  u64int mmumap[4];

  uintptr stack[1];
} __attribute__((aligned(64)));




typedef void KMap;
#define VA(k) ((void *)k)

extern u64int MemMin;

struct Active {
  char machs[128];
  int exiting;
};

extern struct Active active;




struct PCArch {
  char *id;
  int (*ident)(void);
  void (*reset)(void);

  void (*intrinit)(void);
  int (*intrassign)(Vctl *);
  int (*intrirqno)(int, int);
  int (*intrvecno)(int);
  int (*intrspurious)(int);
  void (*introff)(void);
  void (*intron)(void);

  void (*clockinit)(void);
  void (*clockenable)(void);
  uvlong (*fastclock)(uvlong *);
  void (*timerset)(uvlong);
};


enum {

  Xsaveopt = 1 << 0,
  Xsaves = 1 << 3,


  Pclmulqdq = 1 << 1,
  Monitor = 1 << 3,
  Aes = 1 << 25,
  Xsave = 1 << 26,
  Avx = 1 << 28,
  Rdrnd = 1 << 30,


  Fpuonchip = 1 << 0,
  Vmex = 1 << 1,
  Pse = 1 << 3,
  Tsc = 1 << 4,
  Cpumsr = 1 << 5,
  Pae = 1 << 6,
  Mce = 1 << 7,
  Cmpxchg8b = 1 << 8,
  Cpuapic = 1 << 9,
  Mtrr = 1 << 12,
  Pge = 1 << 13,
  Mca = 1 << 14,
  Pat = 1 << 16,
  Pse2 = 1 << 17,
  Clflush = 1 << 19,
  Acpif = 1 << 22,
  Mmx = 1 << 23,
  Fxsr = 1 << 24,
  Sse = 1 << 25,
  Sse2 = 1 << 26,
};

enum {
       PerfEvtbase = 0xc0010000,
       PerfCtrbase = 0xc0010004,

       Efer = 0xc0000080,
       Star = 0xc0000081,
       Lstar = 0xc0000082,
       Cstar = 0xc0000083,
       Sfmask = 0xc0000084,
       FSbase = 0xc0000100,
       GSbase = 0xc0000101,
       KernelGSbase = 0xc0000102,
};




#define NISAOPT 8

struct ISAConf {
  char *type;
  uvlong port;
  int irq;
  ulong dma;
  ulong mem;
  ulong size;
  ulong freq;

  int nopt;
  char *opt[8];
};

extern PCArch *arch;

extern Mach *machp[128];

#define MACHP(n) (machp[n])

extern Mach *m;
extern Proc *up;




typedef struct {
  ulong port;
  int size;
} Devport;

struct DevConf {
  ulong intnum;
  char *type;
  int nports;
  Devport *ports;
};
# 15 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/error.h" 1
extern char Enoerror[];
extern char Emount[];
extern char Eunmount[];
extern char Eismtpt[];
extern char Eunion[];
extern char Emountrpc[];
extern char Eshutdown[];
extern char Enocreate[];
extern char Enonexist[];
extern char Eexist[];
extern char Ebadsharp[];
extern char Enotdir[];
extern char Eisdir[];
extern char Ebadchar[];
extern char Efilename[];
extern char Eperm[];
extern char Ebadusefd[];
extern char Ebadarg[];
extern char Einuse[];
extern char Eio[];
extern char Etoobig[];
extern char Etoosmall[];
extern char Enoport[];
extern char Ehungup[];
extern char Ebadctl[];
extern char Enodev[];
extern char Eprocdied[];
extern char Enochild[];
extern char Eioload[];
extern char Enovmem[];
extern char Ebadfd[];
extern char Enofd[];
extern char Eisstream[];
extern char Ebadexec[];
extern char Etimedout[];
extern char Econrefused[];
extern char Econinuse[];
extern char Eintr[];
extern char Enomem[];
extern char Esoverlap[];
extern char Emouseset[];
extern char Eshort[];
extern char Egreg[];
extern char Ebadspec[];
extern char Enoreg[];
extern char Enoattach[];
extern char Eshortstat[];
extern char Ebadstat[];
extern char Enegoff[];
extern char Ecmdargs[];
extern char Ebadip[];
extern char Edirseek[];
extern char Etoolong[];
extern char Echange[];
# 16 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/fcall.h" 1
# 17 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/fns.h" 1



void _assert(char *);
void accounttime(void);
Timer *addclock0link(void (*)(void), int);
Physseg *addphysseg(Physseg *);
void addbootfile(char *, uchar *, ulong);
void addwatchdog(Watchdog *);
Block *adjustblock(Block *, int);
void alarmkproc(void *);
Block *allocb(int);
int anyhigher(void);
int anyready(void);
Image *attachimage(Chan *, ulong size);
ulong beswal(ulong);
uvlong beswav(uvlong);
int blocklen(Block *);
void bootlinks(void);
void cachedel(Image *, uintptr);
void cachepage(Page *, Image *);
void callwithureg(void (*)(Ureg *));
char *chanpath(Chan *);
int canlock(Lock *);
int canpage(Proc *);
int canqlock(QLock *);
int cmpswap486(long *, long, long);
#define cmpswap(addr,old,new) cmpswap486((long *)(addr), (long)(old), (long)(new))

int canrlock(RWLock *);
void chandevinit(void);
void chandevreset(void);
void chandevshutdown(void);
void chanfree(Chan *);
void checkalarms(void);
void checkpages(void);
void checkb(Block *, char *);
void cinit(void);
Chan *cclone(Chan *);
void cclose(Chan *);
void ccloseq(Chan *);
void closeegrp(Egrp *);
void closefgrp(Fgrp *);
void closepgrp(Pgrp *);
void closergrp(Rgrp *);
long clrfpintr(void);
_Noreturn void cmderror(Cmdbuf *, char *);
int cmount(Chan *, Chan *, int, char *);
void confinit(void);
int consactive(void);
extern void (*consdebug)(void);
void cpushutdown(void);
int copen(Chan *);
void cclunk(Chan *);
Block *concatblock(Block *);
Block *copyblock(Block *, int);
void copypage(Page *, Page *);
void countpagerefs(ulong *, int);
int cread(Chan *, uchar *, int, vlong);
void ctrunc(Chan *);
void cunmount(Chan *, Chan *);
void cupdate(Chan *, uchar *, int, vlong);
void cwrite(Chan *, uchar *, int, vlong);
uintptr dbgpc(Proc *);
Page *deadpage(Page *);
long decref(Ref *);
int decrypt(void *, void *, int);
void delay(int);
Proc *dequeueproc(Schedq *, Proc *);

Chan *devattach(int, char *spec);
Block *devbread(Chan *, long, ulong);
long devbwrite(Chan *, Block *, ulong);
Chan *devclone(Chan *);
int devconfig(int, char *, DevConf *);
Chan *devcreate(Chan *, char *, int, ulong);







void devdir(Chan *c, Qid qid, char *name, vlong length, char *user, long perm,
            Dir *dp);




long devdirread(Chan *c, char *va, long n, Dirtab *tab, int ntab, Devgen *gen);
Devgen devgen;
void devinit(void);
int devno(int, int);
Chan *devopen(Chan *, int, Dirtab *, int, Devgen *);
void devpermcheck(char *, ulong, int);
void devpower(int);
void devremove(Chan *);
void devreset(void);
void devshutdown(void);




int devstat(Chan *c, uchar *dp, int n, Dirtab *tab, int ntab, Devgen *gen);




Walkqid *devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab,
                 int ntab, Devgen *gen);
int devwstat(Chan *, uchar *, int);
Dir *dirchanstat(Chan *);
int donotify(Ureg *);
void syscall_to_9p(Ureg *);
void drawactive(int);
void drawcmap(void);
void dtracytick(Ureg *);
void dumpaproc(Proc *);
void dumpregs(Ureg *);
void dumpstack(void);
Fgrp *dupfgrp(Fgrp *);
void dupswap(Page *);
void edfinit(Proc *);
char *edfadmit(Proc *);
int edfready(Proc *);
void edfrecord(Proc *);
void edfrun(Proc *, int);
void edfstop(Proc *);
void edfyield(void);
int emptystr(char *);
int encrypt(void *, void *, int);
void envcpy(Egrp *, Egrp *);
int eqchan(Chan *, Chan *, int);
int eqchantdqid(Chan *, int, int, Qid, int);
int eqqid(Qid, Qid);
# 146 "wasm/../include/fns.h"
_Noreturn void error(char *e);

void eqlock(QLock *);
uintptr execregs(uintptr, ulong, ulong);
void exhausted(char *);
void exit(int);
uvlong fastticks(uvlong *);
uvlong fastticks2ns(uvlong);
uvlong fastticks2us(uvlong);
int fault(uintptr, uintptr, int);
int fixfault(Segment *, uintptr, int);
void faultnote(char *, char *, uintptr);
void fdclose(int, int);
Chan *fdtochan(int, int, int, int);
int findmount(Chan **, Mhead **, int, int, Qid);
void flushmmu(void);
void forceclosefgrp(void);
void forkchild(Proc *, Ureg *);
void forkret(void);
void fpunotify(Proc *);
void fpunoted(Proc *);



void free(void *p);
void freeb(Block *);
void freeblist(Block *);
int freebroken(void);
void freenote(Note *);
void freenotes(Proc *);
void freepages(Page *, Page *, ulong);
void getcolor(ulong, ulong *, ulong *, ulong *);
uintptr getmalloctag(void *);
uintptr getrealloctag(void *);
_Noreturn void gotolabel(Label *);
char *getconfenv(void);
void growbp(Bpool *, int);
long hostdomainwrite(char *, int);
long hostownerwrite(char *, int);
extern void (*hwrandbuf)(void *, ulong);
void hzsched(void);
Block *iallocb(int);
Block *iallocbp(Bpool *);
uintptr ibrk(uintptr, int);
void ilock(Lock *);
_Noreturn void interrupted(void);
void iunlock(Lock *);
ulong imagecached(void);
ulong imagereclaim(ulong);
long incref(Ref *);
void init0(void);
void initseg(void);
int ioalloc(ulong, ulong, ulong, char *);
void iofree(ulong);
void iomapinit(ulong);
int ioreserve(ulong, ulong, ulong, char *);
int ioreservewin(ulong, ulong, ulong, ulong, char *);
int iounused(ulong, ulong);
int iprint(char *, ...);
int iprint_intr(char *, ...);
void isdir(Chan *);
int iseve(void);
int islo(void);
Segment *isoverlap(uintptr, uintptr);
Physseg *findphysseg(char *);
int kenter(Ureg *);
void kexit(Ureg *);
void kickpager(void);
void killbig(void);
void killproc(Proc *, int);
int kproc(char *, void (*)(void *), void *);
void kprocchild(Proc *, void (*)(void));
void linkproc(void);
extern void (*kproftimer)(uintptr);
void ksetenv(char *, char *, int);
int kopen(char *, int);
void kstrcpy(char *, char *, int);
void kstrdup(char **, char *);
void lock(Lock *);
void logopen(Log *);
void logclose(Log *);
char *logctl(Log *, int, char **, Logflag *);
void logn(Log *, int, void *, int);
long logread(Log *, void *, ulong, long);
void log(Log *, int, char *, ...);
Cmdtab *lookupcmd(Cmdbuf *, Cmdtab *, int);
Page *lookpage(Image *, uintptr);
#define MS2NS(n) (((vlong)(n)) * 1000000LL)
void machinit(void);
# 248 "wasm/../include/fns.h"
void *mallocz(ulong size, int clr);
# 262 "wasm/../include/fns.h"
void *malloc(ulong size);
# 276 "wasm/../include/fns.h"
void *mallocalign(ulong size, ulong align, long offset, ulong span);
void mallocsummary(void);
void memmapdump(void);
uvlong memmapnext(uvlong, ulong);
uvlong memmapsize(uvlong, uvlong);
void memmapadd(uvlong, uvlong, ulong);
uvlong memmapalloc(uvlong, uvlong, uvlong, ulong);
void memmapfree(uvlong, uvlong, ulong);
void mfreeseg(Segment *, uintptr, ulong);
void microdelay(int);
uvlong mk64fract(uvlong, uvlong);
void mkqid(Qid *, vlong, ulong, int);
void mmurelease(Proc *);
void mmuswitch(Proc *);
Chan *mntattach(Chan *, Chan *, char *, int);
Chan *mntauth(Chan *, char *);
int mntversion(Chan *, char *, int, int);
void mouseresize(void);
void mountfree(Mount *);
ulong ms2tk(ulong);
ulong msize(void *);
ulong ms2tk(ulong);
uvlong ms2fastticks(ulong);
void mul64fract(uvlong *, uvlong, uvlong);
void muxclose(Mnt *);
Chan *namec(char *, int, int, ulong);
_Noreturn void namelenerror(char *, int, char *);
int needpages(void *);
Chan *newchan(void);
Egrp *newegrp(void);
int growfd(Fgrp *, int);
void unlockfgrp(Fgrp *);
int newfd(Chan *, int);
Mhead *newmhead(Chan *);
Mount *newmount(Chan *, int, char *);
Image *newimage(ulong);
Page *newpage(uintptr, Segment *);
Path *newpath(char *);
Pgrp *newpgrp(void);
Rgrp *newrgrp(void);
Proc *newproc(void);
_Noreturn void nexterror(void);
Ureg *notify(Ureg *, char *);
int noted(Ureg *, Ureg *, int);
FPsave *notefpsave(Proc *);
ulong nkpages(Confmem *);
uvlong ns2fastticks(uvlong);
int okaddr(uintptr, ulong, int);
int openmode(ulong);
Block *packblock(Block *);
Block *padblock(Block *, int);
void pageinit(void);
ulong pagereclaim(Image *);





_Noreturn void panic(char *fmt, ...);
Cmdbuf *parsecmd(char *a, int n);
void pathclose(Path *);
ulong perfticks(void);
_Noreturn void pexit(char *, int);
void pgrpcpy(Pgrp *, Pgrp *);
void namespace_cid_update(Pgrp *);
void namespace_cid_update_locked(Pgrp *);
ulong pidalloc(Proc *);
#define waserror() setlabel(&up->errlab[up->nerrlab++])
#define poperror() up->nerrlab--
void portcountpagerefs(ulong *, int);
char *popnote(Ureg *);
int postnote(Proc *, int, char *, int);
void postnotepg(ulong, char *, int);
int pprint(char *, ...);
void preempted(int);
void prflush(void);
void printinit(void);
void setkprintqsize(char *);
void prbuf_init(void);
int prbuf_print(char *, int);
void prbuf_start_consumer(void);
int prbuf_has_data(void);
int prbuf_ready(void);
void prbuf_kprint_open(void);
void prbuf_kprint_close(void);
long prbuf_kprint_read(void *, long);
ulong procalarm(ulong);
void procctl(void);
int procfdprint(Chan *, int, char *, int);
void procflushseg(Segment *);
void procflushpseg(Physseg *);
void procflushothers(void);
int procindex(ulong);
void procinit0(void);
void procinterrupt(Proc *);
ulong procpagecount(Proc *);
void procpriority(Proc *, int, int);
void procsetuser(char *);
Proc *proctab(int);
extern void (*proctrace)(Proc *, int, vlong);
void procwired(Proc *, int);
Pte *ptealloc(void);
int pullblock(Block **, int);
Block *pullupblock(Block *, int);
Block *pullupqueue(Queue *, int);
int pushnote(Proc *, Note *);
void putimage(Image *);
void putmhead(Mhead *);
void putmmu(uintptr, uintptr, Page *);
void putpage(Page *);
void putseg(Segment *);
void putstrn(char *, int);
void putswap(Page *);
ulong pwait(Waitmsg *);
int qaddlist(Queue *, Block *);
Block *qbread(Queue *, int);
long qbwrite(Queue *, Block *);
Queue *qbypass(void (*)(void *, Block *), void *);
int qcanread(Queue *);




void qclose(Queue *q);
int qconsume(Queue *, void *, int);
Block *qcopy(Queue *, int, ulong);
int qdiscard(Queue *, int);
void qflush(Queue *);




void qfree(Queue *q);
int qfull(Queue *);
Block *qget(Queue *);
void qhangup(Queue *, char *);
int qisclosed(Queue *);
int qiwrite(Queue *, void *, int);




int qlen(Queue *q);




void qlock(QLock *l);




Queue *qopen(int, int, void (*)(void *), void *);
int qpass(Queue *, Block *);
int qpassnolim(Queue *, Block *);
int qproduce(Queue *, void *, int);
void qputback(Queue *, Block *);





long qread(Queue *q, void *buf, int n);
Block *qremove(Queue *);
void qreopen(Queue *);
void qsetlimit(Queue *, int);




void qunlock(QLock *l);





int qwrite(Queue *q, void *buf, int n);
void qnoblock(Queue *, int);
void qsetnoblock_early(Queue *, int);
void randominit(void);
ulong randomread(void *, ulong);
void rdb(void);
long readblist(Block *, uchar *, long, ulong);
int readnum(ulong, char *, ulong, ulong, int);
int readstr(ulong, char *, ulong, char *);
void ready(Proc *);
void *realloc(void *v, ulong size);
void rebootcmd(int, char **);
void reboot(void *, void *, ulong);
void relocateseg(Segment *, uintptr);
void renameuser(char *, char *);
void resched(char *);
void resrcwait(char *);
int return0(void *);
void rlock(RWLock *);
long rtctime(void);
void runlock(RWLock *);
Proc *runproc(void);
void sched(void);
_Noreturn void schedinit(void);
extern void (*screenputs)(char *, int);
void *secalloc(ulong);
void secfree(void *);
long seconds(void);
uintptr segattach(int, char *, uintptr, uintptr);
void segclock(uintptr);
long segio(Segio *, Segment *, void *, long, vlong, int);
void segpage(Segment *, Page *);
int setcolor(ulong, ulong, ulong, ulong);
void setkernur(Ureg *, Proc *);
int setlabel(Label *);
void setmalloctag(void *, uintptr);
ulong setnoteid(Proc *, ulong);
void setrealloctag(void *, uintptr);
void setregisters(Ureg *, char *, char *, int);
void setupwatchpts(Proc *, Watchpt *, int);
char *skipslash(char *);
void sleep(Rendez *, int (*)(void *), void *);
void *smalloc(ulong);
void *pebble_meta_alloc(ulong);
void pebble_meta_free(void *);
int splhi(void);
int spllo(void);
void splx(int);
void splxpc(int);
char *srvname(Chan *);
void srvrenameuser(char *, char *);
void shrrenameuser(char *, char *);
int swapcount(uintptr);
int swapfull(void);
void syscallfmt(ulong syscallno, uintptr pc, va_list list);
void sysretfmt(ulong syscallno, va_list list, uintptr ret, uvlong start,
               uvlong stop);
void timeradd(Timer *);
void timerdel(Timer *);
void timersinit(void);
void timerintr(Ureg *, Tval);
void timerset(Tval);
ulong tk2ms(ulong);
#define TK2MS(x) ((x) * (1000 / HZ))
uvlong tod2fastticks(vlong);
vlong todget(vlong *, vlong *);
void todsetfreq(vlong);
void todinit(void);
void todset(vlong, vlong, int);
Block *trimblock(Block *, int, int);
void tsleep(Rendez *, int (*)(void *), void *, ulong);
void twakeup(Ureg *, Timer *);
int uartctl(Uart *, char *);
int uartgetc(void);
void uartkick(void *);
void uartmouse(char *, int (*)(Queue *, int), int);
void uartsetmouseputc(char *, int (*)(Queue *, int));
void uartputc(int);
void uartputs(char *, int);
void uartrecv(Uart *, char);
int uartstageoutput(Uart *);
void unbreak(Proc *);
void uncachepage(Page *);
long unionread(Chan *, void *, long);
void unlock(Lock *);
uvlong us2fastticks(uvlong);
void userinit(void);
uintptr userpc(void);
long userwrite(char *, int);
void validaddr(uintptr, ulong, int);
void validname(char *, int);
char *validnamedup(char *, int);
void validstat(uchar *, int);
void *vmemchr(void *, int, ulong);
Proc *wakeup(Rendez *);
int walk(Chan **, char **, int, int, int *);
void wlock(RWLock *);
void wunlock(RWLock *);





void *xalloc(ulong size);





void *xalloc_raw(ulong size);





void *xallocz(ulong size, int zero);





void *xallocz_raw(ulong size, int zero);





void *xalloc_driver(ulong size);
void *xallocz_driver(ulong size, int zero);
void *smalloc_driver(ulong size);
void xfree_driver(void *p);



void xfree(void *p);
void xhole(uintptr, uintptr);
void xinit(void);
int xmerge(void *, void *);
void *xspanalloc(ulong, int, ulong);
void xsummary(void);
void *bootstrap_alloc(ulong size);
void *bootstrap_alloc_aligned(ulong size, ulong alignment);
uintptr get_hhdm_offset(void);
void yield(void);
Page *fillpage(Page *, int);
void zeroprivatepages(void);
Segment *data2txt(Segment *);
Segment *dupseg(Segment **, int, int);
Segment *newseg(int, uintptr, ulong);
Segment *seg(Proc *, uintptr, int);
Segment *txt2data(Segment *);
void hnputv(void *, uvlong);
void hnputl(void *, uint);
void hnputs(void *, ushort);
uvlong nhgetv(void *);
uint nhgetl(void *);
ushort nhgets(void *);





ulong \U000000b5s(void);

long lcycles(void);
extern void (*cycles)(uvlong *);
void devmask(Pgrp *, int, char *);
int devallowed(Pgrp *, int);
int canmount(Pgrp *);


extern int (*pcicfgrw8)(int, int, int, int);
extern int (*pcicfgrw16)(int, int, int, int);
extern int (*pcicfgrw32)(int, int, int, int);


#pragma varargck argpos iprint 1
#pragma varargck argpos panic 1
#pragma varargck argpos pprint 1




extern void *kaddr(uintptr);
#define KADDR(a) kaddr(a)
# 645 "wasm/../include/fns.h"
#define evenaddr(x) 



int userureg(Ureg *);


KMap *kmap(Page *);
void kunmap(KMap *);


#define kmapinval() 


void setuppagetables(void);


void pebbleinit(void);
void pebbleprocinit(Proc *);
void pebble_cleanup(Proc *);


void intrdisable(int, void (*)(Ureg *, void *), void *, int, char *);
void intrenable(int, void (*)(Ureg *, void *), void *, int, char *);


void idlehands(void);
int tas(ulong *);

extern void (*coherence)(void);
void SET(void *);

Dirtab *addarchfile(char *, int, long (*)(Chan *, void *, long, vlong),
                    long (*)(Chan *, void *, long, vlong));

ushort ins(int port);
void outs(int port, ushort value);


uintptr cankaddr(uintptr);





enum PageOwnError pageown_acquire(Proc *, uintptr,
                                  u64int);
enum PageOwnError pageown_release(Proc *, uintptr);

void pageown_cleanup_process(Proc *);



void procsave(Proc *);
void procrestore(Proc *);
void procsetup(Proc *);
void procfork(Proc *);
int proc_setup_p9page(Proc *);
int proc_setup_p9seg_stub(Proc *);
uintptr p9_pick_uaddr(Proc *, const UserCapability *);
void *kernel_setup_init_exchange(
    Proc *);


uintptr *mmuwalk(uintptr *, uintptr, int, int);
u64int getcr3(void);
void putcr3(u64int);


void devregistry_init(void);
void pci_framework_init(void);
int pci_framework_enumerate(void);


void benchmark_init(void);
void benchmark_enable(void);
void benchmark_disable(void);
void benchmark_boot_start(void);
void benchmark_boot_stage(int);
void benchmark_boot_end(void);
void benchmark_print_summary(void);
int validate_all(void);
uvlong rdtsc(void);


void *vmap(uvlong, vlong);
void vunmap(void *, vlong);

long kread(int, void *, long);
long kwrite(int, void *, long);
vlong kseek(int, vlong, int);


void wasm_runtime_init(void);
void wasm_arena_test(void);
struct Chan;
struct M3Function;
int wasm_exec_compile(struct Chan *, struct M3Function **);
void wasm_exec_run(struct M3Function *);
void wasm_runtime_cleanup_process(Proc *);

extern int boot_verbose;






int bprint(const char *fmt, ...);




void bpanic(const char *fmt, ...) __attribute__((noreturn));
# 18 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/mem.h" 1
# 19 "wasm/wasm_runtime.c" 2

# 1 "wasm/../include/pebble_kernel.h" 1
# 10 "wasm/../include/pebble_kernel.h"
#define _PEBBLE_KERNEL_H_ 


typedef struct PebbleKernelAlloc PebbleKernelAlloc;






PebbleKernelAlloc *pebble_kernel_reserve(ulong size);


void pebble_kernel_activate(PebbleKernelAlloc *alloc);


void pebble_kernel_free(PebbleKernelAlloc *alloc);






void *pebble_kernel_alloc(ulong size);

void pebble_kernel_free_ptr(void *ptr);




void pebble_kernel_stats(uvlong *reserves, uvlong *activates, uvlong *frees,
                         uvlong *white, uvlong *black);




void pebble_kernel_init(void);
# 21 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/portlib.h" 1
# 22 "wasm/wasm_runtime.c" 2
# 1 "wasm/../include/u.h" 1
# 23 "wasm/wasm_runtime.c" 2


# 1 "wasm/wasm_host_lux9.h" 1






#define WASM_HOST_LUX9_H 

# 1 "wasm/wasm_runtime/wasm3/wasm3.h" 1
# 9 "wasm/wasm_runtime/wasm3/wasm3.h"
#define wasm3_h 

#define M3_VERSION_MAJOR 0
#define M3_VERSION_MINOR 5
#define M3_VERSION_REV 1
#define M3_VERSION "0.5.1"




# 1 "../kernel/include/u.h" 1
# 20 "wasm/wasm_runtime/wasm3/wasm3.h" 2



typedef u64int uint64_t;
typedef u32int uint32_t;
typedef u16int uint16_t;
typedef u8int uint8_t;
typedef s64int int64_t;
typedef s32int int32_t;
typedef s16int int16_t;
typedef s8int int8_t;
typedef usize size_t;
typedef ssize ssize_t;
typedef uintptr uintptr_t;
typedef intptr intptr_t;

#define NULL nil

#define SIZE_MAX ((size_t)-1)



#define INT8_MIN (-128)
#define INT16_MIN (-32768)
#define INT32_MIN (-2147483647 - 1)
#define INT64_MIN (-9223372036854775807LL - 1)
#define INT8_MAX 127
#define INT16_MAX 32767
#define INT32_MAX 2147483647
#define INT64_MAX 9223372036854775807LL
#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615ULL
# 63 "wasm/wasm_runtime/wasm3/wasm3.h"
# 1 "wasm/wasm_runtime/wasm3/wasm3_defs.h" 1
# 9 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define wasm3_defs_h 

#define M3_STR__(x) #x
#define M3_STR(x) M3_STR__(x)

#define M3_CONCAT__(a,b) a ##b
#define M3_CONCAT(a,b) M3_CONCAT__(a,b)
# 26 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define M3_COMPILER_GCC 1
# 40 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define M3_COMPILER_VER "GCC " __VERSION__
# 52 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define M3_COMPILER_HAS_FEATURE(x) 0



#define M3_COMPILER_HAS_BUILTIN(x) __has_builtin(x)





#define M3_COMPILER_HAS_ATTRIBUTE(x) __has_attribute(x)
# 74 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define M3_LITTLE_ENDIAN 
# 90 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define M3_ARCH "x86_64"
# 228 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define m3_bswap16(x) __builtin_bswap16((x))
#define m3_bswap32(x) __builtin_bswap32((x))
#define m3_bswap64(x) __builtin_bswap64((x))
# 279 "wasm/wasm_runtime/wasm3/wasm3_defs.h"
#define m3_isBitSet(val,pos) ((val & (1 << pos)) != 0)






#define M3_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define M3_LIKELY(x) __builtin_expect(!!(x), 1)
# 64 "wasm/wasm_runtime/wasm3/wasm3.h" 2


#define M3_BACKTRACE_TRUNCATED (IM3BacktraceFrame)(SIZE_MAX)





typedef const char *M3Result;

struct M3Environment;
typedef struct M3Environment *IM3Environment;
struct M3Runtime;
typedef struct M3Runtime *IM3Runtime;
struct M3Module;
typedef struct M3Module *IM3Module;
struct M3Function;
typedef struct M3Function *IM3Function;
struct M3Global;
typedef struct M3Global *IM3Global;

typedef struct M3ErrorInfo {
  M3Result result;

  IM3Runtime runtime;
  IM3Module module;
  IM3Function function;

  const char *file;
  uint32_t line;

  const char *message;
} M3ErrorInfo;

typedef struct M3BacktraceFrame {
  uint32_t moduleOffset;
  IM3Function function;

  struct M3BacktraceFrame *next;
} M3BacktraceFrame, *IM3BacktraceFrame;

typedef struct M3BacktraceInfo {
  IM3BacktraceFrame frames;
  IM3BacktraceFrame lastFrame;
} M3BacktraceInfo, *IM3BacktraceInfo;

typedef enum M3ValueType {
  c_m3Type_none = 0,
  c_m3Type_i32 = 1,
  c_m3Type_i64 = 2,
  c_m3Type_f32 = 3,
  c_m3Type_f64 = 4,

  c_m3Type_unknown
} M3ValueType;

typedef struct M3TaggedValue {
  M3ValueType type;
  union M3ValueUnion {
    uint32_t i32;
    uint64_t i64;
    float f32;
    double f64;
  } value;
} M3TaggedValue, *IM3TaggedValue;

typedef struct M3ImportInfo {
  const char *moduleUtf8;
  const char *fieldUtf8;
} M3ImportInfo, *IM3ImportInfo;

typedef struct M3ImportContext {
  void *userdata;
  IM3Function function;
} M3ImportContext, *IM3ImportContext;
# 152 "wasm/wasm_runtime/wasm3/wasm3.h"
#define d_m3ErrorConst(LABEL,STRING) extern const M3Result m3Err_ ##LABEL;




extern const M3Result m3Err_none;


    extern const M3Result m3Err_mallocFailed;


    extern const M3Result m3Err_incompatibleWasmVersion;
        extern const M3Result m3Err_wasmMalformed; extern const M3Result m3Err_misorderedWasmSection;

            extern const M3Result m3Err_wasmUnderrun;
                extern const M3Result m3Err_wasmOverrun;
                    extern const M3Result m3Err_wasmMissingInitExpr;

                        extern const M3Result m3Err_lebOverflow;
                                                                extern const M3Result m3Err_missingUTF8;

                            extern const M3Result m3Err_wasmSectionUnderrun;


                                extern const M3Result m3Err_wasmSectionOverrun;


                                    extern const M3Result m3Err_invalidTypeId;

                                        extern const M3Result m3Err_tooManyMemorySections;


                                            extern const M3Result m3Err_tooManyArgsRets;




    extern const M3Result m3Err_moduleNotLinked;

        extern const M3Result m3Err_moduleAlreadyLinked;

            extern const M3Result m3Err_functionLookupFailed;
                extern const M3Result m3Err_functionImportMissing;


                    extern const M3Result m3Err_malformedFunctionSignature;



    extern const M3Result m3Err_noCompiler;
        extern const M3Result m3Err_unknownOpcode;
                                         extern const M3Result m3Err_restrictedOpcode;

            extern const M3Result m3Err_functionStackOverflow;

                extern const M3Result m3Err_functionStackUnderrun;

                    extern const M3Result m3Err_mallocFailedCodePage;


                        extern const M3Result m3Err_settingImmutableGlobal;

                            extern const M3Result m3Err_typeMismatch;

                                extern const M3Result m3Err_typeCountMismatch;



    extern const M3Result m3Err_missingCompiledCode;
        extern const M3Result m3Err_wasmMemoryOverflow;
            extern const M3Result m3Err_globalMemoryNotAllocated;

                extern const M3Result m3Err_globaIndexOutOfBounds;

                    extern const M3Result m3Err_argumentCountMismatch;

                        extern const M3Result m3Err_argumentTypeMismatch;

                            extern const M3Result m3Err_globalLookupFailed;

                                extern const M3Result m3Err_globalTypeMismatch;

                                    extern const M3Result m3Err_globalNotMutable;



    extern const M3Result m3Err_trapOutOfBoundsMemoryAccess;

        extern const M3Result m3Err_trapDivisionByZero;
            extern const M3Result m3Err_trapIntegerOverflow;
                extern const M3Result m3Err_trapIntegerConversion;

                    extern const M3Result m3Err_trapIndirectCallTypeMismatch;

                        extern const M3Result m3Err_trapTableIndexOutOfRange;

                            extern const M3Result m3Err_trapTableElementIsNull;

                                extern const M3Result m3Err_trapExit;

                                    extern const M3Result m3Err_trapAbort;


                                        extern const M3Result m3Err_trapUnreachable;


                                            extern const M3Result m3Err_trapStackOverflow;
# 270 "wasm/wasm_runtime/wasm3/wasm3.h"
    IM3Environment m3_NewEnvironment(void);

void m3_FreeEnvironment(IM3Environment i_environment);

typedef M3Result (*M3SectionHandler)(IM3Module i_module, const char *name,
                                     const uint8_t *start, const uint8_t *end);

void m3_SetCustomSectionHandler(IM3Environment i_environment,
                                M3SectionHandler i_handler);





IM3Runtime m3_NewRuntime(IM3Environment io_environment,
                         uint32_t i_stackSizeInBytes, void *i_userdata);

void m3_FreeRuntime(IM3Runtime i_runtime);


uint8_t *m3_GetMemory(IM3Runtime i_runtime, uint32_t *o_memorySizeInBytes,
                      uint32_t i_memoryIndex);


uint32_t m3_GetMemorySize(IM3Runtime i_runtime);

void *m3_GetUserData(IM3Runtime i_runtime);






M3Result m3_ParseModule(IM3Environment i_environment, IM3Module *o_module,
                        const uint8_t *const i_wasmBytes,
                        uint32_t i_numWasmBytes);




void m3_FreeModule(IM3Module i_module);



M3Result m3_LoadModule(IM3Runtime io_runtime, IM3Module io_module);


M3Result m3_CompileModule(IM3Module io_module);


M3Result m3_RunStart(IM3Module i_module);





typedef const void *(*M3RawCall)(IM3Runtime runtime, IM3ImportContext _ctx,
                                 uint64_t *_sp, void *_mem);

M3Result m3_LinkRawFunction(IM3Module io_module, const char *const i_moduleName,
                            const char *const i_functionName,
                            const char *const i_signature,
                            M3RawCall i_function);

M3Result m3_LinkRawFunctionEx(IM3Module io_module,
                              const char *const i_moduleName,
                              const char *const i_functionName,
                              const char *const i_signature,
                              M3RawCall i_function, const void *i_userdata);

const char *m3_GetModuleName(IM3Module i_module);
void m3_SetModuleName(IM3Module i_module, const char *name);
IM3Runtime m3_GetModuleRuntime(IM3Module i_module);




IM3Global m3_FindGlobal(IM3Module io_module, const char *const i_globalName);

M3Result m3_GetGlobal(IM3Global i_global, IM3TaggedValue o_value);

M3Result m3_SetGlobal(IM3Global i_global, const IM3TaggedValue i_value);

M3ValueType m3_GetGlobalType(IM3Global i_global);




M3Result m3_Yield(void);


M3Result m3_FindFunction(IM3Function *o_function, IM3Runtime i_runtime,
                         const char *const i_functionName);
M3Result m3_GetTableFunction(IM3Function *o_function, IM3Module i_module,
                             uint32_t i_index);

uint32_t m3_GetArgCount(IM3Function i_function);
uint32_t m3_GetRetCount(IM3Function i_function);
M3ValueType m3_GetArgType(IM3Function i_function, uint32_t i_index);
M3ValueType m3_GetRetType(IM3Function i_function, uint32_t i_index);

M3Result m3_CallV(IM3Function i_function, ...);
M3Result m3_CallVL(IM3Function i_function, va_list i_args);
M3Result m3_Call(IM3Function i_function, uint32_t i_argc,
                 const void *i_argptrs[]);
M3Result m3_CallArgv(IM3Function i_function, uint32_t i_argc,
                     const char *i_argv[]);

M3Result m3_GetResultsV(IM3Function i_function, ...);
M3Result m3_GetResultsVL(IM3Function i_function, va_list o_rets);
M3Result m3_GetResults(IM3Function i_function, uint32_t i_retc,
                       const void *o_retptrs[]);

void m3_GetErrorInfo(IM3Runtime i_runtime, M3ErrorInfo *o_info);
void m3_ResetErrorInfo(IM3Runtime i_runtime);

const char *m3_GetFunctionName(IM3Function i_function);
IM3Module m3_GetFunctionModule(IM3Function i_function);





void m3_PrintRuntimeInfo(IM3Runtime i_runtime);
void m3_PrintM3Info(void);
void m3_PrintProfilerInfo(void);



IM3BacktraceInfo m3_GetBacktrace(IM3Runtime i_runtime);





#define m3ApiOffsetToPtr(offset) (void *)((uint8_t *)_mem + (uint32_t)(offset))
#define m3ApiPtrToOffset(ptr) (uint32_t)((uint8_t *)ptr - (uint8_t *)_mem)

#define m3ApiReturnType(TYPE) TYPE *raw_return = ((TYPE *)(_sp++));
#define m3ApiMultiValueReturnType(TYPE,NAME) TYPE *NAME = ((TYPE *)(_sp++));
#define m3ApiGetArg(TYPE,NAME) TYPE NAME = *((TYPE *)(_sp++));
#define m3ApiGetArgMem(TYPE,NAME) TYPE NAME = (TYPE)m3ApiOffsetToPtr(*((uint32_t *)(_sp++)));


#define m3ApiIsNullPtr(addr) ((void *)(addr) <= _mem)
#define m3ApiCheckMem(addr,len) { if (M3_UNLIKELY( ((void *)(addr) < _mem) || ((uint64_t)(uintptr_t)(addr) + (len)) > ((uint64_t)(uintptr_t)(_mem) + m3_GetMemorySize(runtime)))) m3ApiTrap(m3Err_trapOutOfBoundsMemoryAccess); }
# 424 "wasm/wasm_runtime/wasm3/wasm3.h"
#define m3ApiRawFunction(NAME) const void *NAME(IM3Runtime runtime, IM3ImportContext _ctx, uint64_t *_sp, void *_mem)


#define m3ApiReturn(VALUE) { *raw_return = (VALUE); return m3Err_none; }




#define m3ApiMultiValueReturn(NAME,VALUE) { *NAME = (VALUE); }



#define m3ApiTrap(VALUE) { return VALUE; }



#define m3ApiSuccess() { return m3Err_none; }
# 467 "wasm/wasm_runtime/wasm3/wasm3.h"
#define m3ApiReadMem8(ptr) (*(uint8_t *)(ptr))
#define m3ApiReadMem16(ptr) (*(uint16_t *)(ptr))
#define m3ApiReadMem32(ptr) (*(uint32_t *)(ptr))
#define m3ApiReadMem64(ptr) (*(uint64_t *)(ptr))
#define m3ApiWriteMem8(ptr,val) { *(uint8_t *)(ptr) = (val); }



#define m3ApiWriteMem16(ptr,val) { *(uint16_t *)(ptr) = (val); }



#define m3ApiWriteMem32(ptr,val) { *(uint32_t *)(ptr) = (val); }



#define m3ApiWriteMem64(ptr,val) { *(uint64_t *)(ptr) = (val); }
# 10 "wasm/wasm_host_lux9.h" 2


M3Result LinkLux9(IM3Module module);
# 26 "wasm/wasm_runtime.c" 2
# 1 "wasm/wasm_runtime.h" 1






       



typedef struct Proc Proc;


#define PERM_WASM_COMPILE (1 << 16)
#define PERM_WASM_EXECUTE (1 << 17)
#define PERM_WASM_NET (1 << 18)
#define PERM_WASM_POSIX (1 << 19)


void wasm_runtime_init(void);


int sys_wasm_compile(Fcall *tx, Fcall *rx);
int sys_wasm_execute(Fcall *tx, Fcall *rx);
int sys_wasm_destroy(Fcall *tx, Fcall *rx);
void wasm_runtime_cleanup_process(Proc *p);


void wasm_runtime_stats(void);
# 27 "wasm/wasm_runtime.c" 2


typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s8int int8_t;
typedef s16int int16_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef usize size_t;
typedef ssize ssize_t;


# 1 "wasm/wasi_lux9_shim.h" 1






#define WASI_LUX9_SHIM_H 






typedef u8int uint8_t;
typedef u16int uint16_t;
typedef u32int uint32_t;
typedef u64int uint64_t;
typedef s32int int32_t;
typedef s64int int64_t;
typedef uintptr uintptr_t;




typedef struct {
  int is_open;
  u32int lux9_fid;
  u32int capability_idx;
  u32int rights;
  u32int rights_inheriting;
  int is_dir;
  int backend;
  char *base_path;
  u64int offset;
} wasi_fd_entry_t;


#define WASI_MAX_FDS 64
typedef struct {
  wasi_fd_entry_t fds[64];
  u32int exit_code;
  int argc;
  char **argv;
  int envc;
  char **envv;
} wasi_context_t;


#define WASI_ALLOW_ARGS 0x00000001u
#define WASI_ALLOW_CLOCK 0x00000002u
#define WASI_ALLOW_RANDOM 0x00000004u
#define WASI_ALLOW_FD 0x00000008u
#define WASI_ALLOW_PATH 0x00000010u
#define WASI_ALLOW_DIR 0x00000020u
#define WASI_ALLOW_PROC 0x00000040u
#define WASI_ALLOW_POLL 0x00000080u
#define WASI_ALLOW_SOCK 0x00000100u

#define WASI_ALLOW_DEFAULT (WASI_ALLOW_ARGS | WASI_ALLOW_CLOCK | WASI_ALLOW_RANDOM | WASI_ALLOW_FD | WASI_ALLOW_PATH | WASI_ALLOW_DIR | WASI_ALLOW_PROC | WASI_ALLOW_POLL)




void wasi_lux9_init_context(wasi_context_t *ctx, Proc *p);
void wasi_lux9_destroy_context(wasi_context_t *ctx);


M3Result LinkWasi(IM3Module module, u32int allow_mask);
# 42 "wasm/wasm_runtime.c" 2



typedef struct WasmRuntime {
  Lock lock;


  struct {
    u64int total_calls;
    u64int total_modules;
    u64int active_instances;
    u64int errors;
  } stats;
} WasmRuntime;

#define WASM_MAX_MODULE_BYTES (16 * 1024 * 1024)
#define WASM_MAX_LINEAR_BYTES (64 * 1024 * 1024)
#define WASM_MAX_FUNC_NAME 255
#define WASM_MAX_ARGS 32
#define WASM_ARG_BYTES 8
#define WASM_LINEAR_GUARD (2 * BY2PG)
#define WASM_LINEAR_SLOTS 16
#define WASM_LINEAR_SLOT_BYTES (WASM_MAX_LINEAR_BYTES + (4 * BY2PG))
#define WASM_HEAP_BYTES (2 * 1024 * 1024)
#define WASM_HEAP_GUARD (2 * BY2PG)
#define WASM_HEAP_ALIGN 16

static WasmRuntime wasm_runtime;
static int runtime_initialized = 0;
static uchar wasm_compile_reply[8];
static uchar wasm_execute_reply[8];


static int wasm_safe_channel_close(Chan **c) {
  if (c && *c) {

    if ((*c)->ref <= 0) {
      print("wasm_safe_channel_close: warning - channel %p already freed\n",
            *c);
      *c = ((void *)0);
      return 0;
    }
    cclose(*c);
    *c = ((void *)0);
    return 1;
  }
  return 0;
}

static int wasm_heap_init(Proc *p);
void *wasm_heap_alloc(size_t size);
void wasm_heap_free(void *ptr);
extern void *memcpy(void *dst, const void *src, size_t n);






int wasm_exec_compile(Chan *tc, IM3Function *out_start) {
  int branch_inited = 0;
  PebbleState *ps;
  u8int *module_bytes = ((void *)0);
  u32int module_size = 0;
  vlong file_size;

  if (!runtime_initialized) {
    print("wasm_exec_compile: runtime not initialized\n");
    return -1;
  }


  file_size = devtab[tc->type]->read(tc, ((void *)0), 0, 0);
  print("wasm_exec_compile: determining file size\n");
  if (file_size <= 0) {
    file_size = 64 * 1024;
  }


  if (up->pid <= 2 && file_size > 512 * 1024) {
    file_size = 512 * 1024;
  }
  print("wasm_exec_compile: size=%lld\n", file_size);

  if (file_size > (16 * 1024 * 1024)) {
    print("wasm_exec_compile: module too large (%lld bytes)\n", file_size);
    return -1;
  }

  module_bytes = malloc(file_size);
  if (module_bytes == ((void *)0)) {
    print("wasm_exec_compile: failed to allocate module buffer\n");
    return -1;
  }


  long wasmerr = devtab[tc->type]->read(tc, module_bytes, file_size, 0);
  if (wasmerr < 0) {
    print("wasm_exec_compile: failed to read file\n");
    free(module_bytes);
    return -1;
  }
  module_size = wasmerr;

  if (wasm_heap_init(up) < 0) {
    free(module_bytes);
    return -1;
  }

  up->wasm.env = m3_NewEnvironment();
  up->wasm.runtime =
      m3_NewRuntime((IM3Environment)up->wasm.env, 64 * 1024, ((void *)0));
  if (up->wasm.runtime == ((void *)0)) {
    print("wasm_exec_compile: failed to create runtime\n");
    free(module_bytes);

    if (up->wasm.env) {
      m3_FreeEnvironment((IM3Environment)up->wasm.env);
      up->wasm.env = ((void *)0);
    }
    return -1;
  }


  IM3Module module;
  M3Result result = m3_ParseModule((IM3Environment)up->wasm.env, &module,
                                   module_bytes, module_size);
  if (result) {
    print("wasm_exec_compile: parse failed: %s\n", result);
    goto fail_compile;
  }
  up->wasm.module = (void *)module;


  result = m3_LoadModule((IM3Runtime)up->wasm.runtime, module);
  if (result) {
    print("wasm_exec_compile: load failed: %s\n", result);
    goto fail_compile;
  }


  u32int mem_size = 0;
  up->wasm.linear_memory =
      m3_GetMemory((IM3Runtime)up->wasm.runtime, &mem_size, 0);
  up->wasm.memory_size = mem_size;
  if (up->wasm.linear_memory) {
    print("wasm_exec_compile: linear memory at %p, size=%u bytes\n",
          up->wasm.linear_memory, mem_size);
  } else {
    print("wasm_exec_compile: no linear memory (size=%u)\n", mem_size);
  }



  up->wasm.wasi_ctx = wasm_heap_alloc(sizeof(wasi_context_t));
  if (up->wasm.wasi_ctx) {
    print("wasm_exec_compile: initializing WASI context at %p\n",
          up->wasm.wasi_ctx);
    wasi_lux9_init_context((wasi_context_t *)up->wasm.wasi_ctx, up);


    wasi_context_t *ctx = (wasi_context_t *)up->wasm.wasi_ctx;
    if (!ctx) {
      print("wasm_exec_compile: ERROR - WASI context is null after init\n");
      wasm_heap_free(up->wasm.wasi_ctx);
      up->wasm.wasi_ctx = ((void *)0);
    } else {

      if (!ctx->fds[0].is_open || !ctx->fds[1].is_open ||
          !ctx->fds[2].is_open) {
        print("wasm_exec_compile: warning - stdio FDs not properly "
              "initialized\n");

      }


      print("wasm_exec_compile: linking WASI functions with mask 0x%08x\n",
            (0x00000001u | 0x00000002u | 0x00000004u | 0x00000008u | 0x00000010u | 0x00000020u | 0x00000040u | 0x00000080u));
      result = LinkWasi(module, (0x00000001u | 0x00000002u | 0x00000004u | 0x00000008u | 0x00000010u | 0x00000020u | 0x00000040u | 0x00000080u));
      if (result) {
        print("wasm_exec_compile: WASI link warning: %s (continuing...)\n",
              result);

      } else {
        print("wasm_exec_compile: WASI functions linked successfully\n");
      }


      print("wasm_exec_compile: linking Lux9 host functions\n");
      result = LinkLux9(module);
      if (result) {
        print("wasm_exec_compile: Lux9 link warning: %s (continuing...)\n",
              result);

      } else {
        print("wasm_exec_compile: Lux9 host functions linked successfully\n");
      }
    }
  } else {
    print("wasm_exec_compile: WARNING - failed to allocate WASI context\n");

  }


  print("wasm_exec_compile: compiling module\n");
  result = m3_CompileModule(module);
  if (result) {
    print("wasm_exec_compile: compile failed: %s\n", result);
    goto fail_compile;
  }


  IM3Function start_func;
  result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "_start");
  if (result) {
    print("wasm_exec_compile: _start not found, trying main\n");
    result = m3_FindFunction(&start_func, (IM3Runtime)up->wasm.runtime, "main");
    if (result) {
      print("wasm_exec_compile: no entry point (_start or main): %s\n", result);
      goto fail_compile;
    }
  }

  print("wasm_exec_compile: success, entry=%p\n", start_func);

  if (out_start)
    *out_start = start_func;


  if (up->wasm.linear_memory && up->wasm.memory_size > 0) {
    volatile u8int first = up->wasm.linear_memory[0];
    volatile u8int last = up->wasm.linear_memory[up->wasm.memory_size - 1];
    print("wasm_exec_compile: linear memory validated [%p-%p]\n",
          up->wasm.linear_memory,
          up->wasm.linear_memory + up->wasm.memory_size);
  } else {
    print("wasm_exec_compile: WARNING - no linear memory mapped\n");
  }
# 294 "wasm/wasm_runtime.c"
  return 0;

fail_compile:
  if (module_bytes)
    free(module_bytes);


  return -1;
}


static const char *wasm_trap_label(M3Result result);





void wasm_exec_run(IM3Function start_func) {
  if (!start_func) {
    print("wasm_exec_run: invalid start_func\n");
    pexit("wasm invalid start", 1);
  }

  print("wasm_exec_run: executing entry point for pid=%lu\n", up->pid);


  M3Result result = m3_CallV(start_func);


  if (result && strcmp(result, m3Err_trapExit) == 0) {
    u32int exit_code = 0;
    if (up->wasm.wasi_ctx) {
      exit_code = ((wasi_context_t *)up->wasm.wasi_ctx)->exit_code;
    }
    print("wasm_exec_run: pid=%lu exited with code %u\n", up->pid, exit_code);
    if (exit_code == 0) {
      pexit(((void *)0), 0);
    } else {
      char exit_msg[32];
      snprint(exit_msg, sizeof(exit_msg), "exit code %u", exit_code);
      pexit(exit_msg, 1);
    }
  }


  if (result) {
    const char *trap_label = wasm_trap_label(result);
    if (trap_label) {
      print("wasm_exec_run: pid=%lu trapped: %s\n", up->pid, trap_label);
      pexit(trap_label, 1);
    } else {
      print("wasm_exec_run: pid=%lu error: %s\n", up->pid, result);
      pexit(result, 1);
    }
  }


  print("wasm_exec_run: pid=%lu completed successfully\n", up->pid);
  pexit(((void *)0), 0);
}






static void wasm_unmap_linear_memory(Proc *p);

typedef struct WasmHeapBlock {
  u32int size;
  u8int free;
  u8int pad[3];
  struct WasmHeapBlock *next;
} WasmHeapBlock;

#define WASM_HEAP_HDR_SIZE ROUNDUP(sizeof(WasmHeapBlock), WASM_HEAP_ALIGN)

static uintptr wasm_linear_base(Proc *p, u32int map_bytes) {
  uintptr region_size = 16 * ((64 * 1024 * 1024) + (4 * (0x1000ull)));
  uintptr base = (0x00007ffffffff000ull) - (16 * 1048576u) - region_size;
  uintptr slot = 0;

  if (p)
    slot = (uintptr)(p->pid % 16);

  base += slot * ((64 * 1024 * 1024) + (4 * (0x1000ull))) + (2 * (0x1000ull));
  if (base < (0x0000000000200000ull) || map_bytes > (64 * 1024 * 1024))
    return 0;
  return base;
}





static const char *wasm_trap_label(M3Result result) {
  if (!result)
    return ((void *)0);
  if (strcmp(result, m3Err_trapOutOfBoundsMemoryAccess) == 0)
    return "out of bounds memory access";
  if (strcmp(result, m3Err_trapDivisionByZero) == 0)
    return "division by zero";
  if (strcmp(result, m3Err_trapIntegerOverflow) == 0)
    return "integer overflow";
  if (strcmp(result, m3Err_trapIntegerConversion) == 0)
    return "invalid integer conversion";
  if (strcmp(result, m3Err_trapIndirectCallTypeMismatch) == 0)
    return "indirect call type mismatch";
  if (strcmp(result, m3Err_trapTableIndexOutOfRange) == 0)
    return "table index out of range";
  if (strcmp(result, m3Err_trapTableElementIsNull) == 0)
    return "null table element";
  if (strcmp(result, m3Err_trapUnreachable) == 0)
    return "unreachable executed";
  if (strcmp(result, m3Err_trapStackOverflow) == 0)
    return "stack overflow";
  if (strcmp(result, m3Err_trapAbort) == 0)
    return "abort";
  return ((void *)0);
}

static int wasm_heap_init(Proc *p) {
  if (!p)
    return -1;
  if (p->wasm.heap_base != ((void *)0))
    return 0;

  uintptr lin_base = wasm_linear_base(p, (64 * 1024 * 1024));
  if (lin_base == 0)
    return -1;
  uintptr heap_base = lin_base - (2 * (0x1000ull)) - (2 * 1024 * 1024);
  heap_base = (((heap_base) + (((0x1000ull)) - 1)) & ~(((0x1000ull)) - 1));
  if (heap_base < (0x0000000000200000ull))
    return -1;

  ulong heap_pages = (2 * 1024 * 1024) / (0x1000ull);
  Segment *h = newseg(SG_BSS | SG_NOEXEC | SG_WASM, heap_base, heap_pages);
  if (h == ((void *)0))
    return -1;

  if (p->seg[SEG4]) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = ((void *)0);
  }
  p->seg[SEG4] = h;


  arena_branch_init(&p->wasm.branch, pebble_state(), (2 * 1024 * 1024));

  p->wasm.heap_base = (u8int *)heap_base;
  p->wasm.heap_size = (2 * 1024 * 1024);
  p->wasm.heap_used = 0;
  p->wasm.heap_head = ((void *)0);
  p->wasm.heap_live = 0;
  p->wasm.linear_charged = 0;
  p->wasm.initialized = 1;
  return 0;
}

static void wasm_heap_destroy(Proc *p) {
  if (!p)
    return;
  if (p->wasm.heap_live > 0)
    arena_branch_free(&p->wasm.branch, p->wasm.heap_live);
  if (p->seg[SEG4] && (p->seg[SEG4]->type & SG_WASM) != 0) {
    putseg(p->seg[SEG4]);
    p->seg[SEG4] = ((void *)0);
  }
  p->wasm.heap_base = ((void *)0);
  p->wasm.heap_size = 0;
  p->wasm.heap_used = 0;
  p->wasm.heap_head = ((void *)0);
  p->wasm.heap_live = 0;
}

static int wasm_heap_contains(Proc *p, void *ptr) {
  if (!p || p->wasm.heap_base == ((void *)0) || ptr == ((void *)0))
    return 0;
  return (u8int *)ptr >= p->wasm.heap_base &&
         (u8int *)ptr < p->wasm.heap_base + p->wasm.heap_size;
}

static int wasm_charge_linear(Proc *p, u32int new_size) {
  if (!p)
    return -1;

  u32int old_size = p->wasm.linear_charged;
  if (p->wasm.branch.max_tokens > 0) {
    u64int cap = (u64int)p->wasm.branch.max_tokens;
    if ((u64int)new_size + (u64int)p->wasm.heap_live > cap) {
      print("wasm_charge_linear: FAIL max_tokens check. new=%lud live=%lud "
            "cap=%lud\n",
            (ulong)new_size, (ulong)p->wasm.heap_live, (ulong)cap);
      return -1;
    }
  }
  if (new_size == old_size)
    return 0;

  if (new_size > old_size) {
    u32int delta = new_size - old_size;
    print("wasm_charge_linear: calling alloc delta=%lud\n", (ulong)delta);
    if (arena_branch_alloc(&p->wasm.branch, delta) < 0) {
      print("wasm_charge_linear: arena_branch_alloc failed for delta=%lud\n",
            (ulong)delta);
      return -1;
    }
  } else {
    u32int delta = old_size - new_size;
    arena_branch_free(&p->wasm.branch, delta);
  }

  p->wasm.linear_charged = new_size;
  return 0;
}

int wasm_linear_charge_reserve(uint32_t new_size, uint32_t old_size) {
  Proc *p = up;
  print("wasm_linear_charge_reserve: new=%lud old=%lud p=%p init=%d\n",
        (ulong)new_size, (ulong)old_size, p, p ? p->wasm.initialized : 0);
  if (!p || !p->wasm.initialized)
    return -1;
  if (wasm_charge_linear(p, new_size) != 0)
    return -1;
  p->wasm.memory_size = new_size;
  p->wasm.memory_pages = new_size / (64 * 1024);
  return 0;
}

void wasm_linear_charge_rollback(uint32_t old_size) {
  Proc *p = up;
  if (!p || !p->wasm.initialized)
    return;
  wasm_charge_linear(p, old_size);
  p->wasm.memory_size = old_size;
  p->wasm.memory_pages = old_size / (64 * 1024);
}

static WasmHeapBlock *wasm_heap_block_from_ptr(Proc *p, void *ptr) {
  if (!wasm_heap_contains(p, ptr))
    return ((void *)0);
  return (WasmHeapBlock *)((u8int *)ptr - (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)));
}

void *wasm_heap_alloc(size_t size) {
  Proc *p = up;
  if (p == ((void *)0) || !p->wasm.initialized)
    return ((void *)0);
  if (p->wasm.heap_base == ((void *)0))
    return ((void *)0);

  size = (((size) + ((16) - 1)) & ~((16) - 1));
  if (size == 0)
    size = 16;
  if (p->wasm.branch.max_tokens > 0) {
    u64int cap = (u64int)p->wasm.branch.max_tokens;
    if ((u64int)size + (u64int)p->wasm.heap_live +
            (u64int)p->wasm.linear_charged >
        cap)
      return ((void *)0);
  }

  WasmHeapBlock *prev = ((void *)0);
  WasmHeapBlock *cur = (WasmHeapBlock *)p->wasm.heap_head;
  while (cur) {
    if (cur->free && cur->size >= size)
      break;
    prev = cur;
    cur = cur->next;
  }

  if (cur) {
    if (arena_branch_alloc(&p->wasm.branch, cur->size) < 0)
      return ((void *)0);
    cur->free = 0;
    p->wasm.heap_live += cur->size;
    void *ptr = (u8int *)cur + (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1));
    memset(ptr, 0, cur->size);
    return ptr;
  }

  u32int used = (((p->wasm.heap_used) + ((16) - 1)) & ~((16) - 1));
  u32int need = (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + (u32int)size;
  if (pebble_debug)
    print("wasm_heap_alloc: size=%lud used=%d need=%d heap_size=%lud\n", size,
          used, need, p->wasm.heap_size);

  if (used + need > p->wasm.heap_size) {
    if (pebble_debug)
      print("wasm_heap_alloc: failed heap_size check\n");
    return ((void *)0);
  }
  if (arena_branch_alloc(&p->wasm.branch, size) < 0) {
    if (pebble_debug)
      print("wasm_heap_alloc: failed arena_branch_alloc\n");
    return ((void *)0);
  }

  WasmHeapBlock *blk = (WasmHeapBlock *)(p->wasm.heap_base + used);
  blk->size = (u32int)size;
  blk->free = 0;
  blk->next = ((void *)0);
  if (prev)
    prev->next = blk;
  else
    p->wasm.heap_head = blk;

  p->wasm.heap_used = used + need;
  p->wasm.heap_live += (u32int)size;

  void *ptr = (u8int *)blk + (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1));
  memset(ptr, 0, size);
  if (pebble_debug)
    print("wasm_heap_alloc: success ptr=%p\n", ptr);
  return ptr;
}

void wasm_heap_free(void *ptr) {
  Proc *p = up;
  WasmHeapBlock *blk = wasm_heap_block_from_ptr(p, ptr);
  if (!blk || blk->free)
    return;

  blk->free = 1;
  if (p->wasm.heap_live >= blk->size)
    p->wasm.heap_live -= blk->size;
  arena_branch_free(&p->wasm.branch, blk->size);


  WasmHeapBlock *next = blk->next;
  if (next && next->free &&
      (u8int *)blk + (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + blk->size == (u8int *)next) {
    blk->size += (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + next->size;
    blk->next = next->next;
  }


  WasmHeapBlock *prev = ((void *)0);
  WasmHeapBlock *cur = (WasmHeapBlock *)p->wasm.heap_head;
  while (cur && cur != blk) {
    prev = cur;
    cur = cur->next;
  }
  if (prev && prev->free &&
      (u8int *)prev + (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + prev->size == (u8int *)blk) {
    prev->size += (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + blk->size;
    prev->next = blk->next;
  }
}

void *wasm_heap_realloc(void *ptr, size_t new_size, size_t old_size) {
  if (__builtin_expect(!!(new_size == old_size), 0))
    return ptr;

  Proc *p = up;
  if (ptr && !wasm_heap_contains(p, ptr))
    return ((void *)0);
  if (ptr) {
    WasmHeapBlock *blk = wasm_heap_block_from_ptr(p, ptr);
    if (blk && new_size <= blk->size)
      return ptr;
    if (blk) {
      u32int needed = (u32int)(((new_size) + ((16) - 1)) & ~((16) - 1));
      if (needed > blk->size) {
        u32int delta = needed - blk->size;
        u8int *blk_end = (u8int *)blk + (((sizeof(WasmHeapBlock)) + ((16) - 1)) & ~((16) - 1)) + blk->size;
        u8int *heap_end = p->wasm.heap_base + p->wasm.heap_used;
        if (!blk->free && blk_end == heap_end &&
            (p->wasm.heap_used + delta) <= p->wasm.heap_size) {
          if (p->wasm.branch.max_tokens > 0) {
            u64int cap = (u64int)p->wasm.branch.max_tokens;
            if ((u64int)delta + (u64int)p->wasm.heap_live +
                    (u64int)p->wasm.linear_charged >
                cap)
              return ((void *)0);
          }
          if (arena_branch_alloc(&p->wasm.branch, delta) == 0) {
            blk->size = needed;
            p->wasm.heap_used += delta;
            p->wasm.heap_live += delta;
            memset(blk_end, 0, delta);
            return ptr;
          }
        }
      }
    }
  }

  void *new_ptr = wasm_heap_alloc(new_size);
  if (new_ptr == ((void *)0))
    return ((void *)0);
  if (ptr) {
    size_t copy = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, ptr, copy);
    wasm_heap_free(ptr);
  }
  return new_ptr;
}
static int wasm_map_guard(Proc *p, int segidx, uintptr base, ulong pages) {
  if (!p || pages == 0)
    return 0;
  if (p->seg[segidx]) {
    if ((p->seg[segidx]->type & SG_WASM) == 0)
      return -1;
    putseg(p->seg[segidx]);
    p->seg[segidx] = ((void *)0);
  }
  Segment *g = newseg(SG_BSS | SG_FAULT | SG_NOEXEC | SG_WASM, base, pages);
  if (g == ((void *)0))
    return -1;
  p->seg[segidx] = g;
  return 0;
}

static void wasm_clear_guard(Proc *p, int segidx) {
  if (!p || !p->seg[segidx])
    return;
  if ((p->seg[segidx]->type & SG_WASM) == 0)
    return;
  putseg(p->seg[segidx]);
  p->seg[segidx] = ((void *)0);
}

static int wasm_map_linear_memory(Proc *p) {
  extern uintptr saved_limine_hhdm_offset;
  if (!p || !p->wasm.linear_memory || p->wasm.memory_size == 0) {
    print("wasm_map_linear_memory: invalid params p=%p mem=%p size=%u\n", p,
          p ? p->wasm.linear_memory : 0, p ? p->wasm.memory_size : 0);
    return -1;
  }

  uintptr lin_ptr = (uintptr)p->wasm.linear_memory;
  uintptr offset = lin_ptr & ((0x1000ull) - 1);
  uintptr base_ptr = lin_ptr - offset;
  u32int total_bytes = p->wasm.memory_size + (u32int)offset;
  u32int map_bytes = (((total_bytes) + (((0x1000ull)) - 1)) & ~(((0x1000ull)) - 1));
  ulong map_pages = map_bytes / (0x1000ull);

  if (map_bytes > (64 * 1024 * 1024)) {
    print("wasm_map_linear_memory: map_bytes too large %u\n", map_bytes);
    return -1;
  }

  if (base_ptr < saved_limine_hhdm_offset ||
      base_ptr + map_bytes > saved_limine_hhdm_offset + (256ULL * 1073741824u)) {
    print(
        "wasm_map_linear_memory: address out of HHDM range base=%p limit=%p\n",
        base_ptr, saved_limine_hhdm_offset);
    return -1;
  }

  uintptr phys_base = ((uintptr)((void *)base_ptr) - (0xffffffff80000000ull));
  for (ulong i = 0; i < map_pages; i++) {
    uintptr va = base_ptr + (i * (0x1000ull));
    if (((uintptr)((void *)va) - (0xffffffff80000000ull)) != phys_base + (i * (0x1000ull))) {
      print("wasm_map_linear_memory: non-contiguous physical memory at i=%lu\n",
            i);
      return -1;
    }
  }

  if (p->seg[LSEG]) {
    if (p->seg[LSEG]->pseg) {
      free(p->seg[LSEG]->pseg);
      p->seg[LSEG]->pseg = ((void *)0);
    }
    putseg(p->seg[LSEG]);
    p->seg[LSEG] = ((void *)0);
  }
  wasm_clear_guard(p, SEG2);
  wasm_clear_guard(p, SEG3);

  uintptr base = wasm_linear_base(p, map_bytes);
  if (base == 0) {
    print("wasm_map_linear_memory: wasm_linear_base returned 0\n");
    return -1;
  }

  Segment *s =
      newseg(SG_PHYSICAL | SG_NOEXEC | SG_WASM, base + offset, map_pages);
  if (s == ((void *)0)) {
    print("wasm_map_linear_memory: newseg failed\n");
    return -1;
  }

  s->pseg = malloc(sizeof(Physseg));
  if (s->pseg == ((void *)0)) {
    print("wasm_map_linear_memory: malloc pseg failed\n");
    putseg(s);
    return -1;
  }

  s->pseg->attr = SG_PHYSICAL | SG_CACHED;
  s->pseg->name = "wasmlinear";
  s->pseg->pa = phys_base;
  s->pseg->size = map_bytes;
  s->pseg->next = ((void *)0);
  s->pseg->prev = ((void *)0);

  p->seg[LSEG] = s;

  uintptr guard_low = (base + offset) - (2 * (0x1000ull));
  uintptr guard_high = (base + offset) + map_pages * (0x1000ull);
  ulong guard_pages = (2 * (0x1000ull)) / (0x1000ull);
  if (guard_low < (0x0000000000200000ull)) {
    print("wasm_map_linear_memory: guard_low too low\n");
    return -1;
  }
  if (p->seg[SEG4] &&
      guard_low <
          p->seg[SEG4]
              ->top) {





  }
  if (guard_high + (2 * (0x1000ull)) > (0x00007ffffffff000ull)) {
    print("wasm_map_linear_memory: guard_high too high\n");
    return -1;
  }
  if (wasm_map_guard(p, SEG2, guard_low, guard_pages) < 0) {
    print("wasm_map_linear_memory: map guard low failed\n");
    wasm_unmap_linear_memory(p);
    return -1;
  }
  if (wasm_map_guard(p, SEG3, guard_high, guard_pages) < 0) {
    print("wasm_map_linear_memory: map guard high failed\n");
    wasm_unmap_linear_memory(p);
    return -1;
  }
  return 0;
}

static int wasm_refresh_linear_mapping(Proc *p) {
  uint32_t new_size = 0;
  uint8_t *new_mem;

  if (!p || !p->wasm.runtime)
    return -1;

  new_mem = m3_GetMemory((IM3Runtime)p->wasm.runtime, &new_size, 0);
  if (new_mem == ((void *)0) || new_size == 0) {
    if (p->wasm.linear_charged > 0)
      wasm_charge_linear(p, 0);
    if (p->wasm.linear_memory != ((void *)0))
      wasm_unmap_linear_memory(p);
    p->wasm.linear_memory = ((void *)0);
    p->wasm.memory_size = 0;
    p->wasm.memory_pages = 0;
    return 0;
  }

  if (new_size > (64 * 1024 * 1024))
    return -1;

  if (new_mem != p->wasm.linear_memory || new_size != p->wasm.memory_size) {
    u32int old_size = p->wasm.memory_size;
    u32int old_pages = p->wasm.memory_pages;
    u8int *old_mem = p->wasm.linear_memory;
    int charged = 0;

    p->wasm.linear_memory = new_mem;
    p->wasm.memory_size = new_size;
    p->wasm.memory_pages = new_size / (64 * 1024);
    if (!wasm_heap_contains(p, new_mem) && p->wasm.linear_charged != new_size) {
      if (wasm_charge_linear(p, new_size) != 0) {
        p->wasm.linear_memory = old_mem;
        p->wasm.memory_size = old_size;
        p->wasm.memory_pages = old_pages;
        return -1;
      }
      charged = 1;
    }
    if (wasm_map_linear_memory(p) != 0) {
      if (charged)
        wasm_charge_linear(p, old_size);
      p->wasm.linear_memory = old_mem;
      p->wasm.memory_size = old_size;
      p->wasm.memory_pages = old_pages;
      return -1;
    }
  }

  return 0;
}

static void wasm_unmap_linear_memory(Proc *p) {
  if (!p || !p->seg[LSEG])
    return;
  wasm_clear_guard(p, SEG2);
  wasm_clear_guard(p, SEG3);
  if (p->seg[LSEG]->pseg) {
    free(p->seg[LSEG]->pseg);
    p->seg[LSEG]->pseg = ((void *)0);
  }
  putseg(p->seg[LSEG]);
  p->seg[LSEG] = ((void *)0);
}

void wasm_runtime_cleanup_process(Proc *p) {
  if (p->wasm.runtime) {

    m3_FreeRuntime((IM3Runtime)p->wasm.runtime);
  }
  p->wasm.runtime = ((void *)0);
  p->wasm.module = ((void *)0);
  if (p->wasm.env) {
    m3_FreeEnvironment((IM3Environment)p->wasm.env);
    p->wasm.env = ((void *)0);
  }
  if (p->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)p->wasm.wasi_ctx);
    wasm_heap_free(p->wasm.wasi_ctx);
    p->wasm.wasi_ctx = ((void *)0);
  }
  if (p->wasm.module_bytes) {
    free(p->wasm.module_bytes);
    p->wasm.module_bytes = ((void *)0);
    p->wasm.module_bytes_len = 0;
  }
  wasm_heap_destroy(p);
  wasm_unmap_linear_memory(p);
  if (p->wasm.linear_charged > 0) {
    arena_branch_free(&p->wasm.branch, p->wasm.linear_charged);
    p->wasm.linear_charged = 0;
  }
  p->wasm.linear_memory = ((void *)0);
  p->wasm.memory_size = 0;
  p->wasm.memory_pages = 0;
}
# 942 "wasm/wasm_runtime.c"
void wasm_runtime_init(void) {
  if (runtime_initialized) {
    print("wasm_runtime: already initialized\n");
    return;
  }

  print("wasm_runtime: initializing isolated wasm3 runtime (Layer 1)\n");







  memset(&wasm_runtime.stats, 0, sizeof(wasm_runtime.stats));

  runtime_initialized = 1;
  print("wasm_runtime: initialization complete (Layer 1 ready)\n");
}



int sys_wasm_compile(Fcall *tx, Fcall *rx) {
  print("WASM: sys_wasm_compile called (scount=%u)\n", tx->scount);
  int branch_inited = 0;
  PebbleState *ps;
  Chan *c = ((void *)0);
  u32int module_size = 0;
  u8int *module_bytes = ((void *)0);
  Dir d;
  uchar statbuf[256];
  int n;
  long rn;

  if (!runtime_initialized) {
    print("DEBUG: sys_wasm_compile: runtime not initialized\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }
  print("DEBUG: sys_wasm_compile: runtime init ok\n");


  if (!(up->capabilities & (1 << 16))) {
    print("DEBUG: sys_wasm_compile: no permission\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "no WASM compile permission");
    wasm_runtime.stats.errors++;
    return -1;
  }
  print("DEBUG: sys_wasm_compile: permission ok\n");


  if (up->wasm.initialized) {
    print("DEBUG: sys_wasm_compile: already initialized\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename),
            "WASM already compiled for this process");
    wasm_runtime.stats.errors++;
    return -1;
  }


  if (!tx->sdata || tx->scount < 4) {
    print("DEBUG: sys_wasm_compile: invalid payload\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename),
            "invalid compile payload (expecting FD)");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u32int fd = (((uchar *)(tx->sdata))[0] | (((uchar *)(tx->sdata))[1] << 8) | (((uchar *)(tx->sdata))[2] << 16) | (((uchar *)(tx->sdata))[3] << 24));
  print("DEBUG: sys_wasm_compile: fd=%d\n", fd);


  if (setlabel(&up->errlab[up->nerrlab++])) {
    print("DEBUG: sys_wasm_compile: error during IO: %s\n", up->errstr);
    wasm_safe_channel_close(&c);
    if (module_bytes)
      free(module_bytes);
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "compile IO failed: %s", up->errstr);
    wasm_runtime.stats.errors++;
    return -1;
  }


  c = fdtochan((int)fd, 0, 0, 1);
  print("DEBUG: sys_wasm_compile: fd resolved to chan\n");


  n = devtab[c->type]->stat(c, statbuf, sizeof(statbuf));
  if (n <= 0)
    error("stat failed");

  convM2D(statbuf, (uint)n, &d, ((void *)0));
  module_size = (u32int)d.length;
  print("DEBUG: sys_wasm_compile: stat ok, size=%d\n", module_size);

  if (module_size == 0 || module_size > (16 * 1024 * 1024)) {
    error("invalid module size");
  }

  print("wasm_runtime: compile request pid=%lu fd=%d size=%u\n", up->pid, fd,
        module_size);


  module_bytes = malloc(module_size);
  if (module_bytes == ((void *)0))
    error(Enomem);
  print("DEBUG: sys_wasm_compile: malloc ok\n");


  rn = devtab[c->type]->read(c, module_bytes, module_size, 0);
  if (rn != module_size) {
    error("short read");
  }
  print("DEBUG: sys_wasm_compile: read ok\n");


  wasm_safe_channel_close(&c);
  up->nerrlab--;
  print("DEBUG: sys_wasm_compile: IO complete\n");

  ps = pebble_state();
  if (ps == ((void *)0)) {
    print("DEBUG: sys_wasm_compile: pebble state nil\n");
    free(module_bytes);
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "pebble state not initialized");
    wasm_runtime.stats.errors++;
    return -1;
  }
  print("DEBUG: sys_wasm_compile: pebble state ok\n");


  up->wasm.module_bytes = module_bytes;
  up->wasm.module_bytes_len = module_size;

  u64int branch_cap = (u64int)(64 * 1024 * 1024) + (2 * 1024 * 1024);
  if (branch_cap < (1024 * 1024))
    branch_cap = 1024 * 1024;
  ulong initial_budget = 1024 * 1024;
  if ((u64int)initial_budget > branch_cap)
    initial_budget = (ulong)branch_cap;
  arena_branch_init(&up->wasm.branch, ps, initial_budget);
  up->wasm.branch.max_tokens = (ulong)(((branch_cap) + ((8) - 1)) & ~((8) - 1));
  print("DEBUG: sys_wasm_compile: arena branch init ok\n");

  if (wasm_heap_init(up) < 0) {
    print("DEBUG: sys_wasm_compile: heap init failed\n");
    snprint(rx->ename, sizeof(rx->ename), "failed to init wasm heap");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: heap init ok\n");
  up->wasm.initialized = 1;
  branch_inited = 1;

  up->wasm.env = m3_NewEnvironment();
  if (!up->wasm.env) {
    print("DEBUG: sys_wasm_compile: env creat failed\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to create wasm3 environment");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: env created %p\n", up->wasm.env);


  up->wasm.runtime =
      m3_NewRuntime((IM3Environment)up->wasm.env, 64 * 1024, ((void *)0));
  if (!up->wasm.runtime) {
    print("DEBUG: sys_wasm_compile: runtime creat failed\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to create wasm3 runtime");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: runtime created %p\n", up->wasm.runtime);


  print("DEBUG: sys_wasm_compile: parsing module\n");
  M3Result result = m3_ParseModule(
      (IM3Environment)up->wasm.env, (IM3Module *)&up->wasm.module,
      up->wasm.module_bytes, up->wasm.module_bytes_len);
  if (result) {
    print("DEBUG: sys_wasm_compile: parse failed: %s\n", result);
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module parse failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: parse ok\n");


  print("DEBUG: sys_wasm_compile: loading module\n");
  result =
      m3_LoadModule((IM3Runtime)up->wasm.runtime, (IM3Module)up->wasm.module);
  if (result) {
    print("DEBUG: sys_wasm_compile: load failed: %s\n", result);
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "module load failed: %s", result);
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: load ok\n");

  if (wasm_refresh_linear_mapping(up) != 0) {
    print("DEBUG: sys_wasm_compile: refresh mapping failed\n");
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "failed to map linear memory");
    wasm_runtime.stats.errors++;
    goto fail_compile;
  }
  print("DEBUG: sys_wasm_compile: refresh mapping ok\n");


  print("DEBUG: sys_wasm_compile: allocating WASI context\n");
  up->wasm.wasi_ctx = wasm_heap_alloc(sizeof(wasi_context_t));
  if (!up->wasm.wasi_ctx) {
    print("wasm_runtime: failed to allocate WASI context\n");

  } else {
    print("DEBUG: sys_wasm_compile: initializing WASI context\n");
    wasi_lux9_init_context((wasi_context_t *)up->wasm.wasi_ctx, up);


    u32int allow_mask = (0x00000001u | 0x00000002u | 0x00000004u | 0x00000008u | 0x00000010u | 0x00000020u | 0x00000040u | 0x00000080u);
    if (up->capabilities & (1 << 18))
      allow_mask |= (0x00000100u | 0x00000080u);

    print("DEBUG: sys_wasm_compile: Linking WASI\n");
    M3Result link_res = LinkWasi((IM3Module)up->wasm.module, allow_mask);
    if (link_res) {
      print("wasm_runtime: WASI link warning: %s\n", link_res);
    }

    print("DEBUG: sys_wasm_compile: Linking Lux9\n");
    M3Result lux_res = LinkLux9((IM3Module)up->wasm.module);
    if (lux_res) {
      print("wasm_runtime: lux9 link warning: %s\n", lux_res);
    }
  }
  print("DEBUG: sys_wasm_compile: linking complete\n");

  wasm_runtime.stats.total_modules++;
  wasm_runtime.stats.active_instances++;

  print("wasm_runtime: compiled pid=%lu memory=%u bytes (%u pages)\n", up->pid,
        up->wasm.memory_size, up->wasm.memory_pages);


  rx->type = Rsyscall;
  rx->tag = tx->tag;

  rx->retval = up->pid;
  rx->scount = 0;
  rx->sdata = ((void *)0);
  print("DEBUG: sys_wasm_compile: returning success pid=%lu\n", up->pid);
  return 0;

fail_compile:
  if (up->wasm.wasi_ctx) {
    wasi_lux9_destroy_context((wasi_context_t *)up->wasm.wasi_ctx);
    wasm_heap_free(up->wasm.wasi_ctx);
    up->wasm.wasi_ctx = ((void *)0);
  }
  if (up->wasm.module_bytes) {
    free(up->wasm.module_bytes);
    up->wasm.module_bytes = ((void *)0);
    up->wasm.module_bytes_len = 0;
  }
  wasm_heap_destroy(up);
  wasm_unmap_linear_memory(up);
  if (up->wasm.runtime) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = ((void *)0);
    up->wasm.module = ((void *)0);
  }
  if (up->wasm.env) {
    m3_FreeEnvironment((IM3Environment)up->wasm.env);
    up->wasm.env = ((void *)0);
  }
  up->wasm.linear_memory = ((void *)0);
  up->wasm.memory_size = 0;
  up->wasm.memory_pages = 0;
  if (branch_inited)
    arena_branch_drain(&up->wasm.branch);
  up->wasm.initialized = 0;
  return -1;
}





int sys_wasm_execute(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }


  if (!up->wasm.initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "not a WASM process");
    wasm_runtime.stats.errors++;
    return -1;
  }


  if (!(up->capabilities & (1 << 17))) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "no WASM execute permission");
    wasm_runtime.stats.errors++;
    return -1;
  }


  if (!tx->sdata || tx->scount < 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid execute payload");
    wasm_runtime.stats.errors++;
    return -1;
  }
  u8int *sdata = (u8int *)tx->sdata;
  u32int func_name_len = 0;
  memmove(&func_name_len, sdata, sizeof(func_name_len));
  if (func_name_len == 0 || func_name_len > 255 ||
      func_name_len > tx->scount - 4) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid function name size");
    wasm_runtime.stats.errors++;
    return -1;
  }
  char *func_name = (char *)(sdata + 4);


  char func_name_buf[256];
  if (func_name_len >= sizeof(func_name_buf)) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "function name too long");
    wasm_runtime.stats.errors++;
    return -1;
  }
  memmove(func_name_buf, func_name, func_name_len);
  func_name_buf[func_name_len] = '\0';

  print("wasm_runtime: execute pid=%lu func='%s'\n", up->pid, func_name_buf);
  uint32_t mem_size = 0;
  void *mem_ptr = m3_GetMemory((IM3Runtime)up->wasm.runtime, &mem_size, 0);
  print("wasm_runtime: runtime=%p memory=%p size=%u\n", up->wasm.runtime,
        mem_ptr, mem_size);


  IM3Function func;
  M3Result result =
      m3_FindFunction(&func, (IM3Runtime)up->wasm.runtime, func_name_buf);
  if (result) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "function not found: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }


  u32int expected_argc = m3_GetArgCount(func);
  u32int argc = 0;
  u8int *argp = sdata + 4 + func_name_len;
  u32int remaining = tx->scount - 4 - func_name_len;

  if (remaining >= 4) {
    memmove(&argc, argp, sizeof(argc));
    argp += 4;
    remaining -= 4;
  }

  if (argc > 32 || remaining < argc * 8) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "invalid args payload");
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (argc != expected_argc) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "arg count mismatch");
    wasm_runtime.stats.errors++;
    return -1;
  }

  union ArgValue {
    u32int i32;
    u64int i64;
    float f32;
    double f64;
  } arg_vals[32];
  void *arg_ptrs[32];

  for (u32int i = 0; i < argc; i++) {
    u64int raw = 0;
    memmove(&raw, argp + (i * 8), sizeof(raw));
    M3ValueType type = m3_GetArgType(func, i);
    switch (type) {
    case c_m3Type_i32:
      arg_vals[i].i32 = (u32int)raw;
      arg_ptrs[i] = &arg_vals[i].i32;
      break;
    case c_m3Type_i64:
      arg_vals[i].i64 = raw;
      arg_ptrs[i] = &arg_vals[i].i64;
      break;
    case c_m3Type_f32: {
      u32int bits = (u32int)raw;
      memmove(&arg_vals[i].f32, &bits, sizeof(bits));
      arg_ptrs[i] = &arg_vals[i].f32;
      break;
    }
    case c_m3Type_f64:
      memmove(&arg_vals[i].f64, &raw, sizeof(raw));
      arg_ptrs[i] = &arg_vals[i].f64;
      break;
    default:
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "unsupported arg type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }




  result = m3_Call(func, argc, (const void **)arg_ptrs);

  if (result) {
    if (strcmp(result, m3Err_trapExit) == 0) {
      u32int exit_code = 0;
      if (up->wasm.wasi_ctx)
        exit_code = ((wasi_context_t *)up->wasm.wasi_ctx)->exit_code;
      rx->type = Rsyscall;
      rx->tag = tx->tag;
      rx->retval = exit_code;
      rx->scount = 0;
      rx->sdata = ((void *)0);
      return 0;
    }
    const char *trap = wasm_trap_label(result);
    rx->type = Rerror;
    if (trap)
      snprint(rx->ename, sizeof(rx->ename), "trap: %s", trap);
    else
      snprint(rx->ename, sizeof(rx->ename), "execution failed: %s", result);
    wasm_runtime.stats.errors++;
    return -1;
  }

  if (wasm_refresh_linear_mapping(up) != 0) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "linear memory remap failed");
    wasm_runtime.stats.errors++;
    return -1;
  }

  wasm_runtime.stats.total_calls++;


  u32int retc = m3_GetRetCount(func);
  u64int retval = 0;
  if (retc > 1) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "multi-value returns unsupported");
    wasm_runtime.stats.errors++;
    return -1;
  }
  if (retc == 1) {
    M3ValueType rtype = m3_GetRetType(func, 0);
    union ArgValue ret_val;
    void *ret_ptrs[1] = {&ret_val};
    result = m3_GetResults(func, 1, (const void **)ret_ptrs);
    if (result) {
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "result fetch failed: %s", result);
      wasm_runtime.stats.errors++;
      return -1;
    }
    switch (rtype) {
    case c_m3Type_i32:
      retval = ret_val.i32;
      break;
    case c_m3Type_i64:
      retval = ret_val.i64;
      break;
    case c_m3Type_f32: {
      u32int bits = 0;
      memmove(&bits, &ret_val.f32, sizeof(bits));
      retval = bits;
      break;
    }
    case c_m3Type_f64:
      memmove(&retval, &ret_val.f64, sizeof(retval));
      break;
    default:
      rx->type = Rerror;
      snprint(rx->ename, sizeof(rx->ename), "unsupported return type");
      wasm_runtime.stats.errors++;
      return -1;
    }
  }

  print("wasm_runtime: executed pid=%lu func='%s' retval=%llu\n", up->pid,
        func_name_buf, retval);


  rx->type = Rsyscall;
  rx->tag = tx->tag;

  rx->retval = retval;
  rx->scount = 0;
  rx->sdata = ((void *)0);
  return 0;
}







int sys_wasm_destroy(Fcall *tx, Fcall *rx) {
  if (!runtime_initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "wasm runtime not initialized");
    return -1;
  }


  if (!up->wasm.initialized) {
    rx->type = Rerror;
    snprint(rx->ename, sizeof(rx->ename), "not a WASM process");
    return -1;
  }

  print("wasm_runtime: destroy pid=%lu\n", up->pid);


  if (up->wasm.runtime) {
    m3_FreeRuntime((IM3Runtime)up->wasm.runtime);
    up->wasm.runtime = ((void *)0);
    up->wasm.module = ((void *)0);
  }

  wasm_runtime_cleanup_process(up);


  arena_branch_drain(&up->wasm.branch);


  up->wasm.initialized = 0;

  wasm_runtime.stats.active_instances--;


  rx->type = Rsyscall;
  rx->tag = tx->tag;
  rx->retval = 0;
  rx->scount = 0;
  rx->sdata = ((void *)0);
  return 0;
}



void wasm_runtime_stats(void) {
  extern Proc *proctab(int i);
  Proc *p;

  print("WASM Runtime Statistics (Layer 1):\n");
  print("  Total calls:       %llu\n", wasm_runtime.stats.total_calls);
  print("  Total modules:     %llu\n", wasm_runtime.stats.total_modules);
  print("  Active instances:  %llu\n", wasm_runtime.stats.active_instances);
  print("  Errors:            %llu\n", wasm_runtime.stats.errors);

  print("  Per-process WASM usage:\n");
  for (int i = 0; (p = proctab(i)) != ((void *)0); i++) {
    if (!p->wasm.initialized)
      continue;
    print("    pid=%lud heap_used=%ud heap_live=%ud linear=%ud\n", p->pid,
          p->wasm.heap_used, p->wasm.heap_live, p->wasm.linear_charged);
    print("    branch local=%lud borrowed=%lud max=%lud\n",
          p->wasm.branch.local_colorless, p->wasm.branch.borrowed_from_proc,
          p->wasm.branch.max_tokens);
  }
}
