# Rootkit


In this project I developed a loadable kernel module that concealed the presence of target programs and activities by hooking kernel system calls such as open, read, getdents, etc.

## The assumption will be that via a successful exploit of a vulnerability, I have gained the ability to execute privileged (root access) code in the system. My “attack” code will be represented by a small program that I wrote, which would load a kernel module that will conceal the presence of my "attack" program as well as some of its "malicious" activities.
