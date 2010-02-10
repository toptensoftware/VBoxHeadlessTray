VBoxHeadlessTray v1.1
=====================


What is it?
-----------

A simple windows app that runs a VirtualBox VM as a tray icon.
	
To use
------

1. Download [VBoxHeadelessTray.zip](http://cloud.github.com/downloads/toptensoftware/VBoxHeadlessTray/VBoxHeadlessTray.zip)

1. Extract the zip file to a suitable directory

2. Run the correct version (x64/Win32) for your platform. (double click in windows explorer)

3. Select a machine from the displayed list.  VBoxHeadlessTray will powerup the machine.

4. Right click the tray icon to start/stop/save etc...

If a VBoxHeadlessTray machine running when Windows is shutdown, it will automatically 
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

Requires VirtualBox 3.1.2 to be installed.  Machine execution, machine management 
and all other functionality is still supplied by Virtual Box.  VBoxHeadlessTray is 
a simple front end that hosts VirtualBox runtime.

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


Known Issues
------------

None

