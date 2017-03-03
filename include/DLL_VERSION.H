#pragma once


#define TBF_MAJOR 0
#define TBF_MINOR 10
#define TBF_BUILD 2
#define TBF_REV   2





#define _A2(a)     #a
#define  _A(a)  _A2(a)
#define _L2(w)  L ## w
#define  _L(w) _L2(w)


#if (TBF_REV > 0)
#define TBF_VERSION_STR_A    _A(TBF_MAJOR) "." _A(TBF_MINOR) "." _A(TBF_BUILD) "." _A(TBF_REV)
#else
#define TBF_VERSION_STR_A    _A(TBF_MAJOR) "." _A(TBF_MINOR) "." _A(TBF_BUILD)
#endif

#define TBF_VERSION_STR_W _L(TBF_VERSION_STR_A)


#define TBF_FILE_VERSION     TBF_MAJOR,TBF_MINOR,TBF_BUILD,TBF_REV
#define TBF_PRODUCT_VERSION  TBF_MAJOR,TBF_MINOR,TBF_BUILD,TBF_REV
