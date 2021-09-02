#include "extcode.h"
#pragma pack(push)
#pragma pack(1)

#ifdef __cplusplus
extern "C" {
#endif
typedef uint16_t  Condition;
#define Condition_ON 0
#define Condition_OFF 1
#define Condition__5V38VON 2
#define Condition__5V 3
typedef uint16_t  Equipment;
#define Equipment__66319D 0
#define Equipment__2306 1

/*!
 * PSUSettingLab
 */
int16_t __stdcall PSUSettingLab(Condition condition, Equipment equipmentID);

MgErr __cdecl LVDLLStatus(char *errStr, int errStrLen, void *module);

#ifdef __cplusplus
} // extern "C"
#endif

#pragma pack(pop)

