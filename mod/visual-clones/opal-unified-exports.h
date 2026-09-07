#pragma once

// Declarations for the Media and Performance objects linked into the one
// Opal DLL. The shell compilation unit calls these; without this header a
// merge that adds late-attach can fail to compile.
#ifdef OPAL_UNIFIED_BUILD
BOOL OpalMedia_ModInit();
void OpalMedia_ModAfterInit();
void OpalMedia_ModSettingsChanged();
void OpalMedia_ModBeforeUninit();
void OpalMedia_ModUninit();
BOOL OpalPerformance_ModInit();
void OpalPerformance_ModAfterInit();
void OpalPerformance_ModSettingsChanged();
void OpalPerformance_ModBeforeUninit();
void OpalPerformance_ModUninit();
bool OpalMedia_EnsureAttached();
bool OpalPerformance_EnsureAttached();
#endif
