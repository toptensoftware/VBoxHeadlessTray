VBoxHeadlessTray
================

What is it?
-----------

VBoxHeadlessTray is simple windows app that runs a VirtualBox VM as a tray icon:

![TrayIcon](http://www.toptensoftware.com/VBoxHeadlessTray/tray1.png)

Common commands are readily available from the context menu:

![TrayMenu](http://www.toptensoftware.com/VBoxHeadlessTray/tray2.png)
	
	
Download Links
--------------

* [VBoxHeadlessTray610Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray610Setup.exe) - for VirtualBox 6.1.0
* [VBoxHeadlessTray604Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray604Setup.exe) - for VirtualBox 6.0.4
* [VBoxHeadlessTray52Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray52Setup.exe) - for VirtualBox 5.2
* [VBoxHeadlessTray51Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray51Setup.exe) - for VirtualBox 5.1
* [VBoxHeadlessTray50Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray50Setup.exe) - for VirtualBox 5.0
* [VBoxHeadlessTray43Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray43Setup.exe) - for VirtualBox 4.3
* [VBoxHeadlessTray42Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray42Setup.exe) - for VirtualBox 4.2
* [VBoxHeadlessTray41Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray41Setup.exe) - for VirtualBox 4.1
* [VBoxHeadlessTray40Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray40Setup.exe) - for VirtualBox 4.0
* [VBoxHeadlessTray32Setup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTray32Setup.exe) - for VirtualBox 3.x
* [VBoxHeadlessTraySetup.exe](http://www.toptensoftware.com/downloads/VBoxHeadlessTraySetup.exe) - for earlier versions of VirtualBox
	
To use
------

1. Download and Install 

2. From Start menu -> Programs run VBoxHeadlessTray

3. Select a machine from the displayed list.  VBoxHeadlessTray will powerup the machine.

4. Right click the tray icon to start/stop/save etc...

If a VBoxHeadlessTray machine is running when Windows is shutdown, it will automatically 
save the machine's state and on Windows restarting, VBoxHeadlessTray will restart itself 
automatically and resume the VM.  

To prevent a machine starting at windows logon, exit VBoxHeadlessTray before shutting 
down windows, or use the -np option.


CommandLine Arguments
---------------------

Usage: VBoxHeadlessTray [-?|-h] [-np] <machinename>

* -np: don't power on the machine
* -h:  show help

Requirements
------------

VBoxHeadlessTray 3.2 requires VirtualBox 3.2 to be installed.
VBoxHeadlessTray 1.2 requires VirtualBox 3.1.2 to be installed.  

Machine execution, machine management and all other functionality is still supplied by Virtual Box.  VBoxHeadlessTray is a simple front end that hosts VirtualBox runtime.

Testing on Windows XP, Windows Vista and Windows 7.  


Custom Menu Commands
--------------------

It is possible to customize the context menu that appears when clicking on the 
VBoxHeadlessTray tray icon.  This is done through VBox Guest Properties.

Say the of the VM being hosted was running a web site and you want a quick way to 
launch a web browser for it.  At the command prompt, use the VBoxManage tool (see 
VirtualBox documentation) to add the following guest properties.

In this this example my VM is running a website toptensoftware.ulamp and my machine 
name is devvm

	VBoxManage guestproperty set devvm "VBoxHeadlessTray\ContextMenus\browse\menutext" "Open Web Browser..."
	VBoxManage guestproperty set devvm "VBoxHeadlessTray\ContextMenus\browse\command" "http://toptensoftware.ulamp"
	VBoxManage guestproperty set devvm "VBoxHeadlessTray\ContextMenus\browse\verb" "open"

If verb is empty, the command is executed (CreateProcess) otherwise it's launched with 
ShellExecute.
	
You can optionally set another property to specify the context under which the 
commmand is available (either Running or Stopped).

	VBoxManage guestproperty set devvm "VBoxHeadlessTray\ContextMenus\browse\context" "Running"


Source Code and Build Instructions
---

Source code for VBoxHeadlessTray is available from [github](http://github.com/toptensoftware/VBoxHeadlessTray).

To build:

  1. Get and build SimpleLib - git://github.com/toptensoftware/SimpleLib.git

  2. Get the [VirtualBox SDK](http://download.virtualbox.org/virtualbox/vboxsdkdownload.html)
  
  3. Get VBoxHeadlessTray - git://github.com/toptensoftware/VBoxHeadlessTray.git
  
  4. Open and build VBoxHeadlessTray.sln

Expected folder structure is:

    \YourProjectsFolder
      \SDKs
        \VirtualBox4
    	 \sdk
      \SimpleLib
      \VBoxHeadlessTray

To build setup program:

  1. Build both x64 and Win32 Release configs
  
  2. Download and install [NSIS](http://nsis.sourceforge.net/Download)
  
  3. Right click on the VBoxHeadlessTraySetup.nsi file in Visual Studio project and select properties
  
  4. Set Excluded from Build to No
  
  5. OK Properties
  
  6. Right click .nsi file again and choose Compile.
  
  7. Set the .nsi file properties back to Excluded from Build.
  
This manual building of the setup is due to the fact that we need both x64 and Win32 exe's for the setup program and Visual Studio doesn't provide a way to do this easily.
  

Known Issues
------------

None


License
-------

![Creative Commons License](http://i.creativecommons.org/l/by-nc-sa/2.5/au/88x31.png)

**VBoxHeadlessTray** by [Topten Software](http://www.toptensoftware.com/VBoxHeadlessTray) is licensed under a [Creative Commons Attribution-Noncommercial-Share Alike 2.5 Australia License](http://creativecommons.org/licenses/by-nc-sa/2.5/au/).