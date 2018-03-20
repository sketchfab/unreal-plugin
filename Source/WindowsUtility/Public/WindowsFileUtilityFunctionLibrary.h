#pragma once

#include "WFULambdaRunnable.h"
#include "WindowsFileUtilityFunctionLibrary.generated.h"

//Struct to Track which delegate is watching files
struct FWatcher
{
	UObject* Delegate;
	
	FString Path;

	WFULambdaRunnable* Runnable = nullptr;

	FThreadSafeBool ShouldRun = true;
	
};

inline bool operator==(const FWatcher& lhs, const FWatcher& rhs)
{
	return lhs.Delegate == rhs.Delegate;
}

UCLASS(ClassGroup = WindowsFileUtility, Blueprintable)
class WINDOWSFILEUTILITY_API UWindowsFileUtilityFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/*Expects full path including name. you can use this function to rename files.*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static bool MoveFileTo(const FString& From, const FString& To);

	/*Expects full path including folder name.*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static bool CreateDirectoryAt(const FString& FullPath);

	/*Deletes file (not directory). Expects full path.*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static bool DeleteFileAt(const FString& FullPath);

	/*Deletes empty folders only. Expects full path.*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static bool DeleteEmptyFolder(const FString& FullPath);

	/*Dangerous function, not exposed to blueprint. */
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static bool DeleteFolderRecursively(const FString& FullPath);

	/** Watch a folder for change. WatcherDelegate should respond to FolderWatchInterface*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static void WatchFolder(const FString& FullPath, UObject* WatcherDelegate);

	/** Stop watching a folder for change. WatcherDelegate should respond to FolderWatchInterface*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static void StopWatchingFolder(const FString& FullPath, UObject* WatcherDelegate);

	/** List the contents, expects UFileListInterface*/
	UFUNCTION(BlueprintCallable, Category = WindowsFileUtility)
	static void ListContentsOfFolder(const FString& FullPath, UObject* ListDelegate);

	//Convenience C++ callback
	static void ListContentsOfFolderToCallback(const FString& FullPath, TFunction<void(const TArray<FString>&, const TArray<FString>&)> OnListCompleteCallback);

	//Todo: add watch folder with threadsafe boolean passthrough
	//static void ListContentsOfFolderToCallback(const FString& FullPath, TFunction<void(const TArray<FString>&, const TArray<FString>&)> OnListCompleteCallback);

private:
	static void WatchFolderOnBgThread(const FString& FullPath, const FWatcher* Watcher);
	static TMap<FString, TArray<FWatcher>> Watchers;
};