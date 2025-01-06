/*
===========================================================================
// Project: Quake Sandbox - Noire.Script
// File: ns_main.c
// Description: Noire.Script (NS) is a lightweight scripting 
//              language designed for Quake Sandbox. It enables 
//              dynamic interaction with game logic, UI, and 
//              server-side functionality, offering flexibility 
//              in modding and gameplay customization.
// Features: - Support for game events and triggers
//           - Integration with game entities and UI
//           - Easy-to-write syntax for creating complex behaviors
//           - Modular structure for server and client-side scripts
===========================================================================
*/

#include "ns_local.h"

/*
###############
Парсинг
###############
*/

// Список операторов для парсера
const char* operators[] = {
    "=", "+", "-", "*", "/", "==", "!=", "<", ">", "<=", ">=", "+=", "-=", "/=", "*="
};
const int num_operators = sizeof(operators) / sizeof(operators[0]);

int is_operator(const char* token) {
    int i;

    for (i = 0; i < num_operators; i++) {
        if (strcmp(token, operators[i]) == 0) {
            return 1; // Это оператор
        }
    }
    return 0; // Не оператор
}

int is_literal(const char* token) {
    int i;

    if (token[0] == '"' || token[0] == '\'') {
        return 1; // Это литерал
    }
    return 0; // Не литерал
}

char* NS_Parse(char** p) {
    char* s = *p;
    static char result[MAX_TOKEN_LENGTH]; // Статический буфер для токенов
    size_t len;
    char* token;

    // Пропустим пробелы
    while (*s) {
        // Пропустим \t
        while (*s && (*s == ' ' || *s == '\t')) {
            s++;
        }

        // Проверяем одно-строчный комментарий
        if (s[0] == '/' && s[1] == '/') {
            while (*s && *s != '\n') { // Пропустим до конца строки
                s++;
            }
            continue; // Идем дальше собирать токены
        }

        // Проверяем много-строчный комментарий
        if (s[0] == '/' && s[1] == '*') {
            s += 2; // Пропускаем '/*'
            while (*s) {
                if (s[0] == '*' && s[1] == '/') {
                    s += 2; // Пропускаем '*/'
                    break; // Идем дальше собирать токены
                }
                s++;
            }
            continue; // Идем дальше собирать токены
        }

        // Проверяем на строковые литералы
        if (*s == '"' || *s == '\'') {
            char quote = *s; // Сохраняем тип кавычек
            char* token = s; // Сохраняем начало токена
            s++; // Пропускаем открывающую кавычку

            // Ищем конец строкового литерала
            while (*s) {
                if (*s == quote && *(s - 1) != '\\') {
                    s++; // Пропускаем закрывающую кавычку
                    break; // Закрывающая кавычка найдена
                }
                s++;
            }

            // Завершаем токен, не изменяя строку
            len = s - token;
            if (len >= MAX_TOKEN_LENGTH) {
                return NULL; // Слишком длинный токен
            }
            strncpy(result, token, len);
            result[len] = '\0';

            *p = s; // Обновляем указатель
            return result; // Возвращаем найденный токен
        }

        // Обработка содержимого скобок
        if (*s == '(') {
            char* token = s; // Сохраняем начало токена
            int depth = 1; // Уровень вложенности
            s++; // Пропускаем открывающую скобку
        
            // Ищем конец содержимого скобок
            while (*s && depth > 0) {
                if (*s == '(') {
                    depth++; // Увеличиваем уровень вложенности при нахождении открывающей скобки
                } else if (*s == ')') {
                    depth--; // Уменьшаем уровень вложенности при нахождении закрывающей скобки
                }
                s++; // Переход к следующему символу
            }
        
            if (depth != 0) {
                return NULL; // Не найдено соответствующей закрывающей скобки
            }
        
            // Завершаем токен, не изменяя строку
            len = s - token; // Длина токена с учетом закрывающей скобки
            if (len >= MAX_TOKEN_LENGTH) {
                return NULL; // Слишком длинный токен
            }
            strncpy(result, token, len);
            result[len] = '\0'; // Завершаем строку
        
            *p = s; // Обновляем указатель
            return result; // Возвращаем найденный токен
        }

        if (*s == '\0') {
            *p = s; // Обновим указатель
            return NULL; // Достигнут конец строки
        }

        // Старт токена
        token = s;

        // Ищем конец токена
        while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != ';' && *s != ',' && *s != '(' && *s != ')') {
            s++;
        }

        if (*s) {
            *s++ = '\0'; // Завершаем токен
        }

        *p = s; // Обновим указатель

        // Возвращаем токен как строку
        return token; // Вернем найденный токен
    }

    *p = s; // Обновим указатель
    return NULL; // Не нашли ничего
}

/*
###############
Переменные
###############
*/

// Массив переменных
Variable variables[MAX_VARS];
int var_count = 0;

// Функция для добавления переменной
int set_variable(Variable *var, VarValue value, const char *operation) {
    if (var == NULL) {
        return 0; // Переменная не может быть NULL
    }

    // Выполняем операцию
    if (strcmp(operation, "=") == 0) {
        var->value = value;
    } else if (strcmp(operation, "+=") == 0) {
        if (var->type == TYPE_INT) {
            var->value.i += value.i;
        } else if (var->type == TYPE_FLOAT) {
            var->value.f += value.f;
        }
    } else if (strcmp(operation, "-=") == 0) {
        if (var->type == TYPE_INT) {
            var->value.i -= value.i;
        } else if (var->type == TYPE_FLOAT) {
            var->value.f -= value.f;
        }
    } else if (strcmp(operation, "*=") == 0) {
        if (var->type == TYPE_INT) {
            var->value.i *= value.i;
        } else if (var->type == TYPE_FLOAT) {
            var->value.f *= value.f;
        }
    } else if (strcmp(operation, "/=") == 0) {
        if (var->type == TYPE_INT) {
            if (value.i == 0) {
                return 0;
            }
            var->value.i /= value.i;
        } else if (var->type == TYPE_FLOAT) {
            if (value.f == 0.0f) {
                return 0;
            }
            var->value.f /= value.f;
        }
    } else {
        Com_Printf("Noire.Script Error: Unknown operator '%s'.\n", operation);
        trap_Cvar_Set("ns_haveerror", "1");
        return 0;
    }

    return 1; // Успешно обновлено
}

// Функция для установки значения
int set_variable_int(Variable *var, int value, const char *operation) {
    if (var == NULL) {
        return 0; // Переменная не может быть NULL
    }

    // Выполняем операцию
    if (strcmp(operation, "=") == 0) {
        var->value.i = value;
    } else if (strcmp(operation, "+=") == 0) {
            var->value.i += value;
    } else if (strcmp(operation, "-=") == 0) {
            var->value.i -= value;
    } else if (strcmp(operation, "*=") == 0) {
            var->value.i *= value;
    } else if (strcmp(operation, "/=") == 0) {
            if (value == 0) {
                return 0;
            }
            var->value.i /= value;
    } else {
        Com_Printf("Noire.Script Error: Unknown operator '%s'.\n", operation);
        trap_Cvar_Set("ns_haveerror", "1");
        return 0;
    }

    return 1; // Успешно обновлено
}

// Функция для отображения переменных
void print_variables() {
    int i;

    if (var_count == 0) {
        Com_Printf("Noire.Script Info: No variables found.\n");
        return;
    }

    Com_Printf("Noire.Script Variables:\n");
    for (i = 0; i < var_count; i++) {
        switch (variables[i].type) {
            case TYPE_CHAR:
                Com_Printf(" - Variable Name = '%s', Type = char, Value = %s\n", variables[i].name, variables[i].value.c);
                break;
            case TYPE_INT:
                Com_Printf(" - Variable Name = '%s', Type = int, Value = %d\n", variables[i].name, variables[i].value.i);
                break;
            case TYPE_FLOAT:
                Com_Printf(" - Variable Name = '%s', Type = float, Value = %.6f\n", variables[i].name, variables[i].value.f);
                break;
            default:
                Com_Printf(" - Variable Name = '%s', Type = INVALID\n", variables[i].name);
                break;
        }
    }
}

// Функция для поиска переменной по имени
Variable* find_variable(const char *name) {
    int i;

    if (name == NULL) {
        return NULL; // Имя переменной не может быть NULL
    }

    for (i = 0; i < var_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return &variables[i];
        }
    }
    return NULL; // Переменная не найдена
}

// Функция для проверки существования переменной по имени
int variable_exists(const char *name) {
    // Используем find_variable для поиска переменной
    if (find_variable(name) != NULL) {
        return 1; // Переменная найдена
    }
    return 0; // Переменная не найдена
}

// Функции для добавления переменных
int add_variable(const char *name, VarValue value, VarType type) {
    int i;

    if (name == NULL) {
        return 0; // Имя переменной не может быть NULL
    }

    if (var_count < MAX_VARS) {
        strncpy(variables[var_count].name, name, MAX_VAR_NAME);
        variables[var_count].name[MAX_VAR_NAME - 1] = '\0'; // Завершаем строку
        variables[var_count].value = value;
        variables[var_count].type = type;
        var_count++;
        return 1; // Успешно добавлено
    }

    Com_Printf("Noire.Script Error: Maximum variable limit reached.\n");
    trap_Cvar_Set("ns_haveerror", "1");
    return 0; // Ошибка: превышено максимальное количество переменных
}

void create_variable(const char *name, const char *value, VarType type) {
    int i;

    if (name == NULL) {
        return;
    }

    if (variable_exists(name)) {
        return; // Имя переменной не может быть NULL
    }

    if (var_count < MAX_VARS) {
        strncpy(variables[var_count].name, name, MAX_VAR_NAME);
        variables[var_count].name[MAX_VAR_NAME - 1] = '\0'; // Завершаем строку
        switch (type) {
            case TYPE_INT:
                variables[var_count].value.i = atoi(value); // Преобразуем строку в int
                break;
            case TYPE_FLOAT:
                variables[var_count].value.f = atof(value); // Преобразуем строку в float
                break;
            case TYPE_CHAR:
                strncpy(variables[var_count].value.c, value, sizeof(variables[var_count].value.c) - 1);
                variables[var_count].value.c[sizeof(variables[var_count].value.c) - 1] = '\0'; // Добавляем завершающий символ
                break;
            default:
                Com_Printf("Noire.Script Error: variable have invalid type.\n");
                trap_Cvar_Set("ns_haveerror", "1");
        }
        variables[var_count].type = type;
        var_count++;
        return;
    }

    Com_Printf("Noire.Script Error: Maximum variable limit reached.\n");
    trap_Cvar_Set("ns_haveerror", "1");
    return;
}

//Функция для установки значения
void set_variable_value(const char *name, const char *value, VarType type) {
    Variable* var;

    var = find_variable(name);
    // Если переменная найдена
    if (var != NULL) {
        switch (type) {
            case TYPE_INT:
                var->value.i = atoi(value); // Преобразуем строку в int
                break;
            case TYPE_FLOAT:
                var->value.f = atof(value); // Преобразуем строку в float
                break;
            case TYPE_CHAR:
                strncpy(var->value.c, value, sizeof(var->value.c) - 1);
                var->value.c[sizeof(var->value.c) - 1] = '\0'; // Добавляем завершающий символ
                break;
            default:
                Com_Printf("Noire.Script Error: variable have invalid type.\n");
                trap_Cvar_Set("ns_haveerror", "1");
        }
        var->type = type;       // Новый тип
    }
    return;
}

// Функции для возвращения значений
int get_variable_int(const char *name) {
    Variable* var;

    var = find_variable(name);
    // Используем find_variable для поиска переменной
    if (var != NULL) {
        return var->value.i; // Int возвращаем
    }
    return 0; // Переменная не найдена
}
float get_variable_float(const char *name) {
    Variable* var;

    var = find_variable(name);
    // Используем find_variable для поиска переменной
    if (var != NULL) {
        return var->value.f; // Float возвращаем
    }
    return 0; // Переменная не найдена
}
char* get_variable_char(const char *name) {
    Variable* var;

    var = find_variable(name);
    // Используем find_variable для поиска переменной
    if (var != NULL) {
        return var->value.c; // Char возвращаем
    }
    return 0; // Переменная не найдена
}

/*
###############
Выражения
###############
*/

float NS_Exp(const char* expr) {
    float result = 0, currentNumber = 0, decimalFactor = 1;
    char operation = '+';
    int i;
    Variable* var;

    for (i = 0; expr[i] != '\0'; ++i) {
        char currentChar = expr[i];

        if (isdigit(currentChar)) {
            currentNumber = currentNumber * 10 + (currentChar - '0');
        } else if (currentChar == '.') {
            decimalFactor = 0.1f; // Сброс decimalFactor
            currentChar = expr[++i];
            while (isdigit(currentChar)) {
                currentNumber += (currentChar - '0') * decimalFactor;
                decimalFactor *= 0.1f;
                currentChar = expr[++i];
            }
            --i; // Уменьшаем индекс
        } else if (isalpha(currentChar)) {
            char varName[MAX_VAR_NAME];
            int j = 0;

            while (isalnum(currentChar) || currentChar == '_') {
                varName[j++] = currentChar;
                currentChar = expr[++i];
            }
            varName[j] = '\0';

            var = find_variable(varName);
            currentNumber = var ? (var->type == TYPE_INT ? (float)var->value.i : var->value.f) : 0;
            --i;
        }

        if (!isdigit(currentChar) && currentChar != ' ' || expr[i + 1] == '\0') {
            switch (operation) {
                case '+': result += currentNumber; break;
                case '-': result -= currentNumber; break;
                case '*': result *= currentNumber; break;
                case '/':
                    if (currentNumber != 0) result /= currentNumber;
                    else {
                        return 0.0;
                    }
                    break;
            }
            currentNumber = 0; // Сброс текущего числа
            decimalFactor = 1; // Сброс decimalFactor
            operation = currentChar; // Обновляем операцию
        }
    }
    return result;
}

int NS_ifResult(int num1, NSOperator operator, int num2) {
    //Com_Printf("Noire.Script Debug: left = %i, operator = %i, right = %i\n", num1, operator, num2);
    switch (operator) {
        case EQUAL:
            return (num1 == num2) ? 1 : 0;
        case NOT_EQUAL:
            return (num1 != num2) ? 1 : 0;
        case LESS_THAN:
            return (num1 < num2) ? 1 : 0;
        case GREATER_THAN:
            return (num1 > num2) ? 1 : 0;
        case LESS_OR_EQUAL:
            return (num1 <= num2) ? 1 : 0;
        case GREATER_OR_EQUAL:
            return (num1 >= num2) ? 1 : 0;
        default:
            return 0; // Возвращаем 0, если оператор не распознан
    }
}

NSOperator NS_CharToOp(const char* operator) {
    if (strcmp(operator, "==") == 0) {
        return EQUAL;
    } else if (strcmp(operator, "!=") == 0) {
        return NOT_EQUAL;
    } else if (strcmp(operator, "<") == 0) {
        return LESS_THAN;
    } else if (strcmp(operator, ">") == 0) {
        return GREATER_THAN;
    } else if (strcmp(operator, "<=") == 0) {
        return LESS_OR_EQUAL;
    } else if (strcmp(operator, ">=") == 0) {
        return GREATER_OR_EQUAL;
    } else {
        // Возвращаем значение по умолчанию или обрабатываем ошибку
        Com_Printf("Noire.Script Error: Unsupported operator: %s\n", operator);
        trap_Cvar_Set("ns_haveerror", "1");
        return -1; // Предполагаем, что -1 не является допустимым значением
    }
}

// Функция для обработки текста с подстановкой значений переменных через $
void NS_Text(const char *input, char *result, int resultSize) {
    int resultIndex = 0; // Index for the resulting string
    char placeholder[MAX_VAR_NAME + 2]; // Placeholder for variable name
    char value[100]; // Buffer for the variable value
    Variable* var;
    int i;
    int k;
    int tempIndex = 0;

    // Create a temporary buffer for processing the string without outer parentheses
    char tempInput[1024];
    int start = 0;
    int end = strlen(input) - 1;

    // Check for outer parentheses
    if (input[0] == '(' && input[end] == ')') {
        start = 1; // Start from the first character after the opening parenthesis
        end--;     // End before the closing parenthesis
    }

    // Copy the string without outer parentheses to tempInput
    for (i = start; i <= end && tempIndex < sizeof(tempInput) - 1; i++) {
        tempInput[tempIndex++] = input[i];
    }
    tempInput[tempIndex] = '\0';

    // Main logic for processing variables
    for (i = 0; tempInput[i] != '\0' && resultIndex < resultSize - 1; i++) {
        if (tempInput[i] == '$') {
            int j = 0;
            i++; // Skip the '$' character

            // Read the variable name until the next '$'
            while (tempInput[i] != '$' && tempInput[i] != '\0' && j < sizeof(placeholder) - 1) {
                placeholder[j++] = tempInput[i++];
            }
            placeholder[j] = '\0'; // Null-terminate the variable name

            if (tempInput[i] == '$') {
                // Found the closing '$'
                //i++; // Skip the closing '$'
            }

            // Find the variable
            var = find_variable(placeholder);
            if (var != NULL) {
                // Use the variable value based on its type
                if (var->type == TYPE_CHAR) {
                    strcpy(value, var->value.c);
                } else if (var->type == TYPE_INT) {
                    Q_snprintf(value, sizeof(value), "%d", var->value.i);
                } else if (var->type == TYPE_FLOAT) {
                    Q_snprintf(value, sizeof(value), "%.6f", var->value.f);
                }

                // Copy the variable value to the result string
                for (k = 0; value[k] != '\0' && resultIndex < resultSize - 1; k++) {
                    result[resultIndex++] = value[k];
                }
            } else {
                // If the variable is not found, just add '$' and the variable name
                result[resultIndex++] = '$';
                for (k = 0; placeholder[k] != '\0' && resultIndex < resultSize - 1; k++) {
                    result[resultIndex++] = placeholder[k];
                }
                result[resultIndex++] = '$'; // Add the closing '$'
            }
        } else {
            result[resultIndex++] = tempInput[i]; // Copy regular characters
        }
    }
    result[resultIndex] = '\0'; // Null-terminate the result string
}

// Функция для обработки текста с подстановкой значений из указателей на переменные через &
void NS_ArgumentText(const char *input, char *result, int resultSize) {
    int resultIndex = 0; // Index for the resulting string
    char placeholder[MAX_VAR_NAME + 2]; // Placeholder for variable name
    char value[100]; // Buffer for the variable value
    Variable* var;
    int i;
    int k;
    int tempIndex = 0;

    // Create a temporary buffer for processing the string without outer parentheses
    char tempInput[1024];
    int start = 0;
    int end = strlen(input) - 1;

    // Check for outer parentheses
    if (input[0] == '(' && input[end] == ')') {
        start = 1; // Start from the first character after the opening parenthesis
        end--;     // End before the closing parenthesis
    }

    // Copy the string without outer parentheses to tempInput
    for (i = start; i <= end && tempIndex < sizeof(tempInput) - 1; i++) {
        tempInput[tempIndex++] = input[i];
    }
    tempInput[tempIndex] = '\0';

    // Main logic for processing variables
    for (i = 0; tempInput[i] != '\0' && resultIndex < resultSize - 1; i++) {
        if (tempInput[i] == '&') {
            int j = 0;
            i++; // Skip the '&' character

            // Read the variable name until the next '&'
            while (tempInput[i] != '&' && tempInput[i] != '\0' && j < sizeof(placeholder) - 1) {
                placeholder[j++] = tempInput[i++];
            }
            placeholder[j] = '\0'; // Null-terminate the variable name

            if (tempInput[i] == '&') {
                // Found the closing '&'
                i++; // Skip the closing '&'
            }

            // Find the variable
            var = find_variable(placeholder);
            if (var != NULL) {
                // Use the variable value based on its type
                if (var->type == TYPE_CHAR) {
                    strcpy(value, var->value.c);
                } else if (var->type == TYPE_INT) {
                    Q_snprintf(value, sizeof(value), "%d", var->value.i);
                } else if (var->type == TYPE_FLOAT) {
                    Q_snprintf(value, sizeof(value), "%.6f", var->value.f);
                }

                // Copy the variable value to the result string
                for (k = 0; value[k] != '\0' && resultIndex < resultSize - 1; k++) {
                    result[resultIndex++] = value[k];
                }
            } else {
                // If the variable is not found, just add '&' and the variable name
                result[resultIndex++] = '&';
                for (k = 0; placeholder[k] != '\0' && resultIndex < resultSize - 1; k++) {
                    result[resultIndex++] = placeholder[k];
                }
                result[resultIndex++] = '&'; // Add the closing '&'
            }
        } else {
            result[resultIndex++] = tempInput[i]; // Copy regular characters
        }
    }
    result[resultIndex] = '\0'; // Null-terminate the result string
}

/*
###############
Функции
###############
*/

char* removeOuterBrackets(const char* str) {
    static char result[MAX_VAR_CHAR_BUF]; // Статический буфер
    size_t len;

    if (str == NULL || *str == '\0') {
        return NULL; // Проверка на пустую строку
    }

    len = strlen(str); // Сначала получаем длину

    // Проверяем наличие скобок в начале и в конце
    if (len > 1 && str[0] == '(' && str[len - 1] == ')') {
        // Убедитесь, что результат не превышает размер буфера
        if (len - 2 < MAX_VAR_CHAR_BUF) {
            strncpy(result, str + 1, len - 2); // Копируем с 1 по len-2
            result[len - 2] = '\0'; // Завершаем строку
            return result; // Возвращаем результат
        } else {
            Com_Printf("Noire.Script Error: Buffer size > 1024\n");
            trap_Cvar_Set("ns_haveerror", "1");
            return NULL; // Если длина строки слишком велика
        }
    }

    return NULL; 
}

int splitArgs(const char* args, char result[MAX_ARGS][MAX_ARG_LENGTH]) {
    const char* start = args;
    const char* end;
    int count = 0;
    int inQuotes = 0; // Флаг, чтобы отслеживать, находимся ли мы внутри кавычек
    int len;
    int i;
    int argIndex;
    const char* p;

    // Инициализация результата
    for (i = 0; i < MAX_ARGS; i++) {
        result[i][0] = '\0'; // Обнуляем каждую строку
    }

    while (*start) {
        // Пропускаем пробелы
        while (*start == ' ') {
            start++;
        }

        if (*start == '\0') {
            break; // Выходим, если конец строки
        }

        // Устанавливаем конец аргумента
        end = start;
        while (*end && (inQuotes || *end != ' ')) {
            // Если находим экранированную кавычку, пропускаем символ '\' и оставляем кавычку
            if (*end == '\\' && *(end + 1) == '"' && inQuotes) {
                // Пропускаем обратный слэш
                end++;
            } else if (*end == '"' || *end == '\'') {
                inQuotes = !inQuotes; // Меняем состояние флага
            }
            end++;
        }

        // Вычисляем длину аргумента
        len = end - start;

        // Если длина больше 0, обрабатываем аргумент
        if (len > 0) {
            // Проверяем, если длина превышает лимит
            if (len >= MAX_ARG_LENGTH) { // Лимит на один аргумент
                return -1; // Ошибка: аргумент слишком длинный
            }

            // Копируем аргумент
            argIndex = 0;
            for (p = start; p < end; p++) {
                // Пропускаем обычные кавычки
                if ((*p == '"' || *p == '\'') && !inQuotes) {
                    continue; // Пропускаем, если кавычка не экранирована
                }

                // Обрабатываем экранированные кавычки
                if (*p == '\\' && *(p + 1) == '"') {
                    p++; // Пропускаем обратный слэш
                }

                // Копируем символ в результат
                result[count][argIndex++] = *p;
            }
            result[count][argIndex] = '\0'; // Завершаем строку

            count++;
        }

        start = end; // Переходим к следующему аргумент

        // Проверяем на превышение максимального количества аргументов
        if (count >= MAX_ARGS) {
            break; // Если превышено, выходим из цикла
        }
    }

    return count; // Возвращаем количество аргументов
}

ArgValue ns_args[MAX_ARGS]; // Массив аргументов типа VarValue

extern void RemoveScriptThread(const char* threadName); //karin: missing decl
#ifdef QAGAME
#include "ns_main_game.c"   //Подстановка сюда кода для qagame.qvm API
#endif
#ifdef CGAME
#include "ns_main_cgame.c"   //Подстановка сюда кода для cgame.qvm API
#endif
#ifdef Q3_UI
#include "ns_main_ui.c"   //Подстановка сюда кода для ui.qvm API
#endif

// Функция для проверки, является ли строка названием функции
int is_function(const char *token) {
    int i;
    
    for (i = 0; i < MAX_FUNCS; i++) {
		if(!function_list[i]) return 0; //karin: use std::strcmp, function_list[i] must not null
        if (strcmp(token, function_list[i]) == 0) {
            if(function_list[i] == NULL){
            Com_Printf("Noire.Script Error: %s - function undefined.\n", token);
            trap_Cvar_Set("ns_haveerror", "1");
            return 0;  // Не нашли функцию   
            }
            return 1;  // Нашли функцию
        }
    }
    return 0;  // Не является функцией
}

// Функция для проверки, является ли строка названием функции
int is_operand(const char *token) {
    return !(is_literal(token) || is_operator(token) || is_function(token));
}

/*
###############
Выполнение
###############
*/

char originalExecuteBuffer[MAX_CYCLE_SIZE]; // Оригинальный буфер для хранения кода
char operationExecuteBuffer[MAX_CYCLE_SIZE]; // Буфер для выполнения кода

void Interpret(char* script) {
    char *pointer = script; // Указатель начала скрипта
    char *token;
    Variable* var;
    int i;

    while ((token = NS_Parse(&pointer)) != NULL) {
        if (token[0] == 0) {
            continue; // Пропускаем пустые токены
        }

        // Обработка токена
        if (is_operator(token)) {
            //Com_Printf("Noire.Script Debug: Operator token: %s\n", token);
        } else if (is_literal(token)) {
            //Com_Printf("Noire.Script Debug: String literal token: %s\n", token);
        } else if (is_function(token)) {
            //Com_Printf("Noire.Script Debug: Function token: %s\n", token);
        } else {
            //Com_Printf("Noire.Script Debug: Operand token: %s\n", token);
        }

        // Упрощённый блок обработки if
        if (strcmp(token, "if") == 0) {
            int firstValue = (int)NS_Exp(NS_Parse(&pointer));
            NSOperator op = NS_CharToOp(NS_Parse(&pointer));
            int secondValue = (int)NS_Exp(NS_Parse(&pointer));

            // Выполняем условие
            if (NS_ifResult(firstValue, op, secondValue)) {
                continue; // Условие истинно, продолжаем обрабатывать токены
            } else {
                // Пропускаем блок if, если условие ложно
                while ((token = NS_Parse(&pointer)) != NULL) {
                    if (strcmp(token, "endif") == 0) {
                        break; // Выход из блока if
                    }
                }
                continue; // Переходим к следующему токену после блока if
            }
        }

        // Упрощённый блок обработки for
        if (strcmp(token, "for") == 0) {
            int iterations = (int)NS_Exp(NS_Parse(&pointer));
            int originalBufferIndex = 0;
            int tokenLength = 0;
            int i = 0;

            // Сначала собираем код внутри блока for в оригинальный буфер
            while ((token = NS_Parse(&pointer)) != NULL) {
                if (strcmp(token, "endfor") == 0) {
                    break; // Выход из блока for
                }

                // Записываем токен в оригинальный буфер с пробелом между токенами
                tokenLength = strlen(token);
                if (originalBufferIndex + tokenLength + 1 < MAX_CYCLE_SIZE) { // +1 для пробела
                    if (originalBufferIndex > 0) {
                        originalExecuteBuffer[originalBufferIndex++] = ' '; // добавляем пробел между токенами
                    }
                    strcpy(originalExecuteBuffer + originalBufferIndex, token);
                    originalBufferIndex += tokenLength;
                } else {
                    // Буфер переполнен
                    Com_Printf("Noire.Script Error: Buffer overflow while processing 'for' loop\n");
                    trap_Cvar_Set("ns_haveerror", "1");
                    break;
                }
            }

            // Завершаем строку
            originalExecuteBuffer[originalBufferIndex] = '\0';

            // Выводим полученный оригинальный буфер в консоль
            //Com_Printf("Noire.Script Debug: Original buffer contents: %s\n", originalExecuteBuffer);

            // Запускаем интерпретацию кода из оригинального буфера нужное количество раз
            for (i = 0; i < iterations; i++) {
                // Копируем содержимое оригинального буфера в буфер для операций
                strcpy(operationExecuteBuffer, originalExecuteBuffer);

                // Выводим номер текущей итерации
                //Com_Printf("Noire.Script Debug: Iteration %d executed.\n", i + 1); // Выводим номер текущей итерации

                // Запускаем интерпретацию кода из буфера для операций
                Interpret(operationExecuteBuffer);
            }

            continue; // Переходим к следующему токену после блока for
        }

        // Упрощённый блок обработки while
        if (strcmp(token, "while") == 0) {
            char *firstValuePointer = pointer;                  //Сохраняем указатель на первое значение
            int firstValue = (int)NS_Exp(NS_Parse(&pointer));
            NSOperator op = NS_CharToOp(NS_Parse(&pointer));
            char *secondValuePointer = pointer;                 //Сохраняем указатель на второе значение
            int secondValue = (int)NS_Exp(NS_Parse(&pointer));
            int originalBufferIndex = 0;
            int tokenLength = 0;

            // Сначала собираем код внутри блока while в оригинальный буфер
            while ((token = NS_Parse(&pointer)) != NULL) {
                if (strcmp(token, "endwhile") == 0) {
                    break; // Выход из блока while
                }

                // Записываем токен в оригинальный буфер с пробелом между токенами
                tokenLength = strlen(token);
                if (originalBufferIndex + tokenLength + 1 < MAX_CYCLE_SIZE) { // +1 для пробела
                    if (originalBufferIndex > 0) {
                        originalExecuteBuffer[originalBufferIndex++] = ' '; // добавляем пробел между токенами
                    }
                    strcpy(originalExecuteBuffer + originalBufferIndex, token);
                    originalBufferIndex += tokenLength;
                } else {
                    // Буфер переполнен
                    Com_Printf("Noire.Script Error: Buffer overflow while processing 'while' loop\n");
                    trap_Cvar_Set("ns_haveerror", "1");
                    break;
                }
            }

            // Завершаем строку
            originalExecuteBuffer[originalBufferIndex] = '\0';

            // Запускаем интерпретацию кода из оригинального буфера, пока условие истинно
            while (NS_ifResult(firstValue, op, secondValue)) {
                // Копируем содержимое оригинального буфера в буфер для операций
                strcpy(operationExecuteBuffer, originalExecuteBuffer);

                // Запускаем интерпретацию кода из буфера для операций
                Interpret(operationExecuteBuffer);

                // Восстанавливаем указатель и повторно считываем значения для проверки условия
                pointer = firstValuePointer; // Возвращаем указатель на первое значение
                firstValue = (int)NS_Exp(NS_Parse(&pointer));
                pointer = secondValuePointer; // Возвращаем указатель на второе значение
                secondValue = (int)NS_Exp(NS_Parse(&pointer));
            }

            continue; // Переходим к следующему токену после блока while
        }

        // Объявление переменных
        if (strcmp(token, "int") == 0 || strcmp(token, "float") == 0 || strcmp(token, "char") == 0) {
            char* varName = NS_Parse(&pointer);
            VarType type;
            VarValue value;
            char* valueToken;
            float resultValue;
            if (variable_exists(varName)){
            continue; // Пропускаем, если переменная уже существует
            }

            type =  strcmp(token, "int") == 0 ? TYPE_INT :
                    strcmp(token, "float") == 0 ? TYPE_FLOAT : TYPE_CHAR;

            token = NS_Parse(&pointer); // Пропускаем "="
            valueToken = NS_Parse(&pointer);
            resultValue = NS_Exp(valueToken);
            switch (type) {
                case TYPE_INT:
                    value.i = (int)resultValue;
                    break;
                case TYPE_FLOAT:
                    value.f = resultValue;
                    break;
                case TYPE_CHAR:
                    NS_Text(valueToken, value.c, sizeof(value.c));
                    break;
                default:
                    break;
            }
            add_variable(varName, value, type);
            continue;
        }

        // Обработка переменных
        var = find_variable(token);
        if (var != NULL) {
            char* op = NS_Parse(&pointer);
            char* valueToken = NS_Parse(&pointer);
            float resultValue;
            if (!is_function(valueToken)) {
                VarValue value;
                resultValue = NS_Exp(valueToken);       //Если не функция то вычисляем значение
                switch (var->type) {
                    case TYPE_INT:
                        value.i = (int)resultValue;
                        break;
                    case TYPE_FLOAT:
                        value.f = resultValue;
                        break;
                    case TYPE_CHAR:
                        NS_Text(valueToken, value.c, sizeof(value.c));
                        break;
                }
                set_variable(var, value, op);
            } else {
                callfunc(var, valueToken, op, NS_Parse(&pointer));
            }
            continue;
        }

        // Обработка функций
        if (is_function(token)) {
            callfunc(NULL, token, NULL, NS_Parse(&pointer));
            continue;
        }
    }
}

/*
###############
Потоки
###############
*/

// Определяем массив для хранения скриптов
ScriptLoop threadsLoops[MAX_SCRIPTS];
int threadsCount = 0;

// Функция для создания потока
void CreateScriptThread(const char* code, const char* threadName, int interval) {
    int i;
    ScriptLoop* script = &threadsLoops[threadsCount]; // Получаем указатель на текущий поток
    if (threadsCount >= MAX_SCRIPTS) {
        Com_Printf("Noire.Script Error: Maximum number of threads reached.\n");
        trap_Cvar_Set("ns_haveerror", "1");
        return;
    }

    // Заполняем структуру ScriptLoop
    strncpy(script->threadName, threadName, MAX_THREAD_NAME - 1);
    script->threadName[MAX_THREAD_NAME - 1] = '\0'; // Убедимся, что строка завершена
    strncpy(script->code, code, MAX_NSSCRIPT_SIZE - 1);
    script->code[MAX_NSSCRIPT_SIZE - 1] = '\0'; // Убедимся, что строка завершена
    script->interval = interval;
    script->lastRunTime = 0; // Инициализируем время последнего запуска

    threadsCount++; // Увеличиваем счетчик потоков

    //Com_Printf("Noire.Script Debug: Script loaded in thread %s with interval %i ms\n", threadName, interval);
}

// Функция для удаления потока по имени
void RemoveScriptThread(const char* threadName) {
    int i, j;
    for (i = 0; i < threadsCount; i++) {
        if (strcmp(threadsLoops[i].threadName, threadName) == 0) {
            // Находим поток для удаления
            //Com_Printf("Noire.Script Debug: Removing thread '%s'.\n", threadName);
            
            // Сдвигаем все последующие потоки влево
            for (j = i; j < threadsCount - 1; j++) {
                threadsLoops[j] = threadsLoops[j + 1];
            }

            threadsCount--; // Уменьшаем счетчик потоков
            return; // Выходим из функции
        }
    }
    Com_Printf("Noire.Script Error: Thread '%s' not found.\n", threadName); // Если поток не найден
}

// Функция для вывода всех работающих потоков
void print_threads() {
    int i;

    if (threadsCount == 0) {
        Com_Printf("Noire.Script Info: No active threads found.\n");
        return;
    }

    Com_Printf("Noire.Script Threads:\n");
    for (i = 0; i < threadsCount; i++) {
        Com_Printf(" - Thread Name: '%s', Interval: %d ms, Last Run Time: %d\n",
                    threadsLoops[i].threadName, threadsLoops[i].interval, threadsLoops[i].lastRunTime);
    }
}

// Функция для проверки существования потока по имени
int thread_exists(const char *threadName) {
    int i;
    // Проходим по массиву потоков
    for (i = 0; i < threadsCount; i++) {
        if (strcmp(threadsLoops[i].threadName, threadName) == 0) {
            return 1; // Поток найден
        }
    }
    return 0; // Поток не найден
}

/*
###############
Запуск
###############
*/

char scriptbuffer[MAX_NSSCRIPT_SIZE];

void NS_OpenScript(const char* filename, const char* threadName, int interval) {
    int len;
    fileHandle_t f;

    // Открыть скрипт с .ns расширением
    len = trap_FS_FOpenFile(filename, &f, FS_READ);

    if (len <= 0) {
        // Если файл NULL его не открыть
        Com_Printf("Noire.Script Error: Could not open script file %s\n", filename);
        return;
    }

    // Читаем файл прямо в буфер
    trap_FS_Read(scriptbuffer, len, f);
    scriptbuffer[len] = '\0'; // Терминальный ноль строки

    // Закроем файл
    trap_FS_FCloseFile(f);

    if(interval <= 0){
        Interpret(scriptbuffer); // Запускаем скрипт сразу
    } else {
        if(!thread_exists(threadName)){
        CreateScriptThread(scriptbuffer, threadName, interval);
        }
    }
}
