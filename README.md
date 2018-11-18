# Rootkit


In this project I developed a loadable kernel module that concealed the presence of target programs and activities by hooking kernel system calls such as open, read, getdents, etc.

The assumption will be that via a successful exploit of a vulnerability, I have gained the ability to execute privileged (root access) code in the system. My “attack” code will be represented by a small program that I wrote, which would load a kernel module that will conceal the presence of my "attack" program as well as some of its "malicious" activities.


### The program should operate in the following steps:
- The program will perform 1 malicious act. It will copy the /etc/passwd file (used for user authentication) to a new file: /tmp/passwd. Then it will open the /etc/passwd file and print a new line to the end of the file that contains a username and password that may allow a desired user to authenticate to the system. Note that this won’t actually allow you to authenticate to the system as the ‘sneakyuser’, but this step illustrates a type of subversive behavior that attackers may utilize. This line added to the password file should be exactly the following: 
sneakyuser:abc123:2000:2000:sneakyuser:/root:bash
