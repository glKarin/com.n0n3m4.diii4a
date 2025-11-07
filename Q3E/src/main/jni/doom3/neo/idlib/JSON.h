/*
 * simple JSON parse/stringify by idLexer
 * all functions are not bound-checking/type-checking safety
 * Number has int and float types
 * no constructor/destructor, must be free manually
 * json_t override [] operator and other type converter and Length()
 * json_t json;
 * JSON_Init(json); // json is null type
 * bool ok = JSON_Parse("path.json"); // parse
 * if(ok) {
 *     // auto &res = array[0]["prop1"]["value"]; // chain access(RW)
 *     // auto &res = array / 0 / "prop1" / "value"; // chain access(readonly)
 *     // auto &res = JSON_Find(json, "array[0].prop1[value].1); // find by link
 *     // auto &res = JSON_Find(json, "array[0]/prop1[value]/1); // find by path
 *     // auto &array = json["array_a"]; // operate JSON object
 *     // auto &res = array[0]; // operate JSON array
 *     // auto &i = (int)json["int_a"]; // operate JSON integer
 *     // auto &f = (float)json["float_a"]; // operate JSON float
 *     // auto &b = (bool)json["bool_a"]; // operate JSON bool
 *     // const auto &str = (const char *)json["str_a"]; // operate JSON string
 *     // auto &str = (idStr)json["str_a"]; // operate JSON string
 *     // auto len = json["object_array_string_a"].Length(); // get JSON object keys/array/string length
 *     // char &i = json["str_b"](0); // operate JSON string characters
 *     // idStrList keys = json["object_array_string_a"].Keys(); // get JSON object keys/array indexes/string indexes
 *     // idList<int> indexes = json["object_array_string_a"].IndexKeys(); // get JSON object keys/array/indexes
 *     // b is false if type == JSON_NULL
 *     idStr text;
 *     JSON_ToString(text, json); // stringify
 *     fileSystem->WriteFile("test.json", text.c_str(), text.Length());
 * }
 * JSON_Free(json); // json reset to null type
 */
#ifndef _SIMPLE_JSON_H
#define _SIMPLE_JSON_H

#define JSON_NO_INDENT (-1)

enum jsonType_e {
	JSON_NULL = 0,
	JSON_STRING,
	JSON_BOOL,
    JSON_NUMBER,
	JSON_OBJECT,
	JSON_ARRAY,
};
typedef int jsonType_t;

enum jsonNumberType_e {
    JSONNUMBER_INT        = 1,
    JSONNUMBER_POINT      = 2,
    JSONNUMBER_SCIENTIFIC = 4,
    JSONNUMBER_FLOAT      = 2 | 4,
};
typedef int jsonNumberType_t;

// idHashTable with keys list
template<class Type>
class jsonMap_t
{
	public:
		typedef struct jsonMapEntry_e {
			const char *key;
			const Type *value;
		} jsonMapEntry_t;
		
	public:
		void			Set(const char *key, Type &value)
		{
			if(keys.FindIndex(key) == -1)
				keys.Append(key);
			table.Set(key, value);
		}

		bool			Get(const char *key, Type **value = NULL) const {
			return table.Get(key, value);
		}
		bool			Remove(const char *key) {
			keys.Remove(key);
			table.Remove(key);
		}

		void			Clear(void) {
			keys.Clear();
			table.Clear();
		}
		void			DeleteContents(void) {
            keys.Clear();
			table.DeleteContents();
		}

		int				Num(void) const {
			return keys.Num();
		}
		Type 			*GetIndex(int index) const {
            if(index >= keys.Num())
                return NULL;
            Type *val;
            if(table.Get(keys[index], &val))
                return val;
			return NULL;
		}
		const char *	GetKeyIndex(int index) const {
			return keys[index].c_str();
		}
        int             FindIndex(const char *key) const {
            return keys.FindIndex(key);
        }
		jsonMapEntry_t 	GetKeyVal(int index) const {
			jsonMapEntry_t entry;
			entry.key = keys[index].c_str();
			Type *res;
			table.Get(entry.key, &res);
			entry.value = res;
			return entry;
		}
		const Type &			operator[](const char *key) const {
			Type *val;
			table.Get(key, &val);
			return *val;
		}
		Type &			operator[](const char *key) {
			Type *val;
			if(!table.Get(key, &val))
			{
				Type val;
                Set(key, val);
			}
            table.Get(key, &val);
			return *val;
		}
		const Type &			operator[](int index) const {
			return operator[](keys[index]);
		}
		Type &			operator[](int index) {
			return operator[](keys[index]);
		}
        const idStrList &Keys(void) const {
            return keys;
        }

	private:
		idStrList keys;
		idHashTable<Type> table;
};

union json_u;

typedef idStr jsonString_t;
typedef bool jsonBool_t;
typedef const void * jsonNull_t;
#if 1
typedef int jsonInteger_t;
typedef float jsonFloat_t;
#else
typedef int64_t jsonInteger_t;
typedef double jsonFloat_t;
#endif

template <class FT, class IT>
struct jsonNumberValueWrapper_t
{
    jsonType_t type;
    jsonNumberType_t numberType;
    FT value;
    IT ivalue;
};

template <class T>
struct jsonValueWrapper_t
{
    jsonType_t type;
	T value;
};

typedef union json_u
{
	jsonType_t type;
	jsonValueWrapper_t<jsonString_t *> s;
	jsonValueWrapper_t<jsonBool_t> b;
	jsonNumberValueWrapper_t<jsonFloat_t, jsonInteger_t> n;
	jsonValueWrapper_t<jsonMap_t<union json_u> *> o;
	jsonValueWrapper_t<idList<union json_u> *> a;

#if 0
    ~json_u(void);
#endif

	// Array
	union json_u & operator[](int index);
	const union json_u & operator[](int index) const;
	const union json_u & operator/(int index) const {
		return operator[](index);
	}
	// Object
	union json_u & operator[](const char *name);
	const union json_u & operator[](const char *name) const;
	const union json_u & operator/(const char *name) const{
		return operator[](name);
	}
	// String
	char & operator()(int index);
	char operator()(int index) const;
	int Num(void) const {
		return Length();
	}
	int Length(void) const;
    idStrList Keys(void) const;
    idList<int> IndexKeys(void) const;

	operator const char *(void) const;
	operator jsonString_t &(void);
	operator jsonBool_t(void) const;
	operator jsonBool_t &(void);
	operator jsonInteger_t(void) const;
	operator jsonInteger_t &(void);
	operator jsonFloat_t(void) const;
	operator jsonFloat_t &(void);
//	operator jsonNull_t(void) const {
//		return NULL;
//	}
	operator const idList<union json_u> &(void) const;
	operator idList<union json_u> &(void);
	operator const jsonMap_t<union json_u> &(void) const;
	operator jsonMap_t<union json_u> &(void);

    bool IsNull(void) const {
        return type == JSON_NULL;
    }
} json_t;

typedef idList<union json_u> jsonArray_t;
typedef jsonMap_t<union json_u> jsonObject_t;

void JSON_Init(json_t &json);
bool JSON_Parse(json_t &json, const char *path);
void JSON_ToString(idStr &text, const json_t &json, int indent = 0);
void JSON_Free(json_t &json);
bool JSON_IsNull(const json_t &json);

// obj.field[key1].0.array[1]
const json_t * JSON_Find(const json_t &json, const char *path, char sep = '.');

#endif
