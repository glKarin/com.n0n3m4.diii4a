#include "JSON.h"

static json_u def; // thread_local

ID_INLINE static void JSON_NewArray(json_t &json)
{
    if(json.type != JSON_ARRAY)
    {
        JSON_Free(json);
        json.type = JSON_ARRAY;
        json.a.value = new jsonArray_t;
    }
}

ID_INLINE static void JSON_NewObject(json_t &json)
{
    if(json.type != JSON_OBJECT)
    {
        JSON_Free(json);
        json.type = JSON_OBJECT;
        json.o.value = new jsonObject_t;
    }
}

ID_INLINE static void JSON_NewString(json_t &json)
{
    if(json.type != JSON_STRING)
    {
        JSON_Free(json);
        json.type = JSON_STRING;
        json.s.value = new jsonString_t;
    }
}

ID_INLINE static void JSON_NewBool(json_t &json)
{
    if(json.type != JSON_BOOL)
    {
        JSON_Free(json);
        json.type = JSON_BOOL;
        json.b.value = false;
    }
}

ID_INLINE static void JSON_NewNumber(json_t &json, bool isFloat = false)
{
    if(json.type != JSON_NUMBER)
    {
        JSON_Free(json);
        json.type = JSON_NUMBER;
        json.n.value = 0.0f;
        json.n.ivalue = 0;
        json.n.numberType = isFloat ? JSONNUMBER_FLOAT : JSONNUMBER_INT;
    }
}

int json_u::Length(void) const {
    switch(type)
    {
        case JSON_ARRAY:
            return a.value->Num();
        case JSON_OBJECT:
            return o.value->Num();
        case JSON_STRING:
            return s.value->Length();
        default:
            return 0;
    }
}

idStrList json_u::Keys(void) const {
    switch(type)
    {
        case JSON_OBJECT:
            return o.value->Keys();
        case JSON_ARRAY: {
            idStrList list;
            for(int i = 0; i < a.value->Num(); i++)
                list.Append(va("%d", i));
            return list;
        }
        case JSON_STRING: {
            idStrList list;
            for(int i = 0; i < s.value->Length(); i++)
                list.Append(va("%d", i));
            return list;
        }
        default:
            return idStrList();
    }
}
idList<int> json_u::IndexKeys(void) const {
    int num = Length();
    idList<int> list;
    for(int i = 0; i < num; i++)
        list.Append(i);
    return list;
}

json_t & json_u::operator[](int index) {
    JSON_NewArray(*this);
    index = index < 0 ? a.value->Num() + index : index;
    if(index < 0)
        index = 0;
    if(index >= a.value->Num())
        a.value->Resize(index + 1);
    return (*a.value)[index];
}

const union json_u & json_u::operator[](int index) const {
    index = index < 0 ? a.value->Num() + index : index;
    if(type != JSON_ARRAY || index < 0 || index >= a.value->Num())
    {
        JSON_Init(def);
        return def;
    }
    return (*a.value)[index];
}

union json_u & json_u::operator[](const char *name) {
    JSON_NewObject(*this);
    return (*o.value)[name];
}

const union json_u & json_u::operator[](const char *name) const {
    if(type != JSON_OBJECT || o.value->FindIndex(name) < 0)
    {
        JSON_Init(def);
        return def;
    }
    return (*o.value)[name];
}

char & json_u::operator()(int index) {
    JSON_NewString(*this);
    index = index < 0 ? s.value->Length() + index : index;
    if(index < 0)
        index = 0;
    while(index >= s.value->Length())
        s.value->Append('\0');
    return (*s.value)[index];
}

char json_u::operator()(int index) const {
    index = index < 0 ? s.value->Length() + index : index;
    if(type != JSON_STRING || index < 0 || index >= s.value->Length())
    {
        return '\0';
    }
    return (*s.value)[index];
}

json_u::operator const char *(void) const {
    return type == JSON_STRING ? s.value->c_str() : NULL;
}

json_u::operator jsonString_t &(void) {
    JSON_NewString(*this);
    return *s.value;
}

json_u::operator jsonBool_t(void) const {
    switch(type)
    {
        case JSON_OBJECT:
            return NULL != o.value;
        case JSON_ARRAY:
            return NULL != a.value;
        case JSON_STRING:
            return NULL != s.value && !s.value->IsEmpty();
        case JSON_NUMBER:
            return n.value != 0.0f;
        case JSON_BOOL:
            return b.value;
        default:
            return false;
    }
}

json_u::operator jsonBool_t &(void) {
    JSON_NewBool(*this);
    return b.value;
}

json_u::operator jsonInteger_t(void) const {
    return type == JSON_NUMBER ? n.ivalue : 0;
}

json_u::operator jsonInteger_t &(void) {
    JSON_NewNumber(*this, false);
    return n.ivalue;
}

json_u::operator jsonFloat_t(void) const {
    return type == JSON_NUMBER ? n.value : 0.0f;
}

json_u::operator jsonFloat_t &(void) {
    JSON_NewNumber(*this, true);
    return n.value;
}

json_u::operator const idList<union json_u> &(void) const {
    if(type != JSON_ARRAY)
    {
        JSON_NewArray(def);
        return def;
    }
    return *a.value;
}

json_u::operator idList<union json_u> &(void) {
    JSON_NewArray(*this);
    return *a.value;
}

json_u::operator const jsonMap_t<union json_u> &(void) const {
    if(type != JSON_OBJECT)
    {
        JSON_NewObject(def);
        return def;
    }
    return *o.value;
}

json_u::operator jsonMap_t<union json_u> &(void) {
    JSON_NewObject(*this);
    return *o.value;
}

#if 0
json_u::~json_u(void) {
        switch(type)
        {
            case JSON_OBJECT:
                delete o.value;
                break;
            case JSON_ARRAY:
                delete a.value;
                break;
            case JSON_STRING:
                delete s.value;
                break;
            case JSON_BOOL:
            case JSON_NUMBER:
            default:
                break;
        }
    }
#endif

enum {
	JR_NONE,
	JR_NAME,
	JR_COLON,
	JR_VALUE,
};

static bool JSON_ParseValue(json_t &json, idLexer &lexer);
static bool JSON_ParseObject(json_t &json, idLexer &lexer);
static bool JSON_ParseArray(json_t &json, idLexer &lexer);
static bool JSON_ParseNumber(json_t &json, idLexer &lexer);

static void JSON_ValueToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_ObjectToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_ArrayToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_NumberToString(idStr &text, const json_t &json);

static void JSON_ValueToString(idList<char> &text, const json_t &json, int indent = 0);
static void JSON_ObjectToString(idList<char> &text, const json_t &json, int indent = 0);
static void JSON_ArrayToString(idList<char> &text, const json_t &json, int indent = 0);
static void JSON_NumberToString(idList<char> &text, const json_t &json);

static void JSON_FreeValue(json_t &json);
static void JSON_FreeObject(json_t &json);
static void JSON_FreeArray(json_t &json);

void JSON_Init(json_t &json)
{
    json.type = JSON_NULL;
    json.b.value = false;
}

bool JSON_ParseObject(json_t &json, idLexer &lexer)
{
	idToken token;
	bool error = false;
	int st = JR_NONE;
	json_t *res = NULL;

	jsonObject_t *ptr = new jsonObject_t();
	jsonObject_t &obj = *ptr;

	lexer.ExpectTokenString("{");
	while(!error)
	{
		if(!lexer.ReadToken(&token))
		{
			token = "";
			error = true;
			break;
		}

		//Sys_Printf("Obj: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
		if(token.type == TT_STRING)
		{
			if(st == JR_NAME)
			{
				error = true;
				break;
			}
			else if(st == JR_COLON)
			{
				error = true;
				break;
			}
			else if(st == JR_VALUE)
			{
				error = true;
				break;
			}

			json_t val;
			obj.Set(token.c_str(), val);
			obj.Get(token.c_str(), &res);
			st = JR_NAME;
			continue;
		}
		else if(token.type == TT_PUNCTUATION)
		{
			if(token == ":")
			{
				if(st == JR_NONE)
				{
					error = true;
					break;
				}
				else if(st == JR_COLON)
				{
					error = true;
					break;
				}
				else if(st == JR_VALUE)
				{
					error = true;
					break;
				}
				st = JR_COLON;
				if(!JSON_ParseValue(*res, lexer))
				{
					error = true;
					break;
				}

				st = JR_VALUE;
				continue;
			}
			else if(token == ",")
			{
				if(st == JR_NONE)
				{
					error = true;
					break;
				}
				else if(st == JR_NAME)
				{
					error = true;
					break;
				}
				else if(st == JR_COLON)
				{
					error = true;
					break;
				}

				st = JR_NONE;
				continue;
			}
			else if(token == "}")
			{
				if(st == JR_NAME)
				{
					error = true;
					break;
				}
				else if(st == JR_COLON)
				{
					error = true;
					break;
				}

				break;
			}
		}

		error = true;
		break;
	}

	if(error)
	{
		if(st == JR_NONE)
		{
			lexer.Error("Parse JSONObject: Expect name string but found '%s'\n", token.c_str());
		}
		else if(st == JR_NAME)
		{
			lexer.Error("Parse JSONObject: Expect ':' but found '%s'\n", token.c_str());
		}
		else if(st == JR_COLON)
		{
			lexer.Error("Parse JSONObject: Expect value but found '%s'\n", token.c_str());
		}
		else if(st == JR_VALUE)
		{
			lexer.Error("Parse JSONObject: Expect ',' or '}' but found '%s'\n", token.c_str());
		}

		delete ptr;
	}
	else
	{
		json.type = JSON_OBJECT;
		json.o.value = ptr;
	}

	return !error;
}

bool JSON_ParseArray(json_t &json, idLexer &lexer)
{
	idToken token;
	int index;
	bool error = false;
	int st = JR_NONE;

	jsonArray_t *ptr = new jsonArray_t();
	jsonArray_t &list = *ptr;

	lexer.ExpectTokenString("[");
	while(!error)
	{
		if(!lexer.ReadToken(&token))
		{
			error = true;
			break;
		}
		//Sys_Printf("Arr: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
		if(token.type == TT_PUNCTUATION)
		{
			if(token == ",")
			{
				if(st == JR_NONE)
				{
					error = true;
					break;
				}

				st = JR_NONE;
				continue;
			}
			else if(token == "]")
			{
				break;
			}
			else if(token == "{" || token == "[" || token == "-"/* if negative number */)
			{
				if(st != JR_NONE)
				{
					error = true;
					break;
				}

				index = list.Append(json_t());
				lexer.UnreadToken(&token);
				if(!JSON_ParseValue(list[index], lexer))
				{
					error = true;
					break;
				}

				st = JR_VALUE;
			}
		}
		else
		{
			if(st != JR_NONE)
			{
				error = true;
				break;
			}
			index = list.Append(json_t());
			lexer.UnreadToken(&token);
			if(!JSON_ParseValue(list[index], lexer))
			{
				error = true;
				break;
			}

			st = JR_VALUE;
		}
	}

	if(error)
	{
		if(st == JR_NONE)
		{
			lexer.Error("Parse JSONArray: Expect value but found '%s'\n", token.c_str());
		}
		else
		{
			lexer.Error("Parse JSONArray: Expect ',' or ']' but found '%s'\n", token.c_str());
		}

		delete ptr;
	}
	else
	{
		json.type = JSON_ARRAY;
		json.a.value = ptr;
	}

	return !error;
}

bool JSON_ParseNumber(json_t &json, idLexer &lexer)
{
	idToken token;
	bool neg = false;
	bool number = false;
	idStr str;
	bool isFloat = false;
	bool e = false;
	bool eNumber = false;
	bool eNeg = false;
	bool end = false;
	bool unread = false;

	while(true)
	{
		if(!lexer.ReadToken(&token))
		{
			break;
		}

		//Sys_Printf("Num: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
		if(token.type == TT_NUMBER)
		{
			if(e)
			{
				if(eNumber)
				{
					end = true;
					unread = true;
				}
				else
				{
					if(token.subtype & TT_INTEGER)
					{
						if(token.GetIntValue() != 0)
						{
							eNumber = true;
							str.Append(token.c_str());
						}
						else
							continue;
					}
					else
					{
						end = true;
						unread = true;
					}
				}
			}
			else
			{
				if(number)
				{
					end = true;
					unread = true;
				}
				else
				{
					number = true;
					if(token.subtype & TT_INTEGER)
					{
						isFloat = false;
					}
					else
					{
						isFloat = true;
					}
					str.Append(token.c_str());
				}
			}
		}
		else if(token.type == TT_LITERAL || token.type == TT_NAME)
		{
			if(!idStr::Icmp(token, "e"))
			{
				if(e)
				{
					end = true;
					unread = true;
				}
				else if(number)
				{
					isFloat = true;
					e = true;
					str.Append(token.c_str());
				}
				else
				{
					end = true;
					unread = true;
				}
			}
			else
			{
				end = true;
				unread = true;
			}
		}
		else if(token.type == TT_PUNCTUATION)
		{
			if(token == "-")
			{
				if(e)
				{
					if(eNeg)
					{
						end = true;
						unread = true;
					}
					else if(!eNumber)
					{
						eNeg = true;
						str.Append(token.c_str());
					}
					else
					{
						end = true;
						unread = true;
					}
				}
				else
				{
					if(neg)
					{
						end = true;
						unread = true;
					}
					else if(!number)
					{
						neg = true;
						str.Append(token.c_str());
					}
					else
					{
						end = true;
						unread = true;
					}
				}
			}
			else
			{
				end = true;
				unread = true;
			}
		}
		else
		{
			end = true;
			unread = true;
			break;
		}

		if(end)
			break;
	}

	if(!number)
	{
		lexer.Error("Parse JSONNumber: Except number value\n");
		return false;
	}
	else if(e && !eNumber)
	{
		lexer.Error("Parse JSONNumber: Except E number value\n");
		return false;
	}

	if(isFloat)
	{
		float d;
		if(e)
		{
			if(sscanf(str.c_str(), "%e", &d) == 1)
            {
                json.n.type = JSON_NUMBER;
                json.n.numberType = JSONNUMBER_SCIENTIFIC;
                json.n.value = (jsonFloat_t)d;
                json.n.ivalue = (jsonInteger_t)d;
            }
			else
			{
				lexer.Error("Parse JSONNumber: Parse E float number error '%s'\n", str.c_str());
				return false;
			}
		}
		else
		{
			if(sscanf(str.c_str(), "%f", &d) == 1)
            {
                json.n.type = JSON_NUMBER;
                json.n.numberType = JSONNUMBER_POINT;
                json.n.value = (jsonFloat_t)d;
                json.n.ivalue = (jsonInteger_t)d;
            }
			else
			{
				lexer.Error("Parse JSONNumber: Parse float number error '%s'\n", str.c_str());
				return false;
			}
		}
	}
	else
	{
		int d;
		if(sscanf(str.c_str(), "%d", &d) == 1)
        {
            json.n.type = JSON_NUMBER;
            json.n.numberType = JSONNUMBER_INT;
            json.n.value = (jsonFloat_t)d;
            json.n.ivalue = (jsonInteger_t)d;
        }
		else
		{
			lexer.Error("Parse JSONNumber: Parse integer number error '%s'\n", str.c_str());
			return false;
		}
	}

	if(unread)
		lexer.UnreadToken(&token);

	return true;
}

bool JSON_ParseValue(json_t &json, idLexer &lexer)
{
	idToken token;

	if(!lexer.ReadToken(&token))
	{
		lexer.Error("Parse JSONValue: Except any value\n");
		return false;
	}

	//Sys_Printf("Val: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
	switch(token.type)
	{
		case TT_NUMBER:
			lexer.UnreadToken(&token);
			return JSON_ParseNumber(json, lexer);
			break;
		case TT_STRING:
			json.type = JSON_STRING; 
			json.s.value = new idStr(token.c_str());
			break;
		case TT_LITERAL:
		case TT_NAME:
			if(token == "true")
			{
				json.type = JSON_BOOL; 
				json.b.value = true;
			}
			else if(token == "false")
			{
				json.type = JSON_BOOL; 
				json.b.value = false;
			}
			else if(token == "null")
			{
				json.type = JSON_NULL;
				json.b.value = false;
			}
			else
			{
				lexer.Error("Parse JSONValue: Unexpect literal token '%s'\n", token.c_str());
				return false;
			}
			break;
		case TT_PUNCTUATION:
			if(token == "[")
			{
				lexer.UnreadToken(&token);
				return JSON_ParseArray(json, lexer);
			}
			else if(token == "{")
			{
				lexer.UnreadToken(&token);
				return JSON_ParseObject(json, lexer);
			}
			else if(token == "-")
			{
				lexer.UnreadToken(&token);
				return JSON_ParseNumber(json, lexer);
			}
			else
			{
				lexer.Error("Parse JSONValue: Unexpect punctuation token '%s'\n", token.c_str());
				return false;
			}
			break;
		default:
			lexer.Error("Parse JSONValue: Unexpect other type token: %s\n", token.c_str());
			return false;
	}

	return true;
}

bool JSON_Parse(json_t &json, const char *data, int length)
{
	idLexer lexer;

	if(!lexer.LoadMemory(data, length, "<implicit json file>"))
		return false;

	return JSON_Parse(json, lexer);
}

bool JSON_Parse(json_t &json, idLexer &lexer)
{
	idToken token;

	if(lexer.ReadToken(&token))
	{
		if(token == "{")
		{
			lexer.UnreadToken(&token);
			return JSON_ParseObject(json, lexer);
		}
		else if(token == "[")
		{
			lexer.UnreadToken(&token);
			return JSON_ParseArray(json, lexer);
		}
		else
		{
			lexer.Error("Parse JSON: root must be object or array, but found '%s'\n", token.c_str());
			return false;
		}
	}

	return false;
}

bool JSON_Parse(json_t &json, const char *path)
{
	idLexer lexer;

	if(!lexer.LoadFile(path))
		return false;

	return JSON_Parse(json, lexer);
}

static void JSON_AppendIndent(idStr &text, int indent)
{
	for(int i = 0; i < indent; i++)
		text.Append("  ");
}

void JSON_ToString(idStr &text, const json_t &json, int indent)
{
	if(json.type == JSON_OBJECT)
		JSON_ObjectToString(text, json, indent);
	else if(json.type == JSON_ARRAY)
		JSON_ArrayToString(text, json, indent);
	else
		JSON_ValueToString(text, json, indent);
}

void JSON_NumberToString(idStr &text, const json_t &json)
{
    switch (json.n.numberType) {
        case JSONNUMBER_INT:
            text.Append(va("%d", json.n.ivalue));
            break;
        case JSONNUMBER_POINT: {
            idStr str = va("%f", json.n.value);
            int index = str.Find('.');
            if(index > 0)
            {
                str.StripTrailing('0');
                if(str[str.Length() - 1] == '.')
                    str.Append('0');
            }
            text.Append(str.c_str());
        }
            break;
        case JSONNUMBER_SCIENTIFIC: {
            idStr str = va("%E", json.n.value);
            int index = str.Find('.');
            idStr nstr;
            if(index > 0)
            {
                int eindex = str.Find('E');
                if(eindex > index)
                {
                    nstr.Append(str.Left(index + 1));
                    int i;
                    for(i = eindex - 1; i > index; i--)
                    {
                        if(str[i] != '0')
                            break;
                    }
                    if(i > index)
                        nstr.Append(str.Mid(index + 1, i - index));
                    nstr.Append(str.Right(str.Length() - eindex));
                    nstr.StripTrailing('0');
                }
                else
                {
                    nstr = str;
                    nstr.StripTrailing('0');
                    if(nstr[nstr.Length() - 1] == '.')
                        nstr.Append('0');
                }
            }
            else
            {
                nstr = str;
                nstr.StripTrailing('0');
            }
            text.Append(nstr.c_str());
        }
            break;
        default:
            text.Append(va("%G", json.n.value));
            break;
    }
}

void JSON_ValueToString(idStr &text, const json_t &json, int indent)
{
	switch(json.type)
	{
		case JSON_OBJECT:
			JSON_ObjectToString(text, json, indent);
			break;
		case JSON_ARRAY:
			JSON_ArrayToString(text, json, indent);
			break;
		case JSON_BOOL:
			text.Append(json.b.value ? "true" : "false");
			break;
		case JSON_NUMBER:
            JSON_NumberToString(text, json);
			break;
		case JSON_STRING:
			text.Append("\"");
			text.Append(*json.s.value);
			text.Append("\"");
			break;
		case JSON_NULL:
		default:
			text.Append("null");
			break;
	}
}

void JSON_ObjectToString(idStr &text, const json_t &json, int indent)
{
	text.Append("{");
	if(indent >= 0 && json.o.value->Num() > 0)
		text.Append("\n");
	for(int i = 0; i < json.o.value->Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		const char *key = json.o.value->GetKeyIndex(i);
		text.Append("\"");
		text.Append(key);
		text.Append("\":");
		JSON_ValueToString(text, (*json.o.value)[key], m);
		if(i < json.o.value->Num() - 1)
		{
			text.Append(",");
			if(indent >= 0)
				text.Append("\n");
		}
	}
	if(indent >= 0 && json.o.value->Num() > 0)
	{
		text.Append("\n");
		JSON_AppendIndent(text, indent);
	}
	text.Append("}");
}

void JSON_ArrayToString(idStr &text, const json_t &json, int indent)
{
	text.Append("[");
	if(indent >= 0 && json.a.value->Num() > 0)
		text.Append("\n");
	for(int i = 0; i < json.a.value->Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		JSON_ValueToString(text, (*json.a.value)[i], m);
		if(i < json.a.value->Num() - 1)
		{
			text.Append(",");
			if(indent >= 0)
				text.Append("\n");
		}
	}
	if(indent >= 0 && json.a.value->Num() > 0)
	{
		text.Append("\n");
		JSON_AppendIndent(text, indent);
	}
	text.Append("]");
}

void JSON_FreeValue(json_t &json)
{
	switch(json.type)
	{
		case JSON_OBJECT:
			JSON_FreeObject(json);
			delete json.o.value;
			break;
		case JSON_ARRAY:
			JSON_FreeArray(json);
			delete json.a.value;
			break;
		case JSON_STRING:
			delete json.s.value;
			break;
		case JSON_BOOL:
		case JSON_NUMBER:
		default:
			break;
	}
}

void JSON_FreeObject(json_t &json)
{
	for(int i = 0; i < json.o.value->Num(); i++)
	{
		JSON_FreeValue((*json.o.value)[i]);
	}
}

void JSON_FreeArray(json_t &json)
{
	for(int i = 0; i < json.a.value->Num(); i++)
	{
		JSON_FreeValue((*json.a.value)[i]);
	}
}

void JSON_Free(json_t &json)
{
    JSON_FreeValue(json);
    JSON_Init(json);
}

bool JSON_IsNull(const json_t &json)
{
    return json.type == JSON_NULL;
}

static void JSON_AppendString(idList<char> &text, const char *str)
{
	int len = strlen(str);
	for(int i = 0; i < len; i++)
	{
		text.Append(str[i]);
	}
}

static void JSON_AppendString(idList<char> &text, const idStr &str)
{
	for(int i = 0; i < str.Length(); i++)
	{
		text.Append(str[i]);
	}
}

static void JSON_AppendIndent(idList<char> &text, int indent)
{
	for(int i = 0; i < indent; i++)
	{
		JSON_AppendString(text, "  ");
	}
}

void JSON_ToArray(idList<char> &text, const json_t &json, int indent)
{
	if(json.type == JSON_OBJECT)
		JSON_ObjectToString(text, json, indent);
	else if(json.type == JSON_ARRAY)
		JSON_ArrayToString(text, json, indent);
	else
		JSON_ValueToString(text, json, indent);
}

void JSON_NumberToString(idList<char> &text, const json_t &json)
{
    switch (json.n.numberType) {
        case JSONNUMBER_INT:
            JSON_AppendString(text, va("%d", json.n.ivalue));
            break;
        case JSONNUMBER_POINT: {
            idStr str = va("%f", json.n.value);
            int index = str.Find('.');
            if(index > 0)
            {
                str.StripTrailing('0');
                if(str[str.Length() - 1] == '.')
                    str.Append('0');
            }
            JSON_AppendString(text, str);
        }
            break;
        case JSONNUMBER_SCIENTIFIC: {
            idStr str = va("%E", json.n.value);
            int index = str.Find('.');
            idStr nstr;
            if(index > 0)
            {
                int eindex = str.Find('E');
                if(eindex > index)
                {
                    nstr.Append(str.Left(index + 1));
                    int i;
                    for(i = eindex - 1; i > index; i--)
                    {
                        if(str[i] != '0')
                            break;
                    }
                    if(i > index)
                        nstr.Append(str.Mid(index + 1, i - index));
                    nstr.Append(str.Right(str.Length() - eindex));
                    nstr.StripTrailing('0');
                }
                else
                {
                    nstr = str;
                    nstr.StripTrailing('0');
                    if(nstr[nstr.Length() - 1] == '.')
                        nstr.Append('0');
                }
            }
            else
            {
                nstr = str;
                nstr.StripTrailing('0');
            }
            JSON_AppendString(text, nstr);
        }
            break;
        default:
            JSON_AppendString(text, va("%G", json.n.value));
            break;
    }
}

void JSON_ValueToString(idList<char> &text, const json_t &json, int indent)
{
	switch(json.type)
	{
		case JSON_OBJECT:
			JSON_ObjectToString(text, json, indent);
			break;
		case JSON_ARRAY:
			JSON_ArrayToString(text, json, indent);
			break;
		case JSON_BOOL:
			JSON_AppendString(text, json.b.value ? "true" : "false");
			break;
		case JSON_NUMBER:
            JSON_NumberToString(text, json);
			break;
		case JSON_STRING:
			JSON_AppendString(text, "\"");
			JSON_AppendString(text, *json.s.value);
			JSON_AppendString(text, "\"");
			break;
		case JSON_NULL:
		default:
			JSON_AppendString(text, "null");
			break;
	}
}

void JSON_ObjectToString(idList<char> &text, const json_t &json, int indent)
{
	JSON_AppendString(text, "{");
	if(indent >= 0 && json.o.value->Num() > 0)
		JSON_AppendString(text, "\n");
	for(int i = 0; i < json.o.value->Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		const char *key = json.o.value->GetKeyIndex(i);
		JSON_AppendString(text, "\"");
		JSON_AppendString(text, key);
		JSON_AppendString(text, "\":");
		JSON_ValueToString(text, (*json.o.value)[key], m);
		if(i < json.o.value->Num() - 1)
		{
			JSON_AppendString(text, ",");
			if(indent >= 0)
				JSON_AppendString(text, "\n");
		}
	}
	if(indent >= 0 && json.o.value->Num() > 0)
	{
		JSON_AppendString(text, "\n");
		JSON_AppendIndent(text, indent);
	}
	JSON_AppendString(text, "}");
}

void JSON_ArrayToString(idList<char> &text, const json_t &json, int indent)
{
	JSON_AppendString(text, "[");
	if(indent >= 0 && json.a.value->Num() > 0)
		JSON_AppendString(text, "\n");
	for(int i = 0; i < json.a.value->Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		JSON_ValueToString(text, (*json.a.value)[i], m);
		if(i < json.a.value->Num() - 1)
		{
			JSON_AppendString(text, ",");
			if(indent >= 0)
				JSON_AppendString(text, "\n");
		}
	}
	if(indent >= 0 && json.a.value->Num() > 0)
	{
		JSON_AppendString(text, "\n");
		JSON_AppendIndent(text, indent);
	}
	JSON_AppendString(text, "]");
}

static bool JSON_FindValue(const json_t *&cur, const char *token)
{
    int len = strlen(token);
    bool isNum = true;
    for(int i = 0; i < len; i++)
    {
        if(token[i] < '0' || token[i] > '9')
        {
            isNum = false;
            break;
        }
    }

    if(cur->type == JSON_OBJECT)
    {
        int index = cur->o.value->FindIndex(token);
        if(index != -1)
            cur = cur->o.value->GetIndex(index);
        else
        {
            printf("[JSON]: not found key '%s' in object\n", token);
            cur = NULL;
            return false;
        }
    }
    else if(cur->type == JSON_ARRAY)
    {
        if(isNum)
        {
            int index;
            sscanf(token, "%d", &index);
            if(sscanf(token, "%d", &index) == 1)
            {
                if(index < cur->a.value->Num())
                    cur = &cur->a.value->operator[](index);
                else
                {
                    printf("[JSON]: not found index '%s' in array\n", token);
                    cur = NULL;
                    return false;
                }
            }
            else
            {
                printf("[JSON]: not array index '%s'\n", token);
                cur = NULL;
                return false;
            }
        }
        else
        {
            printf("[JSON]: index '%s' not number in array\n", token);
            cur = NULL;
            return false;
        }
    }
    else
    {
        printf("[JSON]: not object or array\n");
        cur = NULL;
        return false;
    }
    return true;
}

const json_t * JSON_Find(const json_t &json, const char *path, char sep)
{
    enum {
        JSON_FIND_KEY,
        JSON_FIND_SEP,
        JSON_FIND_BRACE,
    };

    const char *ptr = path;
    idStr token;
    const json_t *cur = &json;
    int st = JSON_FIND_KEY;

    while(true)
    {
        if(!cur)
        {
            printf("[JSON]: current root value is null\n");
            break;
        }

        if(*ptr == '\0')
        {
            if(st == JSON_FIND_SEP)
            {
                printf("[JSON]: require key, but read EOL\n");
                cur = NULL;
                break;
            }

            if(!token.IsEmpty())
            {
                JSON_FindValue(cur, token.c_str());
            }

            break;
        }
        else if(*ptr == sep)
        {
            if(st == JSON_FIND_SEP)
            {
                printf("[JSON]: require key, but found separator '%c'\n", sep);
                cur = NULL;
                break;
            }

            if(st == JSON_FIND_KEY)
            {
                if(token.IsEmpty())
                {
                    printf("[JSON]: current key is empty when find '%c'\n", sep);
                    cur = NULL;
                    break;
                }

                if(!JSON_FindValue(cur, token.c_str()))
                {
                    break;
                }
            }

            token.Clear();
            st = JSON_FIND_SEP;
        }
        else if(*ptr == '[')
        {
            if(st == JSON_FIND_SEP)
            {
                printf("[JSON]: require key, but found '['\n");
                cur = NULL;
                break;
            }

            if(!token.IsEmpty())
            {
                if(!JSON_FindValue(cur, token.c_str()))
                {
                    break;
                }

                token.Clear();
            }
            ptr++;

            while(*ptr)
            {
                if(*ptr == ']')
                    break;
                token.Append(*ptr);
                ptr++;
            }

            if(*ptr != ']')
            {
                printf("[JSON]: missing ']'\n");
                cur = NULL;
                break;
            }

            if(token.IsEmpty())
            {
                printf("[JSON]: index is empty in []\n");
                cur = NULL;
                break;
            }

            if(!JSON_FindValue(cur, token.c_str()))
            {
                break;
            }
            token.Clear();
            st = JSON_FIND_BRACE;
        }
        else
        {
            if(st == JSON_FIND_BRACE)
            {
                printf("[JSON]: require . or [, but found '%c'\n", *ptr);
                cur = NULL;
                break;
            }
            token.Append(*ptr);
            st = JSON_FIND_KEY;
        }

        ptr++;
    }

    return cur;
}
