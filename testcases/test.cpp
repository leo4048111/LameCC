// nonvoid return type function decl with params
int NonVoidFuncDeclWithParams(int parm1, int parm2);

// nonvoid return type function decl without params
char NonVoidFuncDeclWithoutParams();

// nonvoid return type function definition with params
float NonVoidFuncDefWithoutParamsWithEmptyBody()
{
    return 0xAF.D65P-5; // some float representations
}

// nonvoid return type function definition with params
int NonVoidFuncDefWithParamsWithEmptyBody(int param1, int param2)
{
    return 0;
}

// void return type function decl with params
void VoidFuncDeclWithParams(int parm1, int parm2);

// nonvoid return type function decl without params
void VoidFuncDeclWithoutParams();

// void return type function definition with params with empty body
void VoidFuncDefWithoutParamsWithEmptyBody()
{
}

// void return type function definition with params with empty body
void VoidFuncDefWithParamsWithEmptyBody(int param1, int param2)
{
}

// function definition
int main()
{
    int left = 0;                                                                         // DeclStmt
    int right = 100;                                                                      // DeclStmt
    int target = (NonVoidFuncDefWithParamsWithEmptyBody(99, 100) % 2 + 5) - right * left; // complex Expression

    while (left < right) // WhileStmt
    {
        int mid = (left + right) / 2;
        if (mid == target)     // IfStmt
            return mid;        // ReturnStmt
        else if (mid < target) // elseBody which is another IfStmt
            left = mid + 1;    // ValueStmt
        else                   // elseBody
            right = mid;       // ValueStmt
    }

    return left; // ReturnStmt
}