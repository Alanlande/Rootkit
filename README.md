# Rootkit


In this project I developed a loadable kernel module that concealed the presence of target programs and activities by hooking kernel system calls such as open, read, getdents, etc.

The assumption will be that via a successful exploit of a vulnerability, I have gained the ability to execute privileged (root access) code in the system. My “attack” code will be represented by a small program that I wrote, which would load a kernel module that will conceal the presence of my "attack" program as well as some of its "malicious" activities.


### The program should operate in the following steps:

- The program will perform 1 malicious act. It will copy the /etc/passwd file (used for user authentication) to a new file: /tmp/passwd. Then it will open the /etc/passwd file and print a new line to the end of the file that contains a username and password that may allow a desired user to authenticate to the system. Note that this won’t actually allow you to authenticate to the system as the ‘sneakyuser’, but this step illustrates a type of subversive behavior that attackers may utilize. This line added to the password file should be exactly the following: 

```sh
sneakyuser:abc123:2000:2000:sneakyuser:/root:bash
```

- The program will load the sneaky module (sneaky_mod.ko) using the “insmod” command. Note that when loading the module, the sneaky program will also pass its process ID into the module. 

- The program will then enter a loop, reading a character at a time from the keyboard input until it receives the character ‘q’ (for quit). Then the program will exit this waiting loop. Note this step is here so that you will have a chance to interact with the system while: 1) your sneaky process is running, and 2) the sneaky kernel module is loaded. This is the point when the malicious behavior will be tested.

- It will hide the “sneaky_process” executable file from both the ‘ls’ and ‘find’ UNIX commands.

- In a UNIX environment, every executing process will have a directory under /proc that is named with its process ID (e.g /proc/1480). This directory contains many details about the process. My sneaky kernel module will hide the /proc/<sneaky_process_id> directory (note hiding a directory with a particular name is equivalent to hiding a file!).

- It will hide the modifications to the /etc/passwd file that the sneaky_process made. It will do this by opening the saved “/tmp/passwd” when a request to open the “/etc/passwd” is seen. 

- It will hide the fact that the sneaky_module itself is an installed kernel module. The list of active kernel modules is stored in the /proc/modules file. Thus, when the contents of that file are read, the sneaky_module will remove the contents of the line for “sneaky_mod” from the buffer of read data being returned. 

- The program will unload the sneaky kernel module using the “rmmod” command

- The program will restore the /etc/passwd file (and remove the addition of “sneakyuser” authentication information) by copying /tmp/passwd to /etc/passwd.
