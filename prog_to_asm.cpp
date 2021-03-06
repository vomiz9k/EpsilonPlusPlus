
#include "musthave_include.h"
#include "header.h"

#define make_it_to_assm( word )              \
    else if (!strcmp(operation, #word))   \
            {                                \
                ##word_to_assm(branch, assm);\
            }


int STARTING_POINT = 0;

const int MAX_VAR_COUNT = 1000;
char VARIABLE_INDEXES[MAX_VAR_COUNT] = {};

int FREE_VARIABLE = 0;
int CONDITION_COUNTER = 0;

int VAR_PER_FUNC = 100;

int work_with_code()
{
    system("py translate.py");
    poem code = files_to_poem("output.txt", "output.bin");
    error_on_line(0, &code);
    tree_branch** tokinezated_code = tokenizate_code(code);

    //int i = 0;
    //while(tokinezated_code[i] != 0)
        //printf("Token %d: %s\n", ++i, tokinezated_code[i]->data);

    tree_branch* branch = code_get_G(tokinezated_code);

    if (!branch)
        return 0;

    tree_to_asm(branch);
    char *fin_name = "assm.txt", *fout_name = "output.bin";

    int err_code = get_binary_file(fin_name, fout_name);

    if (!err_code)
        err_code = run_binary_file(fout_name);

    return err_code;
}


int tree_to_asm(tree_branch* branch)
{
    FILE* assm = fopen("assm.txt", "w");
    fprintf(assm, "CALL MAIN\n");
    recursive_asm(branch, assm);
    fclose(assm);
}

int recursive_asm(tree_branch* branch, FILE* assm)
{

    switch(branch->type)
    {
        case TREE_HEAD:
        {
            for (int i = 0; i < branch->kids; ++i)
                recursive_asm(branch->next[i], assm);
        }
        break;

        case TREE_MAIN:
            main_to_assm(branch, assm);
        break;

        case TREE_FUNCTION_DEFINE:
             func_define_to_assm(branch, assm);
        break;

        case TREE_FUNCTION:
            func_call_to_assm(branch, assm);
        break;

        case TREE_INTEGER:
            fprintf(assm, "PUSH %0.02lf\n", *(double*)branch->data);
        break;


        case TREE_VAR:
            fprintf(assm, "PUSH ax\nPUSH %d\nADD\nPOP bx\n", get_var_index(*(char*)branch->data));
            fprintf(assm, "PUSH [bx]\n");
        break;

        case TREE_STRING:
            fprintf(assm, "\"%s\"", branch->data);
        break;

        case TREE_VAR_DEFINE:
            variable_define_to_assm(branch, assm);
        break;

        case TREE_OPERATOR:
        {
            char* operation = (char*)branch->data;

            if(!strcmp(operation, "+") || !strcmp(operation, "*"))
            {
                math_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "^"))
            {
                pow_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "="))
            {
                assign_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "print"))
            {
                print_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "scan"))
            {
                scan_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "if"))
            {
                if_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "while"))
            {
                while_to_assm(branch, assm);
            }
            else if (!strcmp(operation, "return"))
            {
                return_to_assm(branch, assm);
            }
        }
    }
}

int return_to_assm(tree_branch* branch, FILE* assm)
{
    for(int i = 0; i < branch->kids; ++i)
        recursive_asm(branch->next[i], assm);
    fprintf(assm, "PUSH ax\nPUSH %d\nSUB\nPOP ax\n", VAR_PER_FUNC);
    fprintf(assm, "RETURN\n");

}

int while_to_assm(tree_branch* branch, FILE* assm)
{
    int curr_num = CONDITION_COUNTER;
    fprintf(assm, "WHILE%d:\n", curr_num);
    recursive_asm(branch->next[0]->next[0], assm);
    recursive_asm(branch->next[0]->next[1], assm);
    print_condition((char*)branch->next[0]->data, assm);
    fprintf(assm, "JMP ENDWHILE%d\n", curr_num);
    fprintf(assm, "%d:\n", curr_num);
    recursive_asm(branch->next[1], assm);
    fprintf(assm, "JMP WHILE%d\n", curr_num);
    fprintf(assm, "ENDWHILE%d:\n", curr_num);
}

int if_to_assm(tree_branch* branch, FILE* assm)
{
    recursive_asm(branch->next[0]->next[0], assm);
    recursive_asm(branch->next[0]->next[1], assm);
    print_condition((char*)branch->next[0]->data, assm);
    int go_to = CONDITION_COUNTER - 1;
    if (branch->kids == 3)
        recursive_asm(branch->next[2], assm);
    fprintf(assm, "JMP ENDIF%d\n", go_to);
    fprintf(assm, "%d:\n", go_to);

    recursive_asm(branch->next[1], assm);
    fprintf(assm, "ENDIF%d:\n", go_to);
}

int scan_to_assm(tree_branch* branch, FILE* assm)
{
    fprintf(assm, "IN\n");
    fprintf(assm, "PUSH ax\nPUSH %d\nADD\n", get_var_index(*(char*)branch->next[0]->data));
    fprintf(assm, "POP bx\n");
    fprintf(assm, "POP [bx]\n");
}

int print_to_assm(tree_branch* branch, FILE* assm)
{
    if (branch->next[0]->type == TREE_STRING)
    {
        fprintf(assm, "OUT \"%s\"\n", branch->next[0]->data);
    }
    else
    {
        recursive_asm(branch->next[0], assm);
        fprintf(assm, "OUT\n");
    }
}

int assign_to_assm(tree_branch* branch, FILE* assm)
{
    recursive_asm(branch->next[1], assm);
    fprintf(assm, "PUSH ax\nPUSH %d\nADD\n", get_var_index(*(char*)branch->next[0]->data));
    fprintf(assm, "POP bx\n");
    fprintf(assm, "POP [bx]\n");
}

int variable_define_to_assm(tree_branch* branch, FILE* assm)
{
    if(branch->next[0]->type == TREE_OPERATOR && *(char*)branch->next[0]->data == '=')
    {
        VARIABLE_INDEXES[FREE_VARIABLE] = *(char*)branch->next[0]->next[0]->data;
        recursive_asm(branch->next[0], assm);
    }
    else
    {
        VARIABLE_INDEXES[FREE_VARIABLE] = *(char*)branch->next[0]->data;
    }
    FREE_VARIABLE++;
}

int pow_to_assm(tree_branch* branch, FILE* assm)
{
    for(int i = 0; i < branch->kids; ++i)
    {
        recursive_asm(branch->next[i], assm);
    }

    for(int i = 0; i < branch->kids - 1; ++i)
    {
        fprintf(assm, "POW\n");
    }
}


int math_to_assm(tree_branch* branch, FILE* assm)
{
    char* operation = (char*)branch->data;
    bool adding = false;
    char assm_pos_op[10] = {};
    char assm_neg_op[10] = {};
    if (*operation == '+')
    {
        adding = true;
        strcpy(assm_pos_op, "ADD\n");
        strcpy(assm_neg_op, "SUB\n");
    }
        else
    {
        strcpy(assm_pos_op, "PROD\n");
        strcpy(assm_neg_op, "DIV\n");
    }

    for(int i = 1; i < branch->kids; ++i)
    {
        recursive_asm(branch->next[i], assm);
    }

    for(int i = 1; i < branch->kids - 1; ++i)
    {
        fprintf(assm, "%s", assm_pos_op);
    }

    if(branch->next[0])
    {
        for(int i = 0; i < branch->next[0]->kids; ++i)
        {
            recursive_asm(branch->next[0]->next[i], assm);
            fprintf(assm, "%s", assm_neg_op);
        }
    }
}

int func_call_to_assm(tree_branch* branch, FILE* assm)
{
    for(int i = 0; i < branch->kids; ++i)
    {
        recursive_asm(branch->next[branch->kids - i - 1], assm);
    }

    if(is_my_function(branch))
    {
        fprintf(assm, "CALL %s\n", branch->data);
    }
    else
    {
        char* str = (char*)branch->data;
        for (str; *str != 0; ++str)
            fprintf(assm, "%c", *str - 'a' + 'A');
        fprintf(assm, "\n");
    }
}

int main_to_assm(tree_branch* branch, FILE* assm)
{
    fprintf(assm, "MAIN:\n");
    fprintf(assm, "PUSH 0\n");
    fprintf(assm, "POP ax\n");
    for(int i = 0; i < branch->kids; ++i)
        recursive_asm(branch->next[i], assm);
    fprintf(assm, "EXIT\n");
}

int func_define_to_assm(tree_branch* branch, FILE* assm)
{
    if(!strcmp(((func_def*)(branch->data))->name, "do"))
    {
        for (int i = 0; i < branch->kids; ++i)
            recursive_asm(branch->next[i], assm);
        return 0;
    }
    fprintf(assm, "%s:\n", ((func_def*)(branch->data))->name);
    fprintf(assm, "PUSH ax\nPUSH %d\nADD\nPOP ax\n", VAR_PER_FUNC);
    int i = 0;
    while(branch->next[i]->type == TREE_ARG)
    {
        fprintf(assm, "PUSH ax\nPUSH %d\nADD\nPOP bx\nPOP [bx]\n", get_var_index(*(char*)branch->next[i]->data));
        i++;
    }
    for(; i < branch->kids; ++i)
        recursive_asm(branch->next[i], assm);
    fprintf(assm, "RETURN\n");
}

int get_var_index(char var)
{
    for(int i = STARTING_POINT; i < MAX_VAR_COUNT; ++i)
        if (VARIABLE_INDEXES[i] == var)
            return i;
    return 0;
}


int print_condition(char* condition, FILE* assm)
{
    if (!strcmp(condition, ">="))
        fprintf(assm, "JAE %d\n", CONDITION_COUNTER);
    else if (!strcmp(condition, ">"))
        fprintf(assm, "JA %d\n", CONDITION_COUNTER);
    else if (!strcmp(condition, "<"))
        fprintf(assm, "JB %d\n", CONDITION_COUNTER);
    else if (!strcmp(condition, "<="))
        fprintf(assm, "JBE %d\n", CONDITION_COUNTER);
    else if (!strcmp(condition, "=="))
        fprintf(assm, "JE %d\n", CONDITION_COUNTER);
    else if (!strcmp(condition, "!="))
        fprintf(assm, "JNE %d\n", CONDITION_COUNTER);
    else

        printf("CONDITION ERROR\n");
    CONDITION_COUNTER++;
}
