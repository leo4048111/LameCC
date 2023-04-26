# LameCC
**A lame c compiler which implements a basic lexer, an LR(1) parser ,a recursive descent parser and LLVM IR generator.**
## Download this project
`git clone --recurse-submodules https://github.com/leo4048111/LameCC`
## Basic features
+ **Lexer &#9745;**
+ **LR(1) Parser &#9745;**
+ **Recursive Descent Parser &#9745;**
+ **Semantic Analysis &#9745;**
+ **Intermediate Code Generator(in both Quaternion and LLVM IR forms) &#9745;**
+ **Code Optimization &#9745;**
+ **Assembly Generator &#9745;**
## Miscellaneous features
+ **Prettified json dump**
+ **Log info/error**
+ **Visualized LR(1) canonical collection, ACTION GOTO table and LR(1) parsing process**
+ **Other features**
## Build prerequisites
+ **OS: Windows or GNU/Linux**
+ **Cmake version >= 3.8**
+ **Installed LLVM libraries and cpp headers, make sure you have set `CMAKE_PREFIX_PATH` or `LLVM_DIR` env variable to LLVM directory properly**
+ **If you are running Windows and have installed MinGW64, simply run `build.bat`**
## Parsing capability
+ **function definitions/local & extern declarations**
+ **`int/float/char` var declaration/definition**
+ **`if-else` statement**
+ **`int/float/void/char` function declaration/definition**
+ **`while` statement**
+ **value statement(complex expression, function call, etc...)**
+ **`return` statement**
+ **GCC dialect `__asm__` statement**
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
LLVM IR:
```
; ModuleID = 'LCC_LLVMIRGenerator'
source_filename = "LCC_LLVMIRGenerator"

declare i32 @NonVoidFuncDeclWithParams(i32, i32)

declare i8 @NonVoidFuncDeclWithoutParams()

define float @NonVoidFuncDefWithoutParamsWithEmptyBody() {
entry:
  %retVal = alloca float, align 4
  store float 0x4015FACA00000000, ptr %retVal, align 4
  br label %return

return:                                           ; preds = %entry
  %0 = load float, ptr %retVal, align 4
  ret float %0
}

define i32 @NonVoidFuncDefWithParamsWithEmptyBody(i32 %param1, i32 %param2) {
entry:
  %param22 = alloca i32, align 4
  %param11 = alloca i32, align 4
  %retVal = alloca i32, align 4
  store i32 %param1, ptr %param11, align 4
  store i32 %param2, ptr %param22, align 4
  store i32 0, ptr %retVal, align 4
  br label %return

return:                                           ; preds = %entry
  %0 = load i32, ptr %retVal, align 4
  ret i32 %0
}

declare void @VoidFuncDeclWithParams(i32, i32)

declare void @VoidFuncDeclWithoutParams()

define void @VoidFuncDefWithoutParamsWithEmptyBody() {
entry:
  br label %return

return:                                           ; preds = %entry
  ret void
}

define void @VoidFuncDefWithParamsWithEmptyBody(i32 %param1, i32 %param2) {
entry:
  %param22 = alloca i32, align 4
  %param11 = alloca i32, align 4
  store i32 %param1, ptr %param11, align 4
  store i32 %param2, ptr %param22, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() {
entry:
  %mid = alloca i32, align 4
  %target = alloca i32, align 4
  %right = alloca i32, align 4
  %left = alloca i32, align 4
  %retVal = alloca i32, align 4
  store i32 0, ptr %left, align 4
  store i32 100, ptr %right, align 4
  %calltmp = call i32 @NonVoidFuncDefWithParamsWithEmptyBody(i32 99, i32 100)
  %remtmp = srem i32 %calltmp, 2
  %addtmp = add i32 %remtmp, 5
  %0 = load i32, ptr %right, align 4
  %1 = load i32, ptr %left, align 4
  %multmp = mul i32 %0, %1
  %subtmp = sub i32 %addtmp, %multmp
  store i32 %subtmp, ptr %target, align 4
  br label %while.cond

while.cond:                                       ; preds = %if.end, %entry
  %2 = load i32, ptr %left, align 4
  %3 = load i32, ptr %right, align 4
  %lttmp = icmp slt i32 %2, %3
  br i1 %lttmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %4 = load i32, ptr %left, align 4
  %5 = load i32, ptr %right, align 4
  %addtmp1 = add i32 %4, %5
  %divtmp = sdiv i32 %addtmp1, 2
  store i32 %divtmp, ptr %mid, align 4
  %6 = load i32, ptr %mid, align 4
  %7 = load i32, ptr %target, align 4
  %eqtmp = icmp eq i32 %6, %7
  br i1 %eqtmp, label %if.body, label %if.else

if.body:                                          ; preds = %while.body
  %8 = load i32, ptr %mid, align 4
  store i32 %8, ptr %retVal, align 4
  br label %if.end

if.else:                                          ; preds = %while.body
  %9 = load i32, ptr %mid, align 4
  %10 = load i32, ptr %target, align 4
  %lttmp2 = icmp slt i32 %9, %10
  br i1 %lttmp2, label %if.body5, label %if.else4

if.body5:                                         ; preds = %if.else
  %11 = load i32, ptr %mid, align 4
  %addtmp6 = add i32 %11, 1
  %12 = load i32, ptr %left, align 4
  store i32 %addtmp6, ptr %left, align 4
  br label %if.end3

if.else4:                                         ; preds = %if.else
  %13 = load i32, ptr %mid, align 4
  %14 = load i32, ptr %right, align 4
  store i32 %13, ptr %right, align 4
  br label %if.end3

if.end3:                                          ; preds = %if.else4, %if.body5
  br label %if.end

if.end:                                           ; preds = %if.end3, %if.body
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %15 = load i32, ptr %left, align 4
  store i32 %15, ptr %retVal, align 4
  br label %return

return:                                           ; preds = %while.end
  %16 = load i32, ptr %retVal, align 4
  ret i32 %16
}
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



