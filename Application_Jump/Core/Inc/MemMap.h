


/*----------------------------------------------------*/
#ifdef START_SEC_MY_MEMBOOTLOAD
#undef START_SEC_MY_MEMBOOTLOAD
#pragma GCC push_options
#define MY_MEMBOOTLOAD_CODE  __attribute__((section(".My_MemBootLoad")))
#endif
#ifdef STOP_SEC_MY_MEMBOOTLOAD
#undef STOP_SEC_MY_MEMBOOTLOAD 
#pragma GCC pop_options
#endif
/*----------------------------------------------------*/
