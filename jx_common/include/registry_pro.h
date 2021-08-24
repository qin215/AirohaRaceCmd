#ifndef   registry_proH
#define   registry_proH
#include "stdafx.h"

CString  ReadConfig(CString key_name);
void  SaveConfig(CString key_name,CString str);
void  DeleteConfig(CString key_name);
#endif
