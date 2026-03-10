#include "SyntySqliteCoreModule.h"

// #include "AutomationTestModule.h"
// #include "SQLiteDatabase.h"
#include "SQLiteDatabaseConnection.h"
#include "SidekickDBSubsystem.h"
#include "Editor.h"

#define LOCTEXT_NAMESPACE "FSyntySqliteCoreModule"

void FSyntySqliteCoreModule::StartupModule()
{

}

void FSyntySqliteCoreModule::ShutdownModule()
{
	if (!GEditor) return;

	USidekickDBSubsystem* DBSubsystem = GEditor->GetEditorSubsystem<USidekickDBSubsystem>();
	if (DBSubsystem)
	{
		DBSubsystem->Deinitialize();
	}
}



#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSyntySqliteCoreModule, SyntySqliteCore)