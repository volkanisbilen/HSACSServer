#include "stdafx.h"
#include "Explosion.h"

#include "BugTrap.h"

#pragma comment(lib, "../shared/BugTrapx64.lib")   

ExplosionHandle::ExplosionHandle()
{

}

#pragma region ConsoleColorHandle::SetupExceptionHandler()
void ExplosionHandle::SetupExceptionHandler()
{
	BT_InstallSehFilter();
	BT_SetAppName(_T("HSACS Knight Online Server Securty"));
	BT_SetSupportEMail(_T("volkan_yng@hotmail.com"));
	BT_SetFlags(BTF_DETAILEDMODE | BTF_EDITMAIL | BTF_ATTACHREPORT | BTF_SCREENCAPTURE);
	BT_SetSupportServer(_T("localhost"), 9999);
	BT_SetSupportURL(_T("http://www.japko.us/"));
}

#pragma endregion 
