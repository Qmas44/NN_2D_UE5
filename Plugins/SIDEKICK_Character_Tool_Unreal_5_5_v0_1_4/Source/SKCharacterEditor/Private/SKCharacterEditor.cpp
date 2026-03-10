#include "SKCharacterEditor.h"
#include "FSKCharacterEditorStyle.h"
#include "SKCharacterEditorCommands.h"

#define LOCTEXT_NAMESPACE "FSKCharacterEditorModule"

DEFINE_LOG_CATEGORY(LogSKCharacterEditor);

void FSKCharacterEditorModule::StartupModule()
{
    UE_LOG(LogSKCharacterEditor, Display, TEXT("SKCharacterEditorModule Startup"));
    FSKCharacterEditorStyle::Initialize();

    FSKCharacterEditorCommands::Register();
}

void FSKCharacterEditorModule::ShutdownModule()
{
    UE_LOG(LogSKCharacterEditor, Display, TEXT("SKCharacterEditorModule Shutdown"));
    FSKCharacterEditorStyle::Shutdown();
    
    FSKCharacterEditorCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FSKCharacterEditorModule, SKCharacterEditor)