#include "extcode.h"
#pragma pack(push)
#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif
typedef uint16_t  Condition;
#define Condition_ON 0
#define Condition_OFF 1

/*!
 * PSUSetting
 */
int16_t __stdcall PSUSetting(Condition condition);

MgErr __cdecl LVDLLStatus(char *errStr, int errStrLen, void *module);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma pack(pop)

