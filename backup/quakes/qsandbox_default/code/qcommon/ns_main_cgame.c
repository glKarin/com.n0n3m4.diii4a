// Массив с названиями функций
const char *function_list[MAX_FUNCS] = {
    "print",
    "command",
    "createThread",
    "deleteThread",
    "getCvar",
    "setCvar",
    "createVariable",
    "getVariable",
    "setVariable",
    "sendVariable",
    "notify",
    NULL
};

// Список типов аргументов для каждой функции
const char *function_arg_types[MAX_FUNCS][MAX_ARGS] = {
    {"char"},                                       // print
    {"char", "char"},                               // command
    {"char", "char", "int"},                        // createThread
    {"char"},                                       // deleteThread
    {"char"},                                       // getCvar
    {"char", "char"},                               // setCvar
    {"char", "char", "char"},                       // createVariable
    {"char"},                                       // getVariable
    {"char", "char", "char"},                       // setVariable
    {"char", "char"},                               // sendVariable
    {"char", "int"},                                // notify
    {NULL}                                          // NULL
};

// Функция для поиска индекса функции по её названию
int funcforindex(const char *name) {
    int i;
    for (i = 0; i < MAX_FUNCS; i++) {
        if (strcmp(function_list[i], name) == 0) {
            return i; // Возвращаем индекс, если найдено совпадение
        }
    }
    return -1; // Если функция не найдена, возвращаем -1
}

char stringArgsBuffer[MAX_ARGS][MAX_ARG_LENGTH]; // Для хранения строковых аргументов

void callfunc(Variable *var, const char *name, const char *operation, const char *args) {
    VarValue result;
    qboolean hasReturnValue = qfalse;
    int argCount;
    char cleanedArgs[MAX_ARG_LENGTH]; // Для хранения очищенных аргументов
    int i;

    //Com_Printf("Noire.Script Debug: %s() args %s\n", name, args);

    // Удаляем внешние скобки и копируем в статический массив
    strncpy(cleanedArgs, removeOuterBrackets(args), MAX_ARG_LENGTH - 1);
    cleanedArgs[MAX_ARG_LENGTH - 1] = '\0'; // Завершаем строку

    // Разделяем аргументы
    argCount = splitArgs(cleanedArgs, stringArgsBuffer);

    for (i = 0; i < argCount; i++) {
        const char* expectedType = function_arg_types[funcforindex(name)][i];
        // Применяем NS_Exp к int и float, и NS_Text к char
        if (strcmp(expectedType, "int") == 0) {
            ns_args[i].i = (int)NS_Exp(stringArgsBuffer[i]);
        } else if (strcmp(expectedType, "float") == 0) {
            ns_args[i].f = NS_Exp(stringArgsBuffer[i]);
        } else {
            NS_Text(stringArgsBuffer[i], ns_args[i].c, sizeof(ns_args[i].c));
        }
    }

    if (strcmp(name, "print") == 0 && argCount >= 1) {
        Com_Printf("%s\n", ns_args[0].c);
    }

    else if (strcmp(name, "command") == 0 && argCount >= 1) {        
        trap_SendConsoleCommand(ns_args[0].c);
    }

    else if (strcmp(name, "createThread") == 0 && argCount >= 3) {
        NS_OpenScript(ns_args[0].c, ns_args[1].c, ns_args[2].i);
    }

    else if (strcmp(name, "deleteThread") == 0 && argCount >= 1) {
        RemoveScriptThread(ns_args[0].c);
    }

    else if (strcmp(name, "getCvar") == 0 && argCount >= 1) {
        NS_getCvar(&result, var->type, ns_args[0].c);
        hasReturnValue = qtrue;
    }

    else if (strcmp(name, "setCvar") == 0 && argCount >= 2) {
        NS_setCvar(ns_args[0].c, ns_args[1].c);
    }

    else if (strcmp(name, "createVariable") == 0 && argCount >= 3) {
        VarType type = (!strcmp(ns_args[2].c, "TYPE_CHAR")) ? TYPE_CHAR :
                        (!strcmp(ns_args[2].c, "TYPE_INT")) ? TYPE_INT : TYPE_FLOAT;
        create_variable(ns_args[0].c, ns_args[1].c, type);
    }

    else if (strcmp(name, "getVariable") == 0 && argCount >= 1) {
        if(var->type == TYPE_CHAR){
            strncpy(result.c, get_variable_char(ns_args[0].c), sizeof(result.c));
        }
        if(var->type == TYPE_INT){
            result.i = get_variable_int(ns_args[0].c);
        }
        if(var->type == TYPE_FLOAT){
            result.f = get_variable_float(ns_args[0].c);
        }
        hasReturnValue = qtrue;
    }

    else if (strcmp(name, "setVariable") == 0 && argCount >= 3) {
        VarType type = (!strcmp(ns_args[2].c, "TYPE_CHAR")) ? TYPE_CHAR :
                        (!strcmp(ns_args[2].c, "TYPE_INT")) ? TYPE_INT : TYPE_FLOAT;
        set_variable_value(ns_args[0].c, ns_args[1].c, type);
    }

    else if (strcmp(name, "sendVariable") == 0 && argCount >= 2) {   
        Variable *sendVar = find_variable(ns_args[1].c); 

    if(!strcmp(ns_args[0].c, "server")){
        if(sendVar->type == TYPE_CHAR){
        trap_SendConsoleCommand(va("ns_sendvariable %s %s %i", ns_args[1].c, sendVar->value.c, TYPE_CHAR));
        }
        if(sendVar->type == TYPE_INT){
        trap_SendConsoleCommand(va("ns_sendvariable %s %i %i", ns_args[1].c, sendVar->value.i, TYPE_INT));
        }
        if(sendVar->type == TYPE_FLOAT){
        trap_SendConsoleCommand(va("ns_sendvariable %s %f %i", ns_args[1].c, sendVar->value.f, TYPE_FLOAT));
        }
    }
    if(!strcmp(ns_args[0].c, "ui")){
        if(sendVar->type == TYPE_CHAR){
        trap_SendConsoleCommand(va("ns_sendvariable_ui %s %s %i", ns_args[1].c, sendVar->value.c, TYPE_CHAR));
        }
        if(sendVar->type == TYPE_INT){
        trap_SendConsoleCommand(va("ns_sendvariable_ui %s %i %i", ns_args[1].c, sendVar->value.i, TYPE_INT));
        }
        if(sendVar->type == TYPE_FLOAT){
        trap_SendConsoleCommand(va("ns_sendvariable_ui %s %f %i", ns_args[1].c, sendVar->value.f, TYPE_FLOAT));
        }
    }

    }

    else if (strcmp(name, "notify") == 0 && argCount >= 2) {
        CG_AddNotify(ns_args[0].c, ns_args[1].i);
    }

    if (hasReturnValue) {
        set_variable(var, result, operation);
    }
}
