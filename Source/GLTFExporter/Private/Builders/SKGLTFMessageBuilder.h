// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFBuilder.h"

enum class ESKGLTFMessageSeverity
{
	Info,
	Warning,
	Error
};

class FGLTFMessageBuilder : public FGLTFBuilder
{
protected:

	FGLTFMessageBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions);

public:

	typedef TTuple<ESKGLTFMessageSeverity, FString> FLogMessage;

	void ClearMessages();

	void AddMessage(ESKGLTFMessageSeverity Severity, const FString& Message);

	void AddInfoMessage(const FString& Message);

	void AddWarningMessage(const FString& Message);

	void AddErrorMessage(const FString& Message);

	const TArray<FLogMessage>& GetMessages() const;

	TArray<FLogMessage> GetMessages(ESKGLTFMessageSeverity Severity) const;

	TArray<FLogMessage> GetInfoMessages() const;

	TArray<FLogMessage> GetWarningMessages() const;

	TArray<FLogMessage> GetErrorMessages() const;

	int32 GetMessageCount() const;

	int32 GetInfoMessageCount() const;

	int32 GetWarningMessageCount() const;

	int32 GetErrorMessageCount() const;

	void ShowMessages() const;

	void WriteMessagesToConsole() const;

private:

	static void WriteMessageToConsole(const FLogMessage& LogMessage);

	static TSharedRef<FTokenizedMessage> CreateTokenizedMessage(const FLogMessage& LogMessage);

	TArray<FLogMessage> Messages;
};
