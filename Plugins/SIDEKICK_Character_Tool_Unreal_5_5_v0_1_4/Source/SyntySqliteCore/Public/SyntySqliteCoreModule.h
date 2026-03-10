#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SQLiteDatabaseConnection.h"

class SYNTYSQLITECORE_API FSyntySqliteCoreModule : public IModuleInterface
{

    
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    
};
