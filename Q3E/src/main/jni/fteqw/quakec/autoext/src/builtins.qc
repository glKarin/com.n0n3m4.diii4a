void(string e) error				= #10;
float(string s) cvar = #45;
float(string s) checkextension = #99;


//FRIK_FILE
//IDEA: FrikaC
float  (string s)                            stof      = #81;  // get numerical value from a string
float  (string filename, float mode)         fopen     = #110; // opens a file inside quake/gamedir/ or quake/gamedir/data/ (mode is FILE_READ, FILE_APPEND, or FILE_WRITE), returns fhandle >= 0 if successful, or fhandle < 0 if unable to open file for any reason
void   (float fhandle)                       fclose    = #111; // closes a file
string (float fhandle)                       fgets     = #112; // reads a line of text from the file and returns as a tempstring
void   (float fhandle, string s)             fputs     = #113; // writes a line of text to the end of the file
float  (string s)                            strlen    = #114; // returns how many characters are in a string
string (string s1, string s2)                strcat    = #115; // concatenates two strings (for example "abc", "def" would return "abcdef") and returns as a tempstring
string (string s, float start, float length) substring = #116; // returns a section of a string as a tempstring
vector (string s)                            stov      = #117; // returns vector value from a string
string (string s)                            strzone   = #118; // makes a copy of a string into the string zone and returns it, this is often used to keep around a tempstring for longer periods of time (tempstrings are replaced often)
void   (string s)                            strunzone = #119; // removes a copy of a string from the string zone (you can not use that string again or it may crash!!!)
float FILE_READ   = 0;
float FILE_APPEND = 1;
float FILE_WRITE  = 2;