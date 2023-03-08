#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
     Please read the comments in validate carefully
   */

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if (strlen(user_id) > 10){
      printf(NO_USER);
      return 0;
  }else if (strlen(user_id) < 10){
    for (int i = strlen(user_id); i<10; i++){
      user_id[i] = '\0';
    }
  }
  if (strlen(password) > 10){
    printf(INVALID);
    return 0;
  }else if(strlen(password) < 10){
    for (int i = strlen(password); i<10; i++){
      password[i] = '\0';
    }
  }

  int fd[2];
  pipe(fd);
  int result = fork();
  
  if (result <0){
    perror("fork");
    exit(-1);
  }else if (result == 0){
    close(fd[1]); //close the write end;
    dup2(fd[0], STDIN_FILENO); //make the stdin to be the stdout of the 
    execl("./validate", "validate", NULL);
    close(fd[0]);
  }else{
    close(fd[0]); //close the read end
    write(fd[1], user_id, 10);
    write(fd[1], password, 10);
    close(fd[1]); //done writing
    int status;
    wait(&status);
    if (WIFEXITED(status)){
      status = WEXITSTATUS(status);
      if (status == 0){
        printf(SUCCESS);
      }else if (status == 2){
        printf(INVALID);
      }else if (status == 3){
        printf(NO_USER);
      }
    }
  }


  return 0;
}