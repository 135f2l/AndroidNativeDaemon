# AndroidNativeDaemon
Native daemon process for Android Apps, start your service automatically when it's gone. 

## Build the source
1. You must have [NDK](https://developer.android.com/tools/sdk/ndk/index.html) installed on your machine.
2. Copy both the C source file (daemon.c) and Android.mk file) into your project directory.
3. On your project directory, run `<NDKPath>/ndk-build`. Normally, it will create an executeable file in `libs/armeabi/` named `daemon_c`, which is excactly what we want.

## Integrate with your App
1. Copy the executable file we have just built into your `assets` path of your Android App's source path.
2. In your Android Java code, release the daemon executeable file (by using FileOutputStream or anything eqaully) into your App's private file path, say `/data/data/com.yourapp/files`, and most importantly, set the file actually EXECUTABLE by using 

		file.setExecutable(true);
		
3. Run this daemon simply by 

		Process process = new ProcessBuilder().command(daemonPath, packageName, processName, serviceName, isEnableLog ? "1" : "0", LogFilePath).start();
		
	the 6 parameters are:
	
	`daemonPath` : the executable path of the daemon file, say `/data/data/com.yourapp/files/daemon`.	
	
	`packageName` : your App package's name. Used in daemon to start your service.
	
	`processName` : your App's process name. Used in daemon to detect whether your App is dead or alive.
	
	`serviceName` : your App's service name. Used in daemon to start your service.
	
	`isEnableLog` : actully it's a char value, "1" for enable log, "0" for otherwise.
	
	`logFilePath` : if you choose to enable log, it's the log file path. Log file name should be like `daemon-yyyy-MM-dd-%VERSION_DEFINED_IN_C%.log`
		
## Notice
1. If you want to start your service through this daemon, you must set your service's `exported` property to true in AndroidManifest.xml file.

		<service
                android:name="com.yourApp.yourService"
                android:exported="true">
        </service>
        
## Known issues
1. May not work properly under Android 5.0+, due to the system will have the daemon process killed when your App is killed. DONOT hesitate to let me know if you have a solution.

## License
This project is licensed under the [MIT license](https://github.com/dotnet/corefx/blob/master/LICENSE).