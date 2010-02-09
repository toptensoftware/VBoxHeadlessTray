VBoxHeadlessTray v0.1
=====================


What is it?
-----------

A simple windows app that runs a VirtualBox VM as a tray icon.
	
To use
------

1. Place VBoxHeadlessTray in C:\Program Files\Sun\VirtualBox (or wherever you have Virtual Box Installed

2. Run it by double clicking

3. Select a machine from the displayed list

4. Right click the tray icon to start/stop/save etc...


Requirements
------------

Requires VirtualBox be installed.  Machine execution, machine management and all other functionality is still supplied by Virtual Box.  VBoxHeadlessTray is a simple front end that hosts VirtualBox runtime.

Testing on Windows XP, Windows Vista and Windows 7.  


Autorun and Windows Login
-------------------------

If a VBoxHeadlessTray machine running when Windows is shutdown, it will automatically save the machine's state.
	
On Windows restarting, VBoxHeadlessTray will restart itself automatically and resume the VM.

To prevent a machine starting at windows logon, exit VBoxHeadlessTray before shutting down windows.


Custom Menu Commands
--------------------

It is possible to customize the context menu that appears when clicking on the VBoxHeadlessTray tray icon.  This is done through VBox Guest Properties.

Say the IP of the VM being hosted was running a web site and you want a quick way to launch a web browser for it.  At the command prompt, use the VBoxManage tool (see VirtualBox documentation) to add the following guest properties.

In this this example my VM is running a website toptensoftware.ulamp and my machine name is devvm

> VBoxManage guestproperty set "VBoxHeadlessTray\ContextMenus\browse\menutext" "Open Web Browser..."
> VBoxManage guestproperty set "VBoxHeadlessTray\ContextMenus\browse\command" "http://toptensoftware.ulamp"
> VBoxManage guestproperty set 





Known Issues
------------

Due to either a bug in VBoxHeadlessTray, or perhaps VirtualBox itself I haven't been able to get VirtualBox's VRDP server functionality working with VBoxHeadlessTray.  If you try you'll be able to connect from a client but will only get a blank screen.

I'd like to say I'm working on a fix for this, but I'm currently stumped so I'm not sure when/if a fix for this will be available.


Thanks
------

Special thanks to the members of VirtualBox development mailing for helping out with my stupid questions.






http://www.toptensoftware.com/vboxheadlesstray
