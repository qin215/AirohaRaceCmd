========================================================================
    STATIC LIBRARY : jx_common Project Overview
========================================================================
集贤生产工具通用库（V1.0) by qinjiangwei 2018/5/9 

1. 所有的集贤上位机软件需要引用该库，保证各个算法的统一性 
/////////////////////////////////////////////////////////////////////////////

2018/12/11
1. 需要静态连接，则在project properties->configuration type->static lib
2. 动态连接，则在project properties->configuration type->dynamatic lib, 并在mywin.dll打开 DLL_EXPORT
