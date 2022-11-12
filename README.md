# LameCC
**A lame c compiler which implements a basic lexer, an LR(1) parser and a recursive descent parser**
## Basic features
+ **Lexer &#9745;**
+ **LR(1) Parser &#9745;**
+ **Recursive Descent Parser &#9745;**
+ **Semantic Analysis &#9744; // TODO**
+ **Intermediate Code Generator &#9744; // TODO**
+ **Code Optimization &#9744; // TODO**
+ **Assembly Generator &#9744; // TODO**
## Miscellaneous features
+ **Prettified json dump**
+ **Log info/error**
+ **Visualized LR(1) canonical collection, ACTION GOTO table and LR(1) parsing process**
+ **Other features**
## Build prerequisites
+ **OS: Windows or GNU/Linux**
+ **Cmake version >= 3.8**
+ **If you are running Windows and have installed MinGW64, simply run `build.bat`**
## Parsing capability
+ **`int/float` var declaration/definition**
+ **`if-else` statement**
+ **`int/float/void` function declaration/definition**
+ **`while` statement**
+ **value statement(complex expression, function call, etc...)**
+ **`return` statement**
## Usage
Example input source file(see `./testcases/test.cpp`):
```cpp
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
int NonVoidFuncDefWithParamsWithEmptyBody(int param1, char param2)
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
```
Command options:
```
PS D:\Projects\CPP\Homework\LameCC\build> .\LameCC.exe -?
Usage:
  LameCC.exe <input file> [options]
Available options:
  -?, --help         show all available options
  -o, --out          set output file path
  -T, --dump-tokens  dump tokens in json format
  -A, --dump-ast     dump AST Nodes in json format
      --LR1          specify grammar with a json file and use LR(1) parser
      --log          print LR(1) parsing process
```
Run command:
```
PS D:\Projects\CPP\Homework\LameCC\build> .\LameCC.exe ../testcases/test.cpp -A -T --LR1 ../src/grammar.gram --log
```
Token dump:
```json
[
  {
    "id": 1,
    "type": "TOKEN_KWINT",
    "content": "int",
    "position": [
      2,
      1
    ]
  },
  {
    "id": 2,
    "type": "TOKEN_IDENTIFIER",
    "content": "NonVoidFuncDeclWithParams",
    "position": [
      2,
      5
    ]
  },
  {
    "id": 3,
    "type": "TOKEN_LPAREN",
    "content": "(",
    "position": [
      2,
      30
    ]
  },
  {
    "id": 4,
    "type": "TOKEN_KWINT",
    "content": "int",
    "position": [
      2,
      31
    ]
  },
  ...
```
AST dump:
```json
{
  "type": "TranslationUnitDecl",
  "children": [
    {
      "type": "FunctionDecl",
      "functionType": "int(int, int)",
      "name": "NonVoidFuncDeclWithParams",
      "params": [
        {
          "type": "ParmVarDecl",
          "name": "parm1"
        },
        {
          "type": "ParmVarDecl",
          "name": "parm2"
        }
      ],
      "body": "empty"
    },
    {
      "type": "FunctionDecl",
      "functionType": "char()",
      "name": "NonVoidFuncDeclWithoutParams",
      "params": [],
      "body": "empty"
    },
    {
      "type": "FunctionDecl",
      "functionType": "float()",
      "name": "NonVoidFuncDefWithoutParamsWithEmptyBody",
      "params": [],
      "body": [
        {
          "type": "CompoundStmt",
          "children": [
            {
              "type": "ReturnStmt",
              "value": [
                {
                  "type": "FloatingLiteral",
                  "value": "5.494911"
                }
              ]
            }
          ]
        }
      ]
    },
    ...
```
LR(1) Canonical Collections:  
![image](https://user-images.githubusercontent.com/74029782/201459552-b618b6cf-a947-4ea0-8bd2-84cfb20fd7a2.png)  
ACTION GOTO Table:  
![image](https://user-images.githubusercontent.com/74029782/201459597-d13e2420-b846-4172-bd68-cf56a0d243ff.png)  
Parsing Process:  
![image](https://user-images.githubusercontent.com/74029782/201459611-6c699f47-4030-4cdd-b789-12a3f775c19a.png)
## Credit
+ https://github.com/rui314/8cc
+ https://github.com/Maoyao233/ToyCC
+ https://github.com/nlohmann/json
+ https://github.com/Fytch/ProgramOptions.hxx



