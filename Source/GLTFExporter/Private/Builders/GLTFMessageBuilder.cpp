// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFMessageBuilder.h"
#include "SKGLTFExporterModule.h"
#include "Interfaces/IPluginManager.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"

FGLTFMessageBuilder::FGLTFMessageBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions)
	: FGLTFBuilder(FilePath, ExportOptions)
{
}

void FGLTFMessageBuilder::ClearMessages()
{
	Messages.Empty();
}

void FGLTFMessageBuilder::AddMessage(ESKGLTFMessageSeverity Severity, const FString& Message)
{
	Messages.Emplace(Severity, Message);
}

void FGLTFMessageBuilder::AddInfoMessage(const FString& Message)
{
	AddMessage(ESKGLTFMessageSeverity::Info, Message);
}

void FGLTFMessageBuilder::AddWarningMessage(const FString& Message)
{
	AddMessage(ESKGLTFMessageSeverity::Warning, Message);
}

void FGLTFMessageBuilder::AddErrorMessage(const FString& Message)
{
	AddMessage(ESKGLTFMessageSeverity::Error, Message);
}

const TArray<FGLTFMessageBuilder::FLogMessage>& FGLTFMessageBuilder::GetMessages() const
{
	return Messages;
}

TArray<FGLTFMessageBuilder::FLogMessage> FGLTFMessageBuilder::GetMessages(ESKGLTFMessageSeverity Severity) const
{
	return Messages.FilterByPredicate(
		[Severity](const FLogMessage& LogMessage)
		{
			return LogMessage.Key == Severity;
		});
}

TArray<FGLTFMessageBuilder::FLogMessage> FGLTFMessageBuilder::GetInfoMessages() const
{
	return GetMessages(ESKGLTFMessageSeverity::Info);
}

TArray<FGLTFMessageBuilder::FLogMessage> FGLTFMessageBuilder::GetWarningMessages() const
{
	return GetMessages(ESKGLTFMessageSeverity::Warning);
}

TArray<FGLTFMessageBuilder::FLogMessage> FGLTFMessageBuilder::GetErrorMessages() const
{
	return GetMessages(ESKGLTFMessageSeverity::Error);
}

int32 FGLTFMessageBuilder::GetMessageCount() const
{
	return Messages.Num();
}

int32 FGLTFMessageBuilder::GetInfoMessageCount() const
{
	return GetInfoMessages().Num();
}

int32 FGLTFMessageBuilder::GetWarningMessageCount() const
{
	return GetWarningMessages().Num();
}

int32 FGLTFMessageBuilder::GetErrorMessageCount() const
{
	return GetErrorMessages().Num();
}

void FGLTFMessageBuilder::ShowMessages() const
{
	if (Messages.Num() > 0)
	{
		FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
		const TSharedRef<IMessageLogListing> LogListing = MessageLogModule.GetLogListing(SKGLTFEXPORTER_MODULE_NAME);

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Sketchfab");
		const FPluginDescriptor& PluginDescriptor = Plugin->GetDescriptor();

		LogListing->SetLabel(FText::FromString(PluginDescriptor.FriendlyName));
		LogListing->ClearMessages();

		for (const FLogMessage& Message : Messages)
		{
			LogListing->AddMessage(CreateTokenizedMessage(Message));
		}

		MessageLogModule.OpenMessageLog(SKGLTFEXPORTER_MODULE_NAME);
	}
}

void FGLTFMessageBuilder::WriteMessagesToConsole() const
{
	for (const FLogMessage& Message : Messages)
	{
		WriteMessageToConsole(Message);
	}
}

void FGLTFMessageBuilder::WriteMessageToConsole(const FLogMessage& LogMessage)
{
	switch (LogMessage.Key)
	{
		case ESKGLTFMessageSeverity::Info:    UE_LOG(LogGLTFExporter, Display, TEXT("%s"), *LogMessage.Value); break;
		case ESKGLTFMessageSeverity::Warning: UE_LOG(LogGLTFExporter, Warning, TEXT("%s"), *LogMessage.Value); break;
		case ESKGLTFMessageSeverity::Error:   UE_LOG(LogGLTFExporter, Error, TEXT("%s"), *LogMessage.Value); break;
		default:
			checkNoEntry();
			break;
	}
}

TSharedRef<FTokenizedMessage> FGLTFMessageBuilder::CreateTokenizedMessage(const FLogMessage& LogMessage)
{
	EMessageSeverity::Type MessageSeverity = EMessageSeverity::Type::CriticalError;

	switch (LogMessage.Key)
	{
		case ESKGLTFMessageSeverity::Info:    MessageSeverity = EMessageSeverity::Type::Info; break;
		case ESKGLTFMessageSeverity::Warning: MessageSeverity = EMessageSeverity::Type::Warning; break;
		case ESKGLTFMessageSeverity::Error:   MessageSeverity = EMessageSeverity::Type::Error; break;
		default:
			checkNoEntry();
			break;
	}

	return FTokenizedMessage::Create(MessageSeverity, FText::FromString(LogMessage.Value));
}
