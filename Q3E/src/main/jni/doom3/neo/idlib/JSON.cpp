#include "JSON.h"

enum {
	JR_NONE,
	JR_NAME,
	JR_COLON,
	JR_VALUE,
};

static bool JSON_Parse(json_t &json, idLexer &lexer);
static bool JSON_ParseValue(json_t &json, idLexer &lexer);
static bool JSON_ParseObject(json_t &json, idLexer &lexer);
static bool JSON_ParseArray(json_t &json, idLexer &lexer);
static bool JSON_ParseNumber(json_t &json, idLexer &lexer);

static void JSON_ValueToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_ObjectToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_ArrayToString(idStr &text, const json_t &json, int indent = 0);
static void JSON_NumberToString(idStr &text, const json_t &json);

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

		//printf("Obj: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
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
		//printf("Arr: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
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
			else if(token == "{" || token == "[")
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

		//printf("Num: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
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

	//printf("Val: %s|%d|%d\n", token.c_str(), token.type, token.subtype);
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
	if(indent >= 0 && json.o.Num() > 0)
		text.Append("\n");
	for(int i = 0; i < json.o.Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		const char *key = json.o.value->GetKeyIndex(i);
		text.Append("\"");
		text.Append(key);
		text.Append("\":");
		JSON_ValueToString(text, json.o[key], m);
		if(i < json.o.Num() - 1)
		{
			text.Append(",");
			if(indent >= 0)
				text.Append("\n");
		}
	}
	if(indent >= 0 && json.o.Num() > 0)
	{
		text.Append("\n");
		JSON_AppendIndent(text, indent);
	}
	text.Append("}");
}

void JSON_ArrayToString(idStr &text, const json_t &json, int indent)
{
	text.Append("[");
	if(indent >= 0 && json.a.Num() > 0)
		text.Append("\n");
	for(int i = 0; i < json.a.Num(); i++)
	{
		int m = indent < 0 ? -1 : (indent + 1);
		JSON_AppendIndent(text, m);
		JSON_ValueToString(text, json.a[i], m);
		if(i < json.a.Num() - 1)
		{
			text.Append(",");
			if(indent >= 0)
				text.Append("\n");
		}
	}
	if(indent >= 0 && json.a.Num() > 0)
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
	for(int i = 0; i < json.o.Num(); i++)
	{
		JSON_FreeValue(json.o[i]);
	}
}

void JSON_FreeArray(json_t &json)
{
	for(int i = 0; i < json.a.Num(); i++)
	{
		JSON_FreeValue(json.a[i]);
	}
}

void JSON_Free(json_t &json)
{
    JSON_FreeValue(json);
    JSON_Init(json);
}

