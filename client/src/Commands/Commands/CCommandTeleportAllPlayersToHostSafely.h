#pragma once
#include "../CCustomCommand.h"

class CCommandTeleportAllPlayersToHostSafely :
    public CCustomCommand
{
    // Inherited via CCustomCommand
    void Process(CRunningScript* script) override;
};
