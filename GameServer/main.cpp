#include "stdafx.h"
#include "../shared/Condition.h"
#include "ConsoleInputThread.h"
#include "../shared/signal_handler.h"
#include "../shared/CrashHandler.h"//x

CGameServerDlg * g_pMain;
static Condition s_hEvent;

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType);

bool g_bRunning = true;

int main()
{
	SetConsoleTitle("HSACS System (GameServer) - v" STRINGIFY(GAME_SOURCE_VERSION));

	DateTime time;
	clock_t x = clock();
	CCrashHandler XxX;//x
	XxX.SetProcessExceptionHandlers();//x
	XxX.SetThreadExceptionHandlers();//x

	// Override the console handler
	SetConsoleCtrlHandler(_ConsoleHandler, TRUE);

	HookSignals(&s_hEvent);

	// Start up the time updater thread
	StartTimeThread();

	ExplosionHandle::SetupExceptionHandler(); // BugTrap 27.09.2020

	// Start up the console input thread
	StartConsoleInputThread();

	g_pMain = new CGameServerDlg();
	g_pMain->s_hEvent = &s_hEvent;

	// Start up server
	if (g_pMain->Startup())
	{
		// Reset Battle Zone Variables.
		g_pMain->ResetBattleZone(BATTLEZONE_NONE);

		printf("The game server has been successfully started.\n");
		printf("** > GameServer Started in (%.2lf Seconds) on %04d-%02d-%02d at %02d:%02d\n", (clock() - x) / (double)CLOCKS_PER_SEC, time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute()); // 20.10.2020 Gameserver aciis suresi hesaplama
		printf("The game has been started. Enjoyable Games.\n");
		// Wait until console's signaled as closing
		s_hEvent.Wait();  
	}
	else
	{
		system("pause");
	}

	printf("Server is shutting down, please wait...\n");

	// This seems redundant, but it's not. 
	// We still have the destructor for the dialog instance, which allows time for threads to properly cleanup.
	g_bRunning = false; 

	delete g_pMain;

	CleanupTimeThread();
	CleanupConsoleInputThread();
	UnhookSignals();

	return 0;
}

BOOL WINAPI _ConsoleHandler(DWORD dwCtrlType)
{
	s_hEvent.BeginSynchronized();
	s_hEvent.Signal();
	s_hEvent.EndSynchronized();
	sleep(10000); // Win7 onwards allows 10 seconds before it'll forcibly terminate
	return TRUE;
}
