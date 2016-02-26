// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include<stdlib.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<unistd.h>
#include<error.h>

typedef struct GraphNode{
	command_t command;
	struct GraphNode* before[100];
	int beforeCount;
	pid_t pid; //initialized to -1
	}GraphNode; 

typedef struct DependencyGraph{
	GraphNode* list_no_dependencies[100];
	GraphNode* list_dependencies[100];
	int nodep_Count;
	int dep_Count;
 }DependencyGraph;

typedef struct ListNode{
	GraphNode* tree;
	char* readList[100];
	char* writeList[100];
	int readListCount;
	int writeListCount;
	struct ListNode* prev;
	struct ListNode* next;
}ListNode;

void static inline addto_rl(ListNode* node, char* filename){
	node->readList[node->readListCount] = filename;
	node->readListCount++;
}

void static inline addto_wl(ListNode* node, char* filename){
	node->writeList[node->writeListCount] = filename;
	node->writeListCount++;
}

void processTree(ListNode* node, command_t cmd){

	switch(cmd->type){

	case SIMPLE_COMMAND:

		if(cmd->input != NULL)
			addto_rl(node,cmd->input);

		int index=0;
		if(strcmp(cmd->u.word[index], "echo") != 0){
			index++;
			while(cmd->u.word[index]!=NULL)
			{
				char* t = cmd->u.word[index];
				if(t[0]!='-')
					addto_rl(node, cmd->u.word[index]);
				index++;
			}
		}
		
		if(cmd->output)
			addto_wl(node, cmd->output);

		break;

	case SUBSHELL_COMMAND:
		if(cmd->input)
			addto_rl(node,cmd->input);
		if(cmd->output)
			addto_wl(node, cmd->output);
		processTree(node, cmd->u.subshell_command);
		break;
	default:
		processTree(node, cmd->u.command[0]);
		processTree(node, cmd->u.command[1]);
		break;
	}	
}

void checkDependency(ListNode* ref_cur, ListNode* cur){
	char* ref;
	char* comp;

	int i = 0;
	int j = 0;
	//WAR
	for(i; i < ref_cur->readListCount ; i++){
		ref = ref_cur->readList[i];
		for(j; j< cur->writeListCount;j++){
			comp = cur->writeList[j];			
			if(strcmp(ref, comp) == 0){
				cur->tree->before[cur->tree->beforeCount] = ref_cur->tree;
				cur->tree->beforeCount++;
				return;
			}
		}
	}
	
	i = 0;
	j = 0;		
	//RAW
	for(i; i < ref_cur->writeListCount ; i++){
		ref = ref_cur->writeList[i];
		for(j; j< cur->readListCount;j++){
			comp = cur->readList[j];				
			if(strcmp(ref, comp) == 0){
				cur->tree->before[cur->tree->beforeCount] = ref_cur->tree;
				cur->tree->beforeCount++;
				return;
			}
		}
	}
	
	i = 0;
	j = 0;
	//WAW	
	for(i; i < ref_cur->writeListCount ; i++){
		ref = ref_cur->writeList[i];
		for(j; j< cur->writeListCount;j++){
			comp = cur->writeList[j];				
			if(strcmp(ref, comp) == 0){
				cur->tree->before[cur->tree->beforeCount] = ref_cur->tree;
				cur->tree->beforeCount++;
				return;
			}
		}
	}

}

DependencyGraph creat_dependency_graph(command_stream_t stream){
	ListNode* alist = NULL;
	ListNode* list_cursor = alist;
	DependencyGraph aGraph;
	aGraph.nodep_Count = 0;
	aGraph.dep_Count = 0;
	command_t cmd;
	while(cmd = read_command_stream(stream))	
	{
		//add to dependency list
		ListNode* list_node_curtree = (ListNode*) malloc(sizeof(ListNode));
		list_node_curtree->readListCount = 0;
		list_node_curtree->writeListCount = 0;
		list_node_curtree->next = NULL;
		GraphNode* cur_graph_node = (GraphNode*) malloc(sizeof(GraphNode));
		cur_graph_node->command = cmd;
		list_node_curtree->tree = cur_graph_node;
		processTree(list_node_curtree,list_node_curtree->tree->command);
		if(alist != NULL){	
			list_node_curtree->prev = list_cursor;
			list_cursor->next = list_node_curtree;
		}
		else{
			alist = list_node_curtree;	
			list_node_curtree->prev = NULL;		
		}
		list_cursor = list_node_curtree;
		
		///add to dependency graph
		ListNode* dep_cursor = alist;
		list_cursor->tree->beforeCount = 0;
		while(dep_cursor != NULL){
			if(dep_cursor != list_cursor){
				checkDependency(dep_cursor,list_cursor);	
			}
			dep_cursor = dep_cursor->next;
		}
		if(list_cursor->tree->beforeCount == 0){
			aGraph.list_no_dependencies[aGraph.nodep_Count] = cur_graph_node;	
			aGraph.nodep_Count++;
		}
		else{
			aGraph.list_dependencies[aGraph.dep_Count] = cur_graph_node;
			aGraph.dep_Count++;
		}
	}
	
	return aGraph;
	
}

void execute_dependency_graph(DependencyGraph graph)
{
	execute_nodep(graph.list_no_dependencies, graph.nodep_Count);
	execute_dep(graph.list_dependencies, graph.dep_Count);
}

void execute_nodep(GraphNode* nodep[], int cnt)
{
	int index=0;
	for(index; index<cnt; index++)
	{	
		pid_t pid = fork();
		if(pid == 0 ){
			execute_command(nodep[index]->command, true);
			exit(0);
		}
		else{
			nodep[index]->pid = pid;
		}
	}
}

void execute_dep(GraphNode* dep[], int cnt)
{
	int index=0;
	for(index; index<cnt; index++)
	{
		int status;
		int temp=0;
		for(temp;temp < dep[index]->beforeCount;temp++)
		{
			waitpid(dep[index]->before[temp]->pid, status, 0);
		}
		pid_t pid=fork();
		if(pid==0){
			execute_command(dep[index]->command, true);
			exit(0);
		}			
		else{
			dep[index]->pid = pid;
		}	
	}
}

void execute_timetravel(command_stream_t command_stream){
	DependencyGraph graph = creat_dependency_graph(command_stream);
	execute_dependency_graph(graph);

}

/////
/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */
void initialize_io (command_t c){
	if (c->input != NULL){
		int fd  = open(c->input, O_RDONLY);
		if (fd < 0){
			exit(1);
		}
		if (dup2(fd , 0) < 0){
			exit(1);
		}
	}
	if(c->output != NULL){
		int fd = open(c->output , O_CREAT|O_TRUNC|O_WRONLY, 0644);
		if (fd < 0){
			exit(1);
		}
		if (dup2(fd , 1) < 0){
			exit(1);
		}
	}	
}


void execute_and (command_t c){
    execute_cmd(c->u.command[0]);
    if(c->u.command[0]->status==0)
    {
        execute_cmd(c->u.command[1]);
        c->status = c->u.command[1]->status;
    }
    else
    {
        c->status = c->u.command[0]->status;
    }
    
}

void execute_or (command_t c){
    execute_cmd(c->u.command[0]);
    if(c->u.command[0]->status==0)
    {
        c->status = c->u.command[0]->status;
    }
    else
    {
        execute_cmd(c->u.command[1]);
        c->status = c->u.command[1]->status;
    }
}

void execute_pipe (command_t c){
	int fd[2];
	pipe(fd);
	int wpid = fork(); 
	int wstatus, rstatus;
	if (wpid == 0){	//child who writes to pipe
		close(fd[0]); //close readend of pipe
		dup2(fd[1], 1);
		execute_cmd(c->u.command[0]);
	}
	else{
		int rpid = fork();
		if (rpid == 0){	//child who reads from pipe
			close(fd[1]);
			dup2(fd[0],0);
			execute_cmd(c->u.command[1]);
		}
		else{
			close(fd[0]);
			close(fd[1]);
			waitpid(wpid, &wstatus, 0);
			waitpid(rpid, &rstatus, 0);
			int estatus = WEXITSTATUS(wstatus);
			c->u.command[0]->status = estatus;
			estatus = WEXITSTATUS(rstatus);
			c->u.command[1]->status = estatus;
		}
	}
}


void execute_sequence (command_t c){
    int status;
    pid_t pid = fork();
    if(pid==0)
    {
        //child process
        pid = fork();
        if(pid==0)
        {
            //child of the child process
            execute_cmd(c->u.command[0]);
             _exit(c->u.command[0]->status);
        }
        else
        {
            waitpid(pid, &status, 0);
            execute_cmd(c->u.command[1]);
            _exit(c->u.command[1]->status);
        }
    }
    else
    {
        waitpid(pid,&status,0);
        int estatus = WEXITSTATUS(status);
        c->status = estatus;
    }
}

void execute_simple (command_t c){
    pid_t pid = fork();
    int status;
    if(pid==0)
    {
        initialize_io(c);
        execvp(c->u.word[0],c->u.word);
    }
    else
    {
        waitpid(pid,&status,0);
        int estatus = WEXITSTATUS(status);
        c->status = estatus;
    }
        
    
}

void execute_subshell (command_t c){
    initialize_io(c);
    execute_cmd(c->u.subshell_command);
}

int
command_status (command_t c)
{
  return c->status;
}

void execute_cmd (command_t c){
    
    switch (c->type) {
        case AND_COMMAND:
            execute_and(c);
            break;
            
        case OR_COMMAND:
            execute_or(c);
            break;
            
        case PIPE_COMMAND:
            execute_pipe(c);
            break;
            
        case SEQUENCE_COMMAND:
            execute_sequence(c);
            break;
            
        case SIMPLE_COMMAND:
            execute_simple(c);
            break;
            
        case SUBSHELL_COMMAND:
            execute_subshell(c);
            break;
        
        default:
            break;
    }
}



void
execute_command (command_t c, bool time_travel)
{

        execute_cmd(c);

}


