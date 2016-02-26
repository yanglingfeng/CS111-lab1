// UCLA CS 111 Lab 1 command reading

#include <ctype.h>
#include "command.h"
#include "command-internals.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <error.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */



/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

struct command_node{
    command_t command;
    struct command_node* next;
};

struct command_stream{
    struct command_node* head;
    struct command_node* tail;
    struct command_node* cursor;
};

static void addto_cs(command_stream_t p_command_s , command_t p_command){
    p_command_s->cursor->command = p_command;
    p_command_s->cursor=p_command_s->cursor->next;
    p_command_s->tail = p_command_s->cursor;
}

enum operator_type{
  op_seq = 1,
  op_and = 2,
  op_or = 3,
  op_pipe = 4,
  op_sub_left = 5,
  op_sub_right = 6,
  op_non = 0,
};

struct stack{
    //two stacks, command and operator.
    command_t stack_command[1000];
    enum operator_type stack_operator[1000];
    
    int op_top;   //index in the array
    int cmd_top;
    int op_empty;
    int cmd_empty;
};

inline static void initialize_stack(struct stack *stk){
  stk->op_top = -1;
  stk->cmd_top = -1;
  stk->op_empty = 1;
  stk->cmd_empty = 1;
}

inline static void push_op(struct stack* stk, enum operator_type op){
    stk->op_top++;
    stk->stack_operator[stk->op_top] = op;
    stk->op_empty = 0;
}

inline static void push_cmd(struct stack* stk, command_t cmd){
    stk->cmd_top++;
    stk->stack_command[stk->cmd_top] = cmd;
    stk->cmd_empty = 0;
}

inline static enum operator_type top_op(struct stack* stk){return stk->stack_operator[stk->op_top];}

inline static command_t top_cmd(struct stack* stk){return stk->stack_command[stk->cmd_top];}

inline static enum operator_type pop_op(struct stack* stk){
    if(stk->op_empty){
        return op_non;
    }
    enum operator_type temp = stk->stack_operator[stk->op_top];
    stk->op_top--;
    if(stk->op_top == -1){
        stk->op_empty = 1;
    }
    return temp;
}

inline static command_t pop_cmd(struct stack* stk){
    if(stk->cmd_empty){
        return NULL;
    }
    command_t temp = stk->stack_command[stk->cmd_top];
    stk->cmd_top--;
    if(stk->cmd_top == -1){
        stk->cmd_empty = 1;
    }
    return temp;
}



/*char characterization*/
int isChar(char input)
{
    if(isalnum(input))
        return 1;
    else if (input=='!'||input=='%'||input=='+'||input==','||input=='-'||input=='.'||input=='/'||input==':'||input=='@'||input=='^'||input=='_'||input=='#')
        return 1;
    else
        return 0;
}
int isOperator(char input)
{
    if(input==';'||input=='|'||input=='&'||input=='('||input==')'||input=='<'||input=='>')
        return 1;
    return 0;
}
int isTerminator(char input){ //residue
  if (input=='\n'||isspace(input) || input == ';')
    return 1;
  return 0;
}
int isValid(char input)
{
    if(isChar(input))
        return 1;
    else if(isOperator(input))
        return 1;
    else if(isTerminator(input)){
      return 1;
    }
    else
        return 0;
}

//////////////////////////////
//Globals
//////////////////////////////
//flags
int has_error = 0;
int inside_comment = 0;
int prev_is_redirect = 0;
int prev_is_seq = 0;
int prev_is_newline = 0;
int prev_is_cmd = 0;
int char_buffer_empty = 1;
int prev_is_single_and = 0;
int prev_is_and = 0;
int prev_is_pipe = 0;
int prev_is_or = 0;
int single_sub_left = 0;
int prev_is_right_sub = 0;
int has_tree = 0;
int direct_counter = 0;

//data structure
char* trees[100][100];
int trees_num = 0;     //number of trespoes, first index in trees
int in_tree_index = 0;  //current position in filling up the tree, temporary second index
char char_buffer[100];
int char_buffer_pos = 0;
int line_num = 1;
int prev_error_line_num = 0;
char temp_char;

//helpers
//print error
void print_error(){
  if(line_num != prev_error_line_num){
    fprintf(stderr, "%d;",line_num);
    prev_error_line_num = line_num;
  }
  has_error = 1;
  exit(1);
}

//add this character
void add_this_char(){
   char_buffer[char_buffer_pos] = temp_char;
   char_buffer_pos++;
   char_buffer_empty = 0;
}

//write to tree
void write_to_tree(){
  char_buffer[char_buffer_pos] = 0;
  trees[trees_num][in_tree_index] = (char*) malloc(100);
  int i = 0;
  char* t_new_tree = trees[trees_num][in_tree_index];
  for(i;i<(char_buffer_pos +1);i++){
    t_new_tree[i] = char_buffer[i];
  } 
  in_tree_index++;
  char_buffer_empty = 1;
  char_buffer_pos = 0;
  has_tree = 1;
  for(i = 0 ; i< (char_buffer_pos +1); i++){
    char_buffer[i] = 0;
  }
}
//tree ends
void tree_ends(){
  trees[trees_num][in_tree_index] = NULL;
  trees_num++;
  in_tree_index = 0;
}

//////globals end//////////////////

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
  

  //read into buffer then add to tree
  while (1){
    temp_char = (char) get_next_byte(get_next_byte_argument);
    if(temp_char == EOF){
      break;
    }
    //invalid character
    if(!isValid(temp_char)){
      print_error();
    }
    
    //comment
    if( inside_comment && temp_char != '\n' || (temp_char == '#' && char_buffer_pos == 0)){
      direct_counter = 0;
      inside_comment = 1;
      continue;
    }
    
    //newline
    if(temp_char == '\n'){
      if(inside_comment){
	inside_comment = 0;
	prev_is_newline = 1;
      }
      if(prev_is_redirect || single_sub_left ){//wrong use of newline following redirect, left sbss, 
        print_error();
      }
      if(char_buffer_empty == 0 && prev_is_cmd){//write a regular command to tree
	write_to_tree();
	prev_is_newline = 1;
	
	if( single_sub_left== 0 && has_tree ){//tree ends
	  tree_ends();
	  prev_is_newline =0;
	  prev_is_cmd = 0;
	  prev_is_right_sub = 0;
	}
      }
      else{//nothing to write
	prev_is_newline = 1;
	if((prev_is_seq || prev_is_cmd) && single_sub_left == 0){//tree ends
	  tree_ends();
	  prev_is_newline = 0;
	  prev_is_cmd = 0;
	  prev_is_seq = 0;
	  prev_is_right_sub = 0;
	}
      }
      direct_counter = 0;
      line_num++;
      continue;
    }

    //spaces
    if(isspace(temp_char)){
      if(char_buffer_empty == 0 && prev_is_cmd){
	write_to_tree(); //write to tree
      }
      continue;
    }
    
    //normal character or redirect or sequence
  if(temp_char == '<' || temp_char == '>' || temp_char == ';'){

    if(char_buffer_empty == 0){
      write_to_tree();
    }
	if(temp_char == '<' || temp_char == '>'){
    if(prev_is_cmd == 0){print_error();}

    if(direct_counter >= 1 ){ //not cmd < cmd > cmd type
      int t = (in_tree_index - 1);
      char* tester_p = trees[trees_num][t];
      char tester = tester_p[0];
      while(isChar(tester)){
        t--;
        tester_p = trees[trees_num][t];
        tester = tester_p[0];
      }
      if (tester != '<' || direct_counter >1){
        print_error();
      }
    }

    
    direct_counter++;
    
    prev_is_redirect = 1;
    prev_is_cmd == 0;
	}	
	if (temp_char == ';'){
    if(prev_is_cmd == 0){
      print_error();
    }
	  prev_is_seq = 1;
    prev_is_cmd = 0;
	}

	//single redirect or sequence
    add_this_char();
	  write_to_tree();
	
	prev_is_cmd = 0; //the only thing possible before is a regular command or subshell!
	prev_is_right_sub = 0;
  direct_counter = 0;
    continue;
      }

      //normal char
      if(isChar(temp_char)){
	if(prev_is_right_sub){//only wrong case (echo a) echo b
	  print_error();
	}
  if(char_buffer[0] == '|'){
        write_to_tree();
        add_this_char();
      }
      else{
        add_this_char();
      }
	   prev_is_cmd = 1;
	   prev_is_redirect = 0;
	   prev_is_and = 0;
	   prev_is_or = 0;
	   prev_is_pipe = 0;
	   prev_is_seq = 0;
	   prev_is_newline = 0;

      continue;
    }

    //and or pipe
    if(temp_char == '&'){
      if(prev_is_cmd == 0 && prev_is_single_and == 0){//wrong use of &
        print_error();
      }
      if(char_buffer_empty == 0 && prev_is_single_and == 0){//write to tree
        write_to_tree();
	add_this_char();
	prev_is_single_and = 1;
      }
      else if(prev_is_single_and){//write to tree
	prev_is_single_and = 0;
	prev_is_and = 1;
        add_this_char();
	write_to_tree();
      }
      else{
	prev_is_single_and = 1;
	add_this_char();
      }
      prev_is_cmd = 0;
      prev_is_right_sub = 0;
      direct_counter = 0;
      continue;
    }
    
    if(temp_char == '|'){
      if(prev_is_cmd == 0 && prev_is_pipe == 0) {//wrong wse
	print_error();
      }
      if(char_buffer_empty ==0 && prev_is_pipe == 0 ){
	write_to_tree();
	add_this_char();
	prev_is_pipe = 1;
      }
      else if(prev_is_pipe){
	prev_is_pipe = 0;
	prev_is_or = 1;
	add_this_char();
	write_to_tree();
      }
      else{
	       prev_is_pipe = 1;
	       add_this_char();
      }
      prev_is_cmd = 0;
      prev_is_right_sub = 0;
      direct_counter = 0;
      continue;
    }

    //subshell
    if(temp_char == '('){
      if( prev_is_cmd || prev_is_redirect ){//can not have command or redirection before a subshell
	print_error();
      }
      if(char_buffer_empty == 0){
        write_to_tree();
      }
      single_sub_left = 1;
      add_this_char();
      write_to_tree();
      continue;
    }
    if(temp_char == ')'){
      if((prev_is_cmd == 0 && prev_is_seq == 0) || single_sub_left == 0 ){//must have a command inside subshell
	print_error();
      }
      if(char_buffer_empty == 0){
        write_to_tree();
      }
      prev_is_right_sub = 1;
      single_sub_left = 0;
      direct_counter = 0;
      add_this_char();
      write_to_tree();
      continue;
    }

  }//end of while
  //if(has_error) {exit(1);}
  if(single_sub_left || prev_is_redirect || prev_is_and || prev_is_or || prev_is_pipe ){
    print_error();
    exit(1);
  }
    
   //create the linked list
    
  command_stream_t cmd_stream=(command_stream_t)malloc(sizeof( struct command_stream));
  cmd_stream->head = NULL;
  cmd_stream->tail = cmd_stream->head;
  
    //iterate through the trees
    int t;
    for(t=0;t<trees_num;t++)
    {
        struct stack command_stack;
        struct stack operator_stack;
  initialize_stack(&command_stack);
  initialize_stack(&operator_stack);
        int j=0;
        
        while(trees[t][j]!=NULL)
        {
            command_t command_temp=(command_t)malloc(sizeof(struct command));
            
            //1.simple command
            char* temp_word=trees[t][j];
            if(isChar(temp_word[0])&&temp_word[0]!='>'&&temp_word[0]!='<')
            {
                command_temp->type=SIMPLE_COMMAND;
                command_temp->u.word= (char**) malloc(100*sizeof(char*));
    int i = 0;
    while(isChar(*trees[t][j])&&*trees[t][j]!='>'&&*trees[t][j]!='<')  //!!!!!!!!!!!!!!!!!!!!!!!!!!To fix
                {
                    command_temp->u.word[i]= (char*) malloc(strlen(temp_word)+1);
                    strcpy(command_temp->u.word[i],trees[t][j]);
                    j++;
        i++;
    if(trees[t][j]==NULL)
                    break;
                }
                push_cmd(&command_stack, command_temp);
                continue;
            }
            
            //2."("
            if(temp_word[0]=='(')
            {
                push_op(&operator_stack, op_sub_left);
                j++;
                continue;
            }
            
            //3.operator and empty operator stack
            if(isOperator(temp_word[0])&&temp_word[0]!='('&&temp_word[0]!=')'&&operator_stack.op_empty==1&&temp_word[0]!='<'&&temp_word[0]!='>')
            {
                if(temp_word[0]==';')
                    push_op(&operator_stack, op_seq);
                else if(temp_word[0]=='&')
                    push_op(&operator_stack,op_and);
                else if(temp_word[0]=='|'&&temp_word[1]=='|')
                    push_op(&operator_stack,op_or);
                else if(temp_word[0]=='|')
                    push_op(&operator_stack, op_pipe);
                j++;
                continue;
            }
            
            //4.operator and operator stack not empty
            if(isOperator(temp_word[0])&&temp_word[0]!='('&&temp_word[0]!='<'&&temp_word[0]!='>'&&temp_word[0]!=')'&&operator_stack.op_empty==0)
            {
                
                //(1);
                if(temp_word[0]==';')
                {
                    while(operator_stack.op_empty==0&&top_op(&operator_stack)!=op_sub_left)
                    {
                        command_t new_command  = (command_t) malloc(sizeof(struct command));
      enum operator_type temp = pop_op(&operator_stack);
                        command_t c1 = pop_cmd(&command_stack);
                        command_t c2 = pop_cmd(&command_stack);
                        switch(temp)
                        {
                            case op_seq:
                                new_command->type=SEQUENCE_COMMAND;
                                break;
                            case op_and:
                                new_command->type=AND_COMMAND;
                                break;
                            case op_or:
                                new_command->type=OR_COMMAND;
                                break;
                            case op_pipe:
                                new_command->type=PIPE_COMMAND;
                                break;
                        }
                        new_command->u.command[0]=c2;
                        new_command->u.command[1]=c1;
                        push_cmd(&command_stack, new_command);
                       
                    }
     push_op(&operator_stack, op_seq);
                }
                //(2)&& and ||
                else if(temp_word[0]=='&'||(temp_word[0]=='|'&&temp_word[1]=='|'))
                {
      while(operator_stack.op_empty==0&&top_op(&operator_stack)!=op_sub_left&&top_op(&operator_stack)!=op_seq)
                    {
                        command_t new_command = (command_t) malloc(sizeof(struct command));
                        enum operator_type temp = pop_op(&operator_stack);
                        command_t c1 = pop_cmd(&command_stack);
                        command_t c2 = pop_cmd(&command_stack);
                        switch(temp)
                        {
                            case op_and:
                                new_command->type=AND_COMMAND;
                                break;
                            case op_or:
                                new_command->type=OR_COMMAND;
                                break;
                            case op_pipe:
                                new_command->type=PIPE_COMMAND;
                                break;
                        }
                        new_command->u.command[0]=c2;
                        new_command->u.command[1]=c1;
                        push_cmd(&command_stack, new_command);

                    }
                        if(temp_word[0]=='&'){
        push_op(&operator_stack,op_and);
      }
                        else{
                            push_op(&operator_stack,op_or);
      }
                }
                //(3)|pipe
                else if(temp_word[0]=='|')
                {
                    if(operator_stack.op_empty==0&&top_op(&operator_stack)==op_pipe)
                    {
                        command_t new_command = (command_t) malloc(sizeof(struct command));
                        enum operator_type temp = pop_op(&operator_stack);
                        command_t c1 = pop_cmd(&command_stack);
                        command_t c2 = pop_cmd(&command_stack);
                        new_command->type=PIPE_COMMAND;
                        new_command->u.command[0]=c2;
                        new_command->u.command[1]=c1;
                        push_cmd(&command_stack,new_command);
                    }
                    push_op(&operator_stack,op_pipe);
                }
                j++;
                continue;
            }
            
            //5.")"
            if(temp_word[0]==')')
            { if(top_op(&operator_stack)!=op_sub_left)
                {while(top_op(&operator_stack)!=op_sub_left)
                {
                    command_t new_command = (command_t) malloc(sizeof(struct command));
                    enum operator_type temp = pop_op(&operator_stack);
        
                    command_t c1 = pop_cmd(&command_stack);
                    command_t c2 = pop_cmd(&command_stack);
                    switch(temp)
                    {
                        case op_seq:
                            new_command->type=SEQUENCE_COMMAND;
                            break;
                        case op_and:
                            new_command->type=AND_COMMAND;
                            break;
                        case op_or:
                            new_command->type=OR_COMMAND;
                            break;
                        case op_pipe:
                            new_command->type=PIPE_COMMAND;
                            break;
                    }
                    new_command->u.command[0]=c2;
                    new_command->u.command[1]=c1;
                    push_cmd(&command_stack, new_command);
                }}
    else
    { 
      pop_op(&operator_stack);
    }
                command_t ncommand=(command_t)malloc(sizeof(struct command));
                ncommand->type=SUBSHELL_COMMAND;
                ncommand->u.subshell_command=pop_cmd(&command_stack);
    push_cmd(&command_stack, ncommand);
                j++;
                continue;
            }
            
            //6.">"and"<",input and output
            if(temp_word[0]=='>'||temp_word[0]=='<')
            {
                char tempc=temp_word[0];
                j++;
                if(tempc=='>')
                    top_cmd(&command_stack)->output=trees[t][j];
                else
                    top_cmd(&command_stack)->input=trees[t][j];
                j++;
                continue;
                
            }
            
            
        }//while loop end
        
        //pop out all remaing operators
        while(operator_stack.op_empty==0)
        {
    command_t new_command=(command_t)malloc(sizeof(struct command));
            enum operator_type temp = pop_op(&operator_stack);
            command_t c1 = pop_cmd(&command_stack);
            command_t c2 = pop_cmd(&command_stack);
            switch(temp)
            {
                case op_seq:
                    new_command->type=SEQUENCE_COMMAND;
                    break;
                case op_and:
                    new_command->type=AND_COMMAND;
                    break;
                case op_or:
                    new_command->type=OR_COMMAND;
                    break;
                case op_pipe:
                    new_command->type=PIPE_COMMAND;
                    break;
            }
            new_command->u.command[0]=c2;
            new_command->u.command[1]=c1;
            push_cmd(&command_stack, new_command);
        }
        
  
        
        struct command_node *temp_node=(struct command_node*) malloc(sizeof(struct command_node));
  
        temp_node->command=(pop_cmd(&command_stack));
        temp_node->next=NULL;
  if(cmd_stream->head == NULL){
    cmd_stream->head = temp_node;
    cmd_stream->tail = temp_node;
  }
  else{
    cmd_stream->tail->next = temp_node;
    cmd_stream->tail = cmd_stream->tail->next;
  }
  
    }
    cmd_stream->cursor = cmd_stream->head;
    return cmd_stream;

}

command_t
read_command_stream (command_stream_t s)
{
    
    if(s->cursor==NULL)
        return NULL;
    command_t temp = s->cursor->command;
    s->cursor=s->cursor->next;
    return temp;
    
}
